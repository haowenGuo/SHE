#include "RogueLiteGame.hpp"

#include <algorithm>
#include <cmath>

namespace she
{
namespace
{
constexpr float kRoomHalfWidth = 10.5F;
constexpr float kRoomHalfHeight = 6.5F;
constexpr float kPlayerRadius = 0.45F;
constexpr float kPortalRadius = 0.85F;
constexpr float kPlayerMoveSpeed = 5.6F;
constexpr float kDashDistance = 2.9F;
constexpr double kDashCooldownSeconds = 1.1;
constexpr double kPlayerInvulnerabilitySeconds = 0.45;

[[nodiscard]] Vector2 Add(Vector2 a, Vector2 b)
{
    return Vector2{a.x + b.x, a.y + b.y};
}

[[nodiscard]] Vector2 Subtract(Vector2 a, Vector2 b)
{
    return Vector2{a.x - b.x, a.y - b.y};
}

[[nodiscard]] Vector2 Scale(Vector2 value, float scalar)
{
    return Vector2{value.x * scalar, value.y * scalar};
}

[[nodiscard]] float LengthSquared(Vector2 value)
{
    return value.x * value.x + value.y * value.y;
}

[[nodiscard]] float Length(Vector2 value)
{
    return std::sqrt(LengthSquared(value));
}
} // namespace

const char* GetRogueLitePhaseName(const RogueLiteGamePhase phase)
{
    switch (phase)
    {
    case RogueLiteGamePhase::Combat:
        return "Combat";
    case RogueLiteGamePhase::RoomClear:
        return "RoomClear";
    case RogueLiteGamePhase::Victory:
        return "Victory";
    case RogueLiteGamePhase::GameOver:
        return "GameOver";
    default:
        return "Unknown";
    }
}

const char* GetRogueLiteEnemyName(const RogueLiteEnemyKind kind)
{
    switch (kind)
    {
    case RogueLiteEnemyKind::Grunt:
        return "Grunt";
    case RogueLiteEnemyKind::Shooter:
        return "Shooter";
    case RogueLiteEnemyKind::Brute:
        return "Brute";
    default:
        return "Unknown";
    }
}

const char* GetRogueLitePickupName(const RogueLitePickupKind kind)
{
    switch (kind)
    {
    case RogueLitePickupKind::Core:
        return "Core";
    case RogueLitePickupKind::Heart:
        return "Heart";
    default:
        return "Unknown";
    }
}

RogueLiteGame::RogueLiteGame() : m_rooms(BuildRoomDefinitions())
{
    Reset();
}

void RogueLiteGame::Reset(const std::uint32_t seed)
{
    m_seed = seed;
    m_playerHitPoints = 5;
    m_playerMaxHitPoints = 5;
    m_scrap = 0;
    m_upgradeLevel = 0;
    m_lastAimDirection = Vector2{0.0F, 1.0F};
    m_lastMoveDirection = Vector2{0.0F, 1.0F};
    m_playerFireCooldown = 0.0;
    m_dashCooldown = 0.0;
    m_invulnerabilitySeconds = 0.0;
    m_phase = RogueLiteGamePhase::Combat;
    LoadRoom(0);
    m_latestStatus = "Room 1 hot drop. Clear the hostiles.";
}

RogueLiteUpdateResult RogueLiteGame::Update(const double deltaSeconds, Vector2 moveInput)
{
    RogueLiteUpdateResult result;
    if (m_rooms.empty())
    {
        return result;
    }

    moveInput = NormalizeMove(moveInput);
    if (LengthSquared(moveInput) > 0.0F)
    {
        m_lastMoveDirection = moveInput;
    }

    m_playerFireCooldown = std::max(0.0, m_playerFireCooldown - deltaSeconds);
    m_dashCooldown = std::max(0.0, m_dashCooldown - deltaSeconds);
    m_invulnerabilitySeconds = std::max(0.0, m_invulnerabilitySeconds - deltaSeconds);

    if (m_phase == RogueLiteGamePhase::Combat || m_phase == RogueLiteGamePhase::RoomClear)
    {
        const float moveSpeed = kPlayerMoveSpeed + static_cast<float>(m_upgradeLevel) * 0.18F;
        m_playerPosition = ClampInsideRoom(
            Add(m_playerPosition, Scale(moveInput, moveSpeed * static_cast<float>(deltaSeconds))),
            kPlayerRadius);
    }

    if (m_phase == RogueLiteGamePhase::GameOver || m_phase == RogueLiteGamePhase::Victory)
    {
        return result;
    }

    if (m_pendingWaveSpawn)
    {
        m_waveSpawnDelay -= deltaSeconds;
        if (m_waveSpawnDelay <= 0.0)
        {
            m_pendingWaveSpawn = false;
            SpawnCurrentWave();
            result.stateChanged = true;
            result.waveSpawned = true;
        }
    }

    for (ProjectileRuntime& projectile : m_projectiles)
    {
        projectile.position = Add(projectile.position, Scale(projectile.velocity, static_cast<float>(deltaSeconds)));
        projectile.lifetimeSeconds -= deltaSeconds;
    }

    for (EnemyRuntime& enemy : m_enemies)
    {
        const Vector2 toPlayer = Subtract(m_playerPosition, enemy.position);
        const float distance = Length(toPlayer);
        const Vector2 direction = distance > 0.001F ? Scale(toPlayer, 1.0F / distance) : Vector2{};

        if (enemy.kind == RogueLiteEnemyKind::Shooter)
        {
            const float idealDistance = 4.8F;
            if (distance > idealDistance + 0.7F)
            {
                enemy.position = ClampInsideRoom(
                    Add(enemy.position, Scale(direction, enemy.moveSpeed * static_cast<float>(deltaSeconds))),
                    enemy.radius);
            }
            else if (distance < idealDistance - 0.7F)
            {
                enemy.position = ClampInsideRoom(
                    Add(enemy.position, Scale(direction, -enemy.moveSpeed * static_cast<float>(deltaSeconds))),
                    enemy.radius);
            }

            enemy.shotCooldown = std::max(0.0, enemy.shotCooldown - deltaSeconds);
            if (enemy.shotCooldown <= 0.0 && distance > 0.001F)
            {
                m_projectiles.push_back(
                    ProjectileRuntime{
                        false,
                        Add(enemy.position, Scale(direction, enemy.radius + 0.28F)),
                        Scale(direction, 6.0F),
                        0.16F,
                        1,
                        1.6});
                enemy.shotCooldown = 1.05;
            }
        }
        else
        {
            enemy.position = ClampInsideRoom(
                Add(enemy.position, Scale(direction, enemy.moveSpeed * static_cast<float>(deltaSeconds))),
                enemy.radius);
        }
    }

    for (ProjectileRuntime& projectile : m_projectiles)
    {
        if (projectile.lifetimeSeconds <= 0.0)
        {
            continue;
        }

        if (projectile.fromPlayer)
        {
            static_cast<void>(ResolvePlayerProjectileHit(projectile, result));
        }
        else
        {
            static_cast<void>(ResolveEnemyProjectileHit(projectile, result));
        }
    }

    ResolvePlayerContactDamage(result);
    RemoveDeadEnemies(result);
    ResolvePickupCollection(result);
    RemoveExpiredProjectiles();

    if (m_phase == RogueLiteGamePhase::Combat && m_enemies.empty() && !m_pendingWaveSpawn)
    {
        const int totalWaves = static_cast<int>(m_rooms[static_cast<std::size_t>(m_currentRoomIndex)].waves.size());
        if (m_currentWaveIndex + 1 < totalWaves)
        {
            ++m_currentWaveIndex;
            m_pendingWaveSpawn = true;
            m_waveSpawnDelay = 0.75;
            m_latestStatus = "Wave cleared. Incoming signatures detected.";
            result.stateChanged = true;
        }
        else
        {
            m_phase = RogueLiteGamePhase::RoomClear;
            m_portalOpen = true;
            SpawnRoomReward();
            m_latestStatus = "Room clear. Grab the reward core and move to the portal.";
            result.stateChanged = true;
            result.roomCleared = true;
            result.pickupSpawned = true;
        }
    }

    if (IsPortalReachable() && TryAdvanceToNextRoom())
    {
        result.stateChanged = true;
        result.roomAdvanced = true;
        if (m_phase == RogueLiteGamePhase::Victory)
        {
            result.runWon = true;
        }
    }

    return result;
}

RogueLiteActionResult RogueLiteGame::ApplyAction(const RogueLiteAction action)
{
    switch (action)
    {
    case RogueLiteAction::Restart:
    {
        Reset(m_seed);
        RogueLiteActionResult result;
        result.stateChanged = true;
        result.restarted = true;
        return result;
    }
    case RogueLiteAction::FireUp:
        return TryFireProjectile(Vector2{0.0F, 1.0F});
    case RogueLiteAction::FireDown:
        return TryFireProjectile(Vector2{0.0F, -1.0F});
    case RogueLiteAction::FireLeft:
        return TryFireProjectile(Vector2{-1.0F, 0.0F});
    case RogueLiteAction::FireRight:
        return TryFireProjectile(Vector2{1.0F, 0.0F});
    case RogueLiteAction::Dash:
        return TryDash();
    default:
        return {};
    }
}

RogueLiteSnapshot RogueLiteGame::BuildSnapshot() const
{
    RogueLiteSnapshot snapshot;
    snapshot.phase = m_phase;
    snapshot.roomIndex = m_currentRoomIndex;
    snapshot.roomCount = static_cast<int>(m_rooms.size());
    snapshot.currentWave = m_currentWaveIndex + 1;
    snapshot.totalWaves = static_cast<int>(m_rooms[static_cast<std::size_t>(m_currentRoomIndex)].waves.size());
    snapshot.playerHitPoints = m_playerHitPoints;
    snapshot.playerMaxHitPoints = m_playerMaxHitPoints;
    snapshot.scrap = m_scrap;
    snapshot.upgradeLevel = m_upgradeLevel;
    snapshot.enemiesRemaining = static_cast<int>(m_enemies.size());
    snapshot.activeProjectileCount = static_cast<int>(m_projectiles.size());
    snapshot.portalOpen = m_portalOpen;
    snapshot.playerPosition = m_playerPosition;
    snapshot.portalPosition = GetPortalPosition();
    snapshot.roomTint = m_rooms[static_cast<std::size_t>(m_currentRoomIndex)].tint;
    snapshot.latestStatus = m_latestStatus;

    snapshot.enemies.reserve(m_enemies.size());
    for (const EnemyRuntime& enemy : m_enemies)
    {
        snapshot.enemies.push_back(RogueLiteEnemyView{enemy.kind, enemy.position, enemy.radius, enemy.hitPoints});
    }

    snapshot.projectiles.reserve(m_projectiles.size());
    for (const ProjectileRuntime& projectile : m_projectiles)
    {
        snapshot.projectiles.push_back(
            RogueLiteProjectileView{projectile.fromPlayer, projectile.position, projectile.velocity, projectile.radius});
    }

    snapshot.pickups.reserve(m_pickups.size());
    for (const PickupRuntime& pickup : m_pickups)
    {
        snapshot.pickups.push_back(RogueLitePickupView{pickup.kind, pickup.position, pickup.radius, pickup.value});
    }

    return snapshot;
}

void RogueLiteGame::DebugClearEnemies()
{
    m_enemies.clear();
}

void RogueLiteGame::DebugForceRoomCleared()
{
    m_enemies.clear();
    m_projectiles.clear();
    m_pendingWaveSpawn = false;
    m_currentWaveIndex = static_cast<int>(m_rooms[static_cast<std::size_t>(m_currentRoomIndex)].waves.size()) - 1;
    m_phase = RogueLiteGamePhase::RoomClear;
    m_portalOpen = true;
    SpawnRoomReward();
    m_latestStatus = "Debug room clear.";
}

void RogueLiteGame::DebugSpawnEnemy(const RogueLiteEnemyKind kind, const Vector2 position)
{
    EnemyRuntime enemy;
    enemy.kind = kind;
    enemy.position = position;
    switch (kind)
    {
    case RogueLiteEnemyKind::Grunt:
        enemy.radius = 0.45F;
        enemy.moveSpeed = 2.2F;
        enemy.hitPoints = 2;
        enemy.contactDamage = 1;
        enemy.rewardScrap = 1;
        break;
    case RogueLiteEnemyKind::Shooter:
        enemy.radius = 0.50F;
        enemy.moveSpeed = 1.6F;
        enemy.hitPoints = 3;
        enemy.contactDamage = 1;
        enemy.rewardScrap = 2;
        enemy.shotCooldown = 0.3;
        break;
    case RogueLiteEnemyKind::Brute:
        enemy.radius = 0.65F;
        enemy.moveSpeed = 1.2F;
        enemy.hitPoints = 5;
        enemy.contactDamage = 2;
        enemy.rewardScrap = 3;
        break;
    }
    m_enemies.push_back(enemy);
}

void RogueLiteGame::DebugDamagePlayer(const int amount)
{
    RogueLiteUpdateResult result;
    static_cast<void>(ApplyDamageToPlayer(amount, result));
}

void RogueLiteGame::DebugSetPlayerPosition(const Vector2 position)
{
    m_playerPosition = ClampInsideRoom(position, kPlayerRadius);
}

std::vector<RogueLiteGame::RoomDefinition> RogueLiteGame::BuildRoomDefinitions()
{
    return {
        RoomDefinition{
            "feature.roguelite.room.01",
            "Entry Gate",
            Color{0.08F, 0.11F, 0.15F, 1.0F},
            {
                WaveDefinition{{SpawnDefinition{RogueLiteEnemyKind::Grunt, Vector2{-5.0F, 2.4F}},
                                SpawnDefinition{RogueLiteEnemyKind::Grunt, Vector2{0.0F, 3.1F}},
                                SpawnDefinition{RogueLiteEnemyKind::Grunt, Vector2{5.0F, 2.2F}}}},
                WaveDefinition{{SpawnDefinition{RogueLiteEnemyKind::Grunt, Vector2{-6.3F, -0.5F}},
                                SpawnDefinition{RogueLiteEnemyKind::Shooter, Vector2{0.0F, 3.9F}},
                                SpawnDefinition{RogueLiteEnemyKind::Grunt, Vector2{6.1F, -0.3F}}}},
            }},
        RoomDefinition{
            "feature.roguelite.room.02",
            "Relay Hall",
            Color{0.07F, 0.12F, 0.10F, 1.0F},
            {
                WaveDefinition{{SpawnDefinition{RogueLiteEnemyKind::Shooter, Vector2{-4.5F, 3.4F}},
                                SpawnDefinition{RogueLiteEnemyKind::Grunt, Vector2{-1.6F, 1.6F}},
                                SpawnDefinition{RogueLiteEnemyKind::Grunt, Vector2{2.0F, 1.4F}},
                                SpawnDefinition{RogueLiteEnemyKind::Shooter, Vector2{4.8F, 3.2F}}}},
                WaveDefinition{{SpawnDefinition{RogueLiteEnemyKind::Brute, Vector2{0.0F, 4.1F}},
                                SpawnDefinition{RogueLiteEnemyKind::Shooter, Vector2{-5.5F, 1.0F}},
                                SpawnDefinition{RogueLiteEnemyKind::Shooter, Vector2{5.5F, 1.0F}}}},
            }},
        RoomDefinition{
            "feature.roguelite.room.03",
            "Core Vault",
            Color{0.13F, 0.09F, 0.14F, 1.0F},
            {
                WaveDefinition{{SpawnDefinition{RogueLiteEnemyKind::Brute, Vector2{-4.8F, 3.0F}},
                                SpawnDefinition{RogueLiteEnemyKind::Grunt, Vector2{0.0F, 2.0F}},
                                SpawnDefinition{RogueLiteEnemyKind::Brute, Vector2{4.8F, 3.0F}}}},
                WaveDefinition{{SpawnDefinition{RogueLiteEnemyKind::Shooter, Vector2{-6.2F, 3.9F}},
                                SpawnDefinition{RogueLiteEnemyKind::Grunt, Vector2{-2.6F, 0.2F}},
                                SpawnDefinition{RogueLiteEnemyKind::Grunt, Vector2{2.6F, 0.2F}},
                                SpawnDefinition{RogueLiteEnemyKind::Shooter, Vector2{6.2F, 3.9F}}}},
                WaveDefinition{{SpawnDefinition{RogueLiteEnemyKind::Brute, Vector2{0.0F, 4.4F}},
                                SpawnDefinition{RogueLiteEnemyKind::Shooter, Vector2{-6.2F, 1.4F}},
                                SpawnDefinition{RogueLiteEnemyKind::Shooter, Vector2{6.2F, 1.4F}},
                                SpawnDefinition{RogueLiteEnemyKind::Grunt, Vector2{-3.2F, -0.8F}},
                                SpawnDefinition{RogueLiteEnemyKind::Grunt, Vector2{3.2F, -0.8F}}}},
            }},
    };
}

void RogueLiteGame::LoadRoom(const int roomIndex)
{
    m_currentRoomIndex = std::clamp(roomIndex, 0, static_cast<int>(m_rooms.size()) - 1);
    m_currentWaveIndex = 0;
    m_enemies.clear();
    m_projectiles.clear();
    m_pickups.clear();
    m_playerPosition = Vector2{0.0F, -4.4F};
    m_portalOpen = false;
    m_rewardSpawned = false;
    m_pendingWaveSpawn = false;
    m_waveSpawnDelay = 0.0;
    m_phase = RogueLiteGamePhase::Combat;
    SpawnCurrentWave();
}

void RogueLiteGame::SpawnCurrentWave()
{
    const RoomDefinition& room = m_rooms[static_cast<std::size_t>(m_currentRoomIndex)];
    const WaveDefinition& wave = room.waves[static_cast<std::size_t>(m_currentWaveIndex)];
    for (const SpawnDefinition& spawn : wave.spawns)
    {
        DebugSpawnEnemy(spawn.kind, spawn.position);
    }

    m_latestStatus = room.roomName + " wave " + std::to_string(m_currentWaveIndex + 1) + " online.";
}

void RogueLiteGame::SpawnRoomReward()
{
    if (m_rewardSpawned)
    {
        return;
    }

    m_rewardSpawned = true;
    m_pickups.push_back(PickupRuntime{RogueLitePickupKind::Core, Vector2{0.0F, 0.0F}, 0.42F, 1});
    if (m_playerHitPoints < m_playerMaxHitPoints)
    {
        m_pickups.push_back(PickupRuntime{RogueLitePickupKind::Heart, Vector2{1.6F, 0.0F}, 0.38F, 1});
    }
}

bool RogueLiteGame::TryAdvanceToNextRoom()
{
    if (!IsPortalReachable())
    {
        return false;
    }

    if (m_currentRoomIndex + 1 < static_cast<int>(m_rooms.size()))
    {
        LoadRoom(m_currentRoomIndex + 1);
        m_latestStatus = "Room " + std::to_string(m_currentRoomIndex + 1) + " breach confirmed.";
        return true;
    }

    m_phase = RogueLiteGamePhase::Victory;
    m_portalOpen = true;
    m_latestStatus = "Core vault secured. Run clear.";
    return true;
}

bool RogueLiteGame::IsPortalReachable() const
{
    if (!m_portalOpen || !m_pickups.empty())
    {
        return false;
    }

    return DistanceSquared(m_playerPosition, GetPortalPosition()) <= (kPlayerRadius + kPortalRadius) * (kPlayerRadius + kPortalRadius);
}

RogueLiteActionResult RogueLiteGame::TryFireProjectile(const Vector2 direction)
{
    RogueLiteActionResult result;
    if (m_phase != RogueLiteGamePhase::Combat || m_playerFireCooldown > 0.0)
    {
        return result;
    }

    const Vector2 normalizedDirection = NormalizeMove(direction);
    if (LengthSquared(normalizedDirection) == 0.0F)
    {
        return result;
    }

    m_lastAimDirection = normalizedDirection;
    m_playerFireCooldown = GetPlayerFireCooldown();
    m_projectiles.push_back(
        ProjectileRuntime{
            true,
            Add(m_playerPosition, Scale(normalizedDirection, kPlayerRadius + 0.28F)),
            Scale(normalizedDirection, GetPlayerProjectileSpeed()),
            0.15F,
            GetPlayerProjectileDamage(),
            1.3});
    m_latestStatus = "Weapon discharge registered.";
    result.stateChanged = true;
    result.projectileFired = true;
    return result;
}

RogueLiteActionResult RogueLiteGame::TryDash()
{
    RogueLiteActionResult result;
    if (m_phase != RogueLiteGamePhase::Combat || m_dashCooldown > 0.0)
    {
        return result;
    }

    Vector2 dashDirection = LengthSquared(m_lastMoveDirection) > 0.0F ? m_lastMoveDirection : m_lastAimDirection;
    dashDirection = NormalizeMove(dashDirection);
    if (LengthSquared(dashDirection) == 0.0F)
    {
        dashDirection = Vector2{0.0F, 1.0F};
    }

    m_playerPosition = ClampInsideRoom(Add(m_playerPosition, Scale(dashDirection, kDashDistance)), kPlayerRadius);
    m_dashCooldown = kDashCooldownSeconds;
    m_invulnerabilitySeconds = 0.18;
    m_latestStatus = "Combat roll executed.";
    result.stateChanged = true;
    result.dashed = true;
    return result;
}

Vector2 RogueLiteGame::NormalizeMove(const Vector2 value) const
{
    const float length = Length(value);
    if (length <= 0.0001F)
    {
        return {};
    }

    return Vector2{value.x / length, value.y / length};
}

bool RogueLiteGame::IsInsideRoom(const Vector2 position, const float radius) const
{
    return position.x >= -kRoomHalfWidth + radius && position.x <= kRoomHalfWidth - radius &&
           position.y >= -kRoomHalfHeight + radius && position.y <= kRoomHalfHeight - radius;
}

Vector2 RogueLiteGame::ClampInsideRoom(const Vector2 position, const float radius) const
{
    return Vector2{
        std::clamp(position.x, -kRoomHalfWidth + radius, kRoomHalfWidth - radius),
        std::clamp(position.y, -kRoomHalfHeight + radius, kRoomHalfHeight - radius)};
}

bool RogueLiteGame::ApplyDamageToPlayer(const int amount, RogueLiteUpdateResult& result)
{
    if (m_invulnerabilitySeconds > 0.0 || m_phase == RogueLiteGamePhase::GameOver || amount <= 0)
    {
        return false;
    }

    m_playerHitPoints = std::max(0, m_playerHitPoints - amount);
    m_invulnerabilitySeconds = kPlayerInvulnerabilitySeconds;
    result.stateChanged = true;
    result.playerDamageTaken += amount;
    m_latestStatus = "Player hit. Reposition or roll through the lane.";
    if (m_playerHitPoints == 0)
    {
        m_phase = RogueLiteGamePhase::GameOver;
        result.runLost = true;
        m_latestStatus = "Run collapsed. Press R to redeploy.";
        return true;
    }

    return true;
}

void RogueLiteGame::RemoveDeadEnemies(RogueLiteUpdateResult& result)
{
    const auto beforeCount = m_enemies.size();
    int scrapGain = 0;
    m_enemies.erase(
        std::remove_if(
            m_enemies.begin(),
            m_enemies.end(),
            [&result, &scrapGain](const EnemyRuntime& enemy)
            {
                if (enemy.hitPoints > 0)
                {
                    return false;
                }

                ++result.defeatedEnemies;
                scrapGain += enemy.rewardScrap;
                return true;
            }),
        m_enemies.end());

    if (beforeCount != m_enemies.size())
    {
        m_scrap += scrapGain;
        result.scrapGained += scrapGain;
        result.stateChanged = true;
        m_latestStatus = "Target down. Keep pressing the room.";
    }
}

void RogueLiteGame::RemoveExpiredProjectiles()
{
    m_projectiles.erase(
        std::remove_if(
            m_projectiles.begin(),
            m_projectiles.end(),
            [this](const ProjectileRuntime& projectile)
            {
                return projectile.lifetimeSeconds <= 0.0 || !IsInsideRoom(projectile.position, projectile.radius);
            }),
        m_projectiles.end());
}

bool RogueLiteGame::ResolvePlayerProjectileHit(ProjectileRuntime& projectile, RogueLiteUpdateResult& result)
{
    for (EnemyRuntime& enemy : m_enemies)
    {
        const float combinedRadius = projectile.radius + enemy.radius;
        if (DistanceSquared(projectile.position, enemy.position) > combinedRadius * combinedRadius)
        {
            continue;
        }

        enemy.hitPoints -= projectile.damage;
        projectile.lifetimeSeconds = 0.0;
        result.stateChanged = true;
        return true;
    }

    return false;
}

bool RogueLiteGame::ResolveEnemyProjectileHit(ProjectileRuntime& projectile, RogueLiteUpdateResult& result)
{
    const float combinedRadius = projectile.radius + kPlayerRadius;
    if (DistanceSquared(projectile.position, m_playerPosition) > combinedRadius * combinedRadius)
    {
        return false;
    }

    projectile.lifetimeSeconds = 0.0;
    return ApplyDamageToPlayer(projectile.damage, result);
}

void RogueLiteGame::ResolvePlayerContactDamage(RogueLiteUpdateResult& result)
{
    for (const EnemyRuntime& enemy : m_enemies)
    {
        const float combinedRadius = enemy.radius + kPlayerRadius;
        if (DistanceSquared(enemy.position, m_playerPosition) <= combinedRadius * combinedRadius)
        {
            static_cast<void>(ApplyDamageToPlayer(enemy.contactDamage, result));
            return;
        }
    }
}

void RogueLiteGame::ResolvePickupCollection(RogueLiteUpdateResult& result)
{
    std::size_t collectedCount = 0;
    m_pickups.erase(
        std::remove_if(
            m_pickups.begin(),
            m_pickups.end(),
            [this, &result, &collectedCount](const PickupRuntime& pickup)
            {
                const float combinedRadius = pickup.radius + kPlayerRadius;
                if (DistanceSquared(pickup.position, m_playerPosition) > combinedRadius * combinedRadius)
                {
                    return false;
                }

                ++collectedCount;
                result.collectedPickups += 1;
                result.stateChanged = true;
                if (pickup.kind == RogueLitePickupKind::Core)
                {
                    ++m_upgradeLevel;
                    m_scrap += pickup.value;
                    result.scrapGained += pickup.value;
                    m_latestStatus = "Core synced. Weapon output improved.";
                }
                else
                {
                    m_playerHitPoints = std::min(m_playerMaxHitPoints, m_playerHitPoints + pickup.value);
                    m_latestStatus = "Field repair charge collected.";
                }
                return true;
            }),
        m_pickups.end());

    static_cast<void>(collectedCount);
}

int RogueLiteGame::GetPlayerProjectileDamage() const
{
    return 1 + m_upgradeLevel;
}

double RogueLiteGame::GetPlayerFireCooldown() const
{
    return std::max(0.11, 0.28 - static_cast<double>(m_upgradeLevel) * 0.025);
}

float RogueLiteGame::GetPlayerProjectileSpeed() const
{
    return 10.5F + static_cast<float>(m_upgradeLevel) * 0.8F;
}

Vector2 RogueLiteGame::GetPortalPosition() const
{
    return Vector2{0.0F, kRoomHalfHeight - 0.65F};
}

float RogueLiteGame::DistanceSquared(const Vector2 a, const Vector2 b) const
{
    return LengthSquared(Subtract(a, b));
}
} // namespace she
