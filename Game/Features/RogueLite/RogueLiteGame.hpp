#pragma once

#include "SHE/Core/MathTypes.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace she
{
enum class RogueLiteGamePhase
{
    Combat,
    RoomClear,
    Victory,
    GameOver
};

enum class RogueLiteEnemyKind
{
    Grunt,
    Shooter,
    Brute
};

enum class RogueLitePickupKind
{
    Core,
    Heart
};

enum class RogueLiteAction
{
    FireUp,
    FireDown,
    FireLeft,
    FireRight,
    Dash,
    Restart
};

struct RogueLiteEnemyView
{
    RogueLiteEnemyKind kind = RogueLiteEnemyKind::Grunt;
    Vector2 position{};
    float radius = 0.5F;
    int hitPoints = 1;
};

struct RogueLiteProjectileView
{
    bool fromPlayer = true;
    Vector2 position{};
    Vector2 velocity{};
    float radius = 0.15F;
};

struct RogueLitePickupView
{
    RogueLitePickupKind kind = RogueLitePickupKind::Core;
    Vector2 position{};
    float radius = 0.4F;
    int value = 1;
};

struct RogueLiteActionResult
{
    bool stateChanged = false;
    bool projectileFired = false;
    bool dashed = false;
    bool restarted = false;
    bool triggeredGameOver = false;
};

struct RogueLiteUpdateResult
{
    bool stateChanged = false;
    bool waveSpawned = false;
    bool roomCleared = false;
    bool roomAdvanced = false;
    bool pickupSpawned = false;
    bool runWon = false;
    bool runLost = false;
    int defeatedEnemies = 0;
    int scrapGained = 0;
    int playerDamageTaken = 0;
    int collectedPickups = 0;
};

struct RogueLiteSnapshot
{
    RogueLiteGamePhase phase = RogueLiteGamePhase::Combat;
    int roomIndex = 0;
    int roomCount = 0;
    int currentWave = 0;
    int totalWaves = 0;
    int playerHitPoints = 0;
    int playerMaxHitPoints = 0;
    int scrap = 0;
    int upgradeLevel = 0;
    int enemiesRemaining = 0;
    int activeProjectileCount = 0;
    bool portalOpen = false;
    Vector2 playerPosition{};
    Vector2 portalPosition{};
    Color roomTint{};
    std::string latestStatus;
    std::vector<RogueLiteEnemyView> enemies;
    std::vector<RogueLiteProjectileView> projectiles;
    std::vector<RogueLitePickupView> pickups;
};

[[nodiscard]] const char* GetRogueLitePhaseName(RogueLiteGamePhase phase);
[[nodiscard]] const char* GetRogueLiteEnemyName(RogueLiteEnemyKind kind);
[[nodiscard]] const char* GetRogueLitePickupName(RogueLitePickupKind kind);

class RogueLiteGame final
{
public:
    RogueLiteGame();

    void Reset(std::uint32_t seed = 0x21U);
    [[nodiscard]] RogueLiteUpdateResult Update(double deltaSeconds, Vector2 moveInput);
    [[nodiscard]] RogueLiteActionResult ApplyAction(RogueLiteAction action);
    [[nodiscard]] RogueLiteSnapshot BuildSnapshot() const;

    void DebugClearEnemies();
    void DebugForceRoomCleared();
    void DebugSpawnEnemy(RogueLiteEnemyKind kind, Vector2 position);
    void DebugDamagePlayer(int amount);
    void DebugSetPlayerPosition(Vector2 position);

private:
    struct SpawnDefinition
    {
        RogueLiteEnemyKind kind = RogueLiteEnemyKind::Grunt;
        Vector2 position{};
    };

    struct WaveDefinition
    {
        std::vector<SpawnDefinition> spawns;
    };

    struct RoomDefinition
    {
        std::string recordId;
        std::string roomName;
        Color tint{};
        std::vector<WaveDefinition> waves;
    };

    struct EnemyRuntime
    {
        RogueLiteEnemyKind kind = RogueLiteEnemyKind::Grunt;
        Vector2 position{};
        Vector2 velocity{};
        float radius = 0.5F;
        float moveSpeed = 2.0F;
        int hitPoints = 1;
        int contactDamage = 1;
        int rewardScrap = 1;
        double shotCooldown = 0.0;
    };

    struct ProjectileRuntime
    {
        bool fromPlayer = true;
        Vector2 position{};
        Vector2 velocity{};
        float radius = 0.15F;
        int damage = 1;
        double lifetimeSeconds = 1.0;
    };

    struct PickupRuntime
    {
        RogueLitePickupKind kind = RogueLitePickupKind::Core;
        Vector2 position{};
        float radius = 0.4F;
        int value = 1;
    };

    [[nodiscard]] static std::vector<RoomDefinition> BuildRoomDefinitions();
    void LoadRoom(int roomIndex);
    void SpawnCurrentWave();
    void SpawnRoomReward();
    [[nodiscard]] bool TryAdvanceToNextRoom();
    [[nodiscard]] bool IsPortalReachable() const;
    [[nodiscard]] RogueLiteActionResult TryFireProjectile(Vector2 direction);
    [[nodiscard]] RogueLiteActionResult TryDash();
    [[nodiscard]] Vector2 NormalizeMove(Vector2 value) const;
    [[nodiscard]] bool IsInsideRoom(Vector2 position, float radius) const;
    [[nodiscard]] Vector2 ClampInsideRoom(Vector2 position, float radius) const;
    [[nodiscard]] bool ApplyDamageToPlayer(int amount, RogueLiteUpdateResult& result);
    void RemoveDeadEnemies(RogueLiteUpdateResult& result);
    void RemoveExpiredProjectiles();
    [[nodiscard]] bool ResolvePlayerProjectileHit(ProjectileRuntime& projectile, RogueLiteUpdateResult& result);
    [[nodiscard]] bool ResolveEnemyProjectileHit(ProjectileRuntime& projectile, RogueLiteUpdateResult& result);
    void ResolvePlayerContactDamage(RogueLiteUpdateResult& result);
    void ResolvePickupCollection(RogueLiteUpdateResult& result);
    [[nodiscard]] int GetPlayerProjectileDamage() const;
    [[nodiscard]] double GetPlayerFireCooldown() const;
    [[nodiscard]] float GetPlayerProjectileSpeed() const;
    [[nodiscard]] Vector2 GetPortalPosition() const;
    [[nodiscard]] float DistanceSquared(Vector2 a, Vector2 b) const;

    std::vector<RoomDefinition> m_rooms;
    std::vector<EnemyRuntime> m_enemies;
    std::vector<ProjectileRuntime> m_projectiles;
    std::vector<PickupRuntime> m_pickups;
    RogueLiteGamePhase m_phase = RogueLiteGamePhase::Combat;
    int m_currentRoomIndex = 0;
    int m_currentWaveIndex = 0;
    int m_playerHitPoints = 5;
    int m_playerMaxHitPoints = 5;
    int m_scrap = 0;
    int m_upgradeLevel = 0;
    Vector2 m_playerPosition{};
    Vector2 m_lastAimDirection{0.0F, 1.0F};
    Vector2 m_lastMoveDirection{0.0F, 1.0F};
    double m_playerFireCooldown = 0.0;
    double m_dashCooldown = 0.0;
    double m_invulnerabilitySeconds = 0.0;
    double m_waveSpawnDelay = 0.0;
    bool m_pendingWaveSpawn = false;
    bool m_portalOpen = false;
    bool m_rewardSpawned = false;
    std::uint32_t m_seed = 0x21U;
    std::string m_latestStatus = "Drop into the room and clear the wave.";
};
} // namespace she
