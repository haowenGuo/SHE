#include "RogueLiteFeatureLayer.hpp"

#include "SHE/Core/Logger.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>

namespace she
{
namespace
{
constexpr std::string_view kSceneName = "RogueLiteRooms";
constexpr std::string_view kFireUpCommand = "RogueLiteFireUp";
constexpr std::string_view kFireDownCommand = "RogueLiteFireDown";
constexpr std::string_view kFireLeftCommand = "RogueLiteFireLeft";
constexpr std::string_view kFireRightCommand = "RogueLiteFireRight";
constexpr std::string_view kDashCommand = "RogueLiteDash";
constexpr std::string_view kRestartCommand = "RestartRogueLiteRun";

constexpr Vector2 kCameraCenter{0.0F, 0.0F};
constexpr float kCameraZoom = 0.75F;
constexpr float kRoomHalfWidth = 10.5F;
constexpr float kRoomHalfHeight = 6.5F;

[[nodiscard]] bool IsPressed(const RuntimeServices& services, const KeyCode primary, const KeyCode secondary)
{
    return services.window->GetKeyState(primary).pressed || services.window->GetKeyState(secondary).pressed;
}

[[nodiscard]] bool IsDown(const RuntimeServices& services, const KeyCode primary, const KeyCode secondary)
{
    return services.window->GetKeyState(primary).isDown || services.window->GetKeyState(secondary).isDown;
}

[[nodiscard]] Vector2 BuildMoveInput(const RuntimeServices& services)
{
    float x = 0.0F;
    float y = 0.0F;
    if (services.window->GetKeyState(KeyCode::A).isDown)
    {
        x -= 1.0F;
    }
    if (services.window->GetKeyState(KeyCode::D).isDown)
    {
        x += 1.0F;
    }
    if (services.window->GetKeyState(KeyCode::W).isDown)
    {
        y += 1.0F;
    }
    if (services.window->GetKeyState(KeyCode::S).isDown)
    {
        y -= 1.0F;
    }

    return Vector2{x, y};
}

[[nodiscard]] std::string_view GetEnemyMaterialName(const RogueLiteEnemyKind kind)
{
    switch (kind)
    {
    case RogueLiteEnemyKind::Grunt:
        return "materials/roguelite.enemy_grunt";
    case RogueLiteEnemyKind::Shooter:
        return "materials/roguelite.enemy_shooter";
    case RogueLiteEnemyKind::Brute:
        return "materials/roguelite.enemy_brute";
    default:
        return "materials/roguelite.enemy_grunt";
    }
}

[[nodiscard]] Color GetEnemyTint(const RogueLiteEnemyKind kind)
{
    switch (kind)
    {
    case RogueLiteEnemyKind::Grunt:
        return Color{0.93F, 0.44F, 0.40F, 1.0F};
    case RogueLiteEnemyKind::Shooter:
        return Color{0.95F, 0.72F, 0.33F, 1.0F};
    case RogueLiteEnemyKind::Brute:
        return Color{0.70F, 0.32F, 0.95F, 1.0F};
    default:
        return Color{1.0F, 1.0F, 1.0F, 1.0F};
    }
}

[[nodiscard]] std::string_view GetPickupMaterialName(const RogueLitePickupKind kind)
{
    switch (kind)
    {
    case RogueLitePickupKind::Core:
        return "materials/roguelite.core";
    case RogueLitePickupKind::Heart:
        return "materials/roguelite.heart";
    default:
        return "materials/roguelite.core";
    }
}

[[nodiscard]] Color BuildPhaseTint(const RogueLiteGamePhase phase)
{
    switch (phase)
    {
    case RogueLiteGamePhase::Combat:
        return Color{0.18F, 0.48F, 0.92F, 1.0F};
    case RogueLiteGamePhase::RoomClear:
        return Color{0.20F, 0.82F, 0.54F, 1.0F};
    case RogueLiteGamePhase::Victory:
        return Color{0.96F, 0.82F, 0.34F, 1.0F};
    case RogueLiteGamePhase::GameOver:
        return Color{0.88F, 0.20F, 0.24F, 1.0F};
    default:
        return Color{0.80F, 0.80F, 0.80F, 1.0F};
    }
}
} // namespace

RogueLiteFeatureLayer::RogueLiteFeatureLayer() : Layer("RogueLiteFeatureLayer")
{
}

void RogueLiteFeatureLayer::OnAttach(RuntimeServices& services)
{
    m_services = &services;
    m_loggedControls = false;
    m_latestDebugReport.clear();
    m_latestAuthoringContext.clear();
    m_latestGameplayDigest.clear();
    m_lastCompletedFrame.reset();

    services.ai->SetAuthoringIntent(
        "Build a room-based top-down action roguelite in Game/Features/RogueLite using gameplay events, data schemas, "
        "renderable 2D rooms, and AI-readable diagnostics.");

    RegisterFeatureContracts(services);
    RegisterDataContracts(services);
    RegisterVisualAssets(services);
    RegisterGameplayContracts(services);
    SubscribeToGameplay(services);

    services.scene->SetActiveScene(std::string(kSceneName));
    m_cameraEntityId = services.scene->CreateEntity("RogueLiteCamera");
    services.scene->TrySetEntityTransform(m_cameraEntityId, SceneTransform{kCameraCenter, 0.0F, Vector2{1.0F, 1.0F}});

    m_game.Reset();
    const RogueLiteSnapshot snapshot = m_game.BuildSnapshot();
    services.gameplay->QueueEvent(
        "roguelite",
        "RunStarted",
        "room=" + std::to_string(snapshot.roomIndex + 1) + "; room_count=" + std::to_string(snapshot.roomCount));
}

void RogueLiteFeatureLayer::OnUpdate(const TickContext& context)
{
    if (!m_loggedControls)
    {
        m_loggedControls = true;
        SHE_LOG_INFO(
            "Game",
            "RogueLite controls: WASD move, arrow keys fire, Shift dash, R restart, Esc quit. Walk into the top portal after clearing a room.");
    }

    if (context.services.window->GetKeyState(KeyCode::Escape).pressed)
    {
        context.services.window->RequestClose();
        return;
    }

    QueueInputCommands(context);

    const RogueLiteSnapshot before = m_game.BuildSnapshot();
    const RogueLiteUpdateResult result = m_game.Update(context.deltaSeconds, BuildMoveInput(context.services));
    const RogueLiteSnapshot after = m_game.BuildSnapshot();
    PublishUpdateEvents(result, before, after);
}

void RogueLiteFeatureLayer::OnRender(const TickContext& context)
{
    const RogueLiteSnapshot snapshot = m_game.BuildSnapshot();
    const WindowState windowState = context.services.window->GetWindowState();
    context.services.renderer->SubmitCamera(
        Camera2DSubmission{
            m_cameraEntityId,
            "RogueLiteCamera",
            kCameraCenter,
            kCameraZoom,
            windowState.width,
            windowState.height,
            snapshot.roomTint});

    RenderRoom(context, snapshot);
    RenderHud(context, snapshot);
}

void RogueLiteFeatureLayer::OnUi(const TickContext& context)
{
    m_latestDebugReport = context.services.ui->BuildLatestDebugReport();
    m_latestAuthoringContext = context.services.ai->ExportAuthoringContext();
    m_latestGameplayDigest = context.services.gameplay->BuildGameplayDigest();
    const auto lastFrame = context.services.renderer->GetLastCompletedFrame();
    if (lastFrame.has_value())
    {
        m_lastCompletedFrame = lastFrame;
    }
}

void RogueLiteFeatureLayer::OnDetach(RuntimeServices& services)
{
    if (m_actionSubscriptionId != 0)
    {
        services.gameplay->UnsubscribeFromEvent(m_actionSubscriptionId);
        m_actionSubscriptionId = 0;
    }

    m_latestDebugReport = services.ui->BuildLatestDebugReport();
    m_latestAuthoringContext = services.ai->ExportAuthoringContext();
    m_latestGameplayDigest = services.gameplay->BuildGameplayDigest();
    m_lastCompletedFrame = services.renderer->GetLastCompletedFrame();
    m_services = nullptr;
}

RogueLiteSnapshot RogueLiteFeatureLayer::BuildSnapshot() const
{
    return m_game.BuildSnapshot();
}

const std::string& RogueLiteFeatureLayer::GetLatestDebugReport() const
{
    return m_latestDebugReport;
}

const std::string& RogueLiteFeatureLayer::GetLatestAuthoringContext() const
{
    return m_latestAuthoringContext;
}

const std::string& RogueLiteFeatureLayer::GetLatestGameplayDigest() const
{
    return m_latestGameplayDigest;
}

const std::optional<RenderFrameSnapshot>& RogueLiteFeatureLayer::GetLastCompletedFrame() const
{
    return m_lastCompletedFrame;
}

void RogueLiteFeatureLayer::RegisterFeatureContracts(RuntimeServices& services)
{
    services.reflection->RegisterType("feature_component", "RogueLiteEnemy", "Top-down roguelite hostile runtime entry.");
    services.reflection->RegisterType("feature_component", "RogueLiteProjectile", "Top-down roguelite projectile runtime entry.");
    services.reflection->RegisterType("feature_component", "RogueLitePickup", "Top-down roguelite reward pickup entry.");
    services.reflection->RegisterFeature(
        "RogueLiteFeature",
        "Game/Features/RogueLite",
        "Room-based action roguelite slice that proves the current complexity ceiling of the AI-native 2D runtime.");
}

void RogueLiteFeatureLayer::RegisterDataContracts(RuntimeServices& services)
{
    static_cast<void>(services.data->RegisterSchema(
        DataSchemaContract{
            "feature.roguelite.room",
            "Room authoring surface for the roguelite slice.",
            "Game/Features/RogueLite",
            {
                DataSchemaFieldContract{"id", "scalar", true, "Stable room identifier."},
                DataSchemaFieldContract{"theme", "scalar", true, "Palette/theme label for the room."},
                DataSchemaFieldContract{"waves", "list", true, "Ordered wave identifiers."},
                DataSchemaFieldContract{"reward", "scalar", true, "Primary reward type unlocked after clear."},
            }}));
    static_cast<void>(services.data->RegisterSchema(
        DataSchemaContract{
            "feature.roguelite.enemy",
            "Enemy archetype contract for the roguelite slice.",
            "Game/Features/RogueLite",
            {
                DataSchemaFieldContract{"id", "scalar", true, "Stable enemy identifier."},
                DataSchemaFieldContract{"kind", "scalar", true, "Enemy runtime family."},
                DataSchemaFieldContract{"hit_points", "scalar", true, "Base hit points."},
                DataSchemaFieldContract{"move_speed", "scalar", true, "Base move speed."},
                DataSchemaFieldContract{"reward_scrap", "scalar", true, "Scrap reward on defeat."},
            }}));
    static_cast<void>(services.data->RegisterSchema(
        DataSchemaContract{
            "feature.roguelite.weapon",
            "Weapon baseline contract for the roguelite slice.",
            "Game/Features/RogueLite",
            {
                DataSchemaFieldContract{"id", "scalar", true, "Stable weapon identifier."},
                DataSchemaFieldContract{"damage", "scalar", true, "Base projectile damage."},
                DataSchemaFieldContract{"fire_cooldown", "scalar", true, "Base fire cooldown."},
                DataSchemaFieldContract{"projectile_speed", "scalar", true, "Base projectile speed."},
            }}));
    static_cast<void>(services.data->RegisterSchema(
        DataSchemaContract{
            "feature.roguelite.pickup",
            "Pickup contract for room rewards in the roguelite slice.",
            "Game/Features/RogueLite",
            {
                DataSchemaFieldContract{"id", "scalar", true, "Stable pickup identifier."},
                DataSchemaFieldContract{"kind", "scalar", true, "Pickup family."},
                DataSchemaFieldContract{"value", "scalar", true, "Pickup effect value."},
            }}));

    static_cast<void>(services.data->LoadRecord(
        DataLoadRequest{
            "feature.roguelite.room",
            "feature.roguelite.room.01",
            "Game/Features/RogueLite/Data/room_01.yml",
            "id: feature.roguelite.room.01\n"
            "theme: entry_gate\n"
            "waves:\n"
            "  - opening_line\n"
            "  - crossfire_check\n"
            "reward: upgrade_core\n"}));
    static_cast<void>(services.data->LoadRecord(
        DataLoadRequest{
            "feature.roguelite.room",
            "feature.roguelite.room.02",
            "Game/Features/RogueLite/Data/room_02.yml",
            "id: feature.roguelite.room.02\n"
            "theme: relay_hall\n"
            "waves:\n"
            "  - twin_shooters\n"
            "  - brute_anchor\n"
            "reward: upgrade_core\n"}));
    static_cast<void>(services.data->LoadRecord(
        DataLoadRequest{
            "feature.roguelite.room",
            "feature.roguelite.room.03",
            "Game/Features/RogueLite/Data/room_03.yml",
            "id: feature.roguelite.room.03\n"
            "theme: core_vault\n"
            "waves:\n"
            "  - double_brute\n"
            "  - suppressing_crossfire\n"
            "  - vault_break\n"
            "reward: vault_clear\n"}));

    static_cast<void>(services.data->LoadRecord(
        DataLoadRequest{
            "feature.roguelite.enemy",
            "feature.roguelite.enemy.grunt",
            "Game/Features/RogueLite/Data/enemy_grunt.yml",
            "id: feature.roguelite.enemy.grunt\n"
            "kind: grunt\n"
            "hit_points: 2\n"
            "move_speed: 2.2\n"
            "reward_scrap: 1\n"}));
    static_cast<void>(services.data->LoadRecord(
        DataLoadRequest{
            "feature.roguelite.enemy",
            "feature.roguelite.enemy.shooter",
            "Game/Features/RogueLite/Data/enemy_shooter.yml",
            "id: feature.roguelite.enemy.shooter\n"
            "kind: shooter\n"
            "hit_points: 3\n"
            "move_speed: 1.6\n"
            "reward_scrap: 2\n"}));
    static_cast<void>(services.data->LoadRecord(
        DataLoadRequest{
            "feature.roguelite.enemy",
            "feature.roguelite.enemy.brute",
            "Game/Features/RogueLite/Data/enemy_brute.yml",
            "id: feature.roguelite.enemy.brute\n"
            "kind: brute\n"
            "hit_points: 5\n"
            "move_speed: 1.2\n"
            "reward_scrap: 3\n"}));

    static_cast<void>(services.data->LoadRecord(
        DataLoadRequest{
            "feature.roguelite.weapon",
            "feature.roguelite.weapon.sidearm",
            "Game/Features/RogueLite/Data/weapon_sidearm.yml",
            "id: feature.roguelite.weapon.sidearm\n"
            "damage: 1\n"
            "fire_cooldown: 0.28\n"
            "projectile_speed: 10.5\n"}));

    static_cast<void>(services.data->LoadRecord(
        DataLoadRequest{
            "feature.roguelite.pickup",
            "feature.roguelite.pickup.core",
            "Game/Features/RogueLite/Data/pickup_core.yml",
            "id: feature.roguelite.pickup.core\n"
            "kind: core\n"
            "value: 1\n"}));
    static_cast<void>(services.data->LoadRecord(
        DataLoadRequest{
            "feature.roguelite.pickup",
            "feature.roguelite.pickup.heart",
            "Game/Features/RogueLite/Data/pickup_heart.yml",
            "id: feature.roguelite.pickup.heart\n"
            "kind: heart\n"
            "value: 1\n"}));
}

void RogueLiteFeatureLayer::RegisterVisualAssets(RuntimeServices& services)
{
    if (!services.assets->HasLoader("texture_placeholder"))
    {
        services.assets->RegisterLoader(
            AssetLoaderDescriptor{
                "texture_placeholder",
                "texture",
                {".placeholder", ".png"},
                "Placeholder texture loader for feature-owned 2D materials."});
    }

    const auto registerTexture = [&services](std::string logicalName, std::string sourcePath, std::string recordId)
    {
        services.assets->RegisterAsset(
            AssetMetadata{
                kInvalidAssetId,
                std::move(logicalName),
                std::move(sourcePath),
                "texture",
                "",
                "Game/Features/RogueLite",
                std::move(recordId),
                {"roguelite", "texture"},
                {
                    {"usage", "runtime_render"},
                }});
    };

    registerTexture("textures/roguelite/room_floor", "Game/Features/RogueLite/Data/room_floor.placeholder", "feature.roguelite.texture.room_floor");
    registerTexture("textures/roguelite/wall", "Game/Features/RogueLite/Data/wall.placeholder", "feature.roguelite.texture.wall");
    registerTexture("textures/roguelite/player", "Game/Features/RogueLite/Data/player.placeholder", "feature.roguelite.texture.player");
    registerTexture("textures/roguelite/enemy_grunt", "Game/Features/RogueLite/Data/enemy_grunt.placeholder", "feature.roguelite.texture.enemy_grunt");
    registerTexture("textures/roguelite/enemy_shooter", "Game/Features/RogueLite/Data/enemy_shooter.placeholder", "feature.roguelite.texture.enemy_shooter");
    registerTexture("textures/roguelite/enemy_brute", "Game/Features/RogueLite/Data/enemy_brute.placeholder", "feature.roguelite.texture.enemy_brute");
    registerTexture("textures/roguelite/projectile_player", "Game/Features/RogueLite/Data/projectile_player.placeholder", "feature.roguelite.texture.projectile_player");
    registerTexture("textures/roguelite/projectile_enemy", "Game/Features/RogueLite/Data/projectile_enemy.placeholder", "feature.roguelite.texture.projectile_enemy");
    registerTexture("textures/roguelite/portal", "Game/Features/RogueLite/Data/portal.placeholder", "feature.roguelite.texture.portal");
    registerTexture("textures/roguelite/core", "Game/Features/RogueLite/Data/core.placeholder", "feature.roguelite.texture.core");
    registerTexture("textures/roguelite/heart", "Game/Features/RogueLite/Data/heart.placeholder", "feature.roguelite.texture.heart");
    registerTexture("textures/roguelite/hud", "Game/Features/RogueLite/Data/hud.placeholder", "feature.roguelite.texture.hud");

    services.renderer->RegisterMaterial(Material2DDescriptor{"materials/roguelite.room_floor", "textures/roguelite/room_floor", Color{0.18F, 0.20F, 0.24F, 1.0F}});
    services.renderer->RegisterMaterial(Material2DDescriptor{"materials/roguelite.wall", "textures/roguelite/wall", Color{0.62F, 0.68F, 0.76F, 1.0F}});
    services.renderer->RegisterMaterial(Material2DDescriptor{"materials/roguelite.player", "textures/roguelite/player", Color{0.36F, 0.82F, 0.97F, 1.0F}});
    services.renderer->RegisterMaterial(Material2DDescriptor{"materials/roguelite.enemy_grunt", "textures/roguelite/enemy_grunt", Color{0.93F, 0.44F, 0.40F, 1.0F}});
    services.renderer->RegisterMaterial(Material2DDescriptor{"materials/roguelite.enemy_shooter", "textures/roguelite/enemy_shooter", Color{0.95F, 0.72F, 0.33F, 1.0F}});
    services.renderer->RegisterMaterial(Material2DDescriptor{"materials/roguelite.enemy_brute", "textures/roguelite/enemy_brute", Color{0.70F, 0.32F, 0.95F, 1.0F}});
    services.renderer->RegisterMaterial(Material2DDescriptor{"materials/roguelite.projectile_player", "textures/roguelite/projectile_player", Color{0.88F, 0.96F, 1.0F, 1.0F}});
    services.renderer->RegisterMaterial(Material2DDescriptor{"materials/roguelite.projectile_enemy", "textures/roguelite/projectile_enemy", Color{1.0F, 0.48F, 0.30F, 1.0F}});
    services.renderer->RegisterMaterial(Material2DDescriptor{"materials/roguelite.portal", "textures/roguelite/portal", Color{0.25F, 0.95F, 0.62F, 1.0F}});
    services.renderer->RegisterMaterial(Material2DDescriptor{"materials/roguelite.core", "textures/roguelite/core", Color{0.90F, 0.95F, 1.0F, 1.0F}});
    services.renderer->RegisterMaterial(Material2DDescriptor{"materials/roguelite.heart", "textures/roguelite/heart", Color{0.98F, 0.42F, 0.54F, 1.0F}});
    services.renderer->RegisterMaterial(Material2DDescriptor{"materials/roguelite.hud", "textures/roguelite/hud", Color{0.86F, 0.90F, 0.96F, 1.0F}});
}

void RogueLiteFeatureLayer::RegisterGameplayContracts(RuntimeServices& services)
{
    services.gameplay->RegisterCommand(
        {std::string(kFireUpCommand), "Fire the sidearm upward.", "roguelite", "FireUpRequested"});
    services.gameplay->RegisterCommand(
        {std::string(kFireDownCommand), "Fire the sidearm downward.", "roguelite", "FireDownRequested"});
    services.gameplay->RegisterCommand(
        {std::string(kFireLeftCommand), "Fire the sidearm left.", "roguelite", "FireLeftRequested"});
    services.gameplay->RegisterCommand(
        {std::string(kFireRightCommand), "Fire the sidearm right.", "roguelite", "FireRightRequested"});
    services.gameplay->RegisterCommand(
        {std::string(kDashCommand), "Perform a combat roll in the current move direction.", "roguelite", "DashRequested"});
    services.gameplay->RegisterCommand(
        {std::string(kRestartCommand), "Restart the current roguelite run.", "roguelite", "RestartRequested"});
}

void RogueLiteFeatureLayer::SubscribeToGameplay(RuntimeServices& services)
{
    m_actionSubscriptionId = services.gameplay->SubscribeToEvent(
        "roguelite",
        "*",
        [this](const GameplayEvent& event)
        {
            HandleGameplayAction(event);
        });
}

void RogueLiteFeatureLayer::QueueGameplayCommand(std::string_view name, std::string payload)
{
    if (m_services == nullptr)
    {
        return;
    }

    m_services->gameplay->QueueCommand(std::string(name), std::move(payload));
}

void RogueLiteFeatureLayer::QueueInputCommands(const TickContext& context)
{
    if (IsDown(context.services, KeyCode::Up, KeyCode::Unknown))
    {
        QueueGameplayCommand(kFireUpCommand, "source=input");
    }
    if (IsDown(context.services, KeyCode::Down, KeyCode::Unknown))
    {
        QueueGameplayCommand(kFireDownCommand, "source=input");
    }
    if (IsDown(context.services, KeyCode::Left, KeyCode::Unknown))
    {
        QueueGameplayCommand(kFireLeftCommand, "source=input");
    }
    if (IsDown(context.services, KeyCode::Right, KeyCode::Unknown))
    {
        QueueGameplayCommand(kFireRightCommand, "source=input");
    }
    if (IsPressed(context.services, KeyCode::ShiftLeft, KeyCode::ShiftRight))
    {
        QueueGameplayCommand(kDashCommand, "source=input");
    }
    if (context.services.window->GetKeyState(KeyCode::R).pressed)
    {
        QueueGameplayCommand(kRestartCommand, "source=input");
    }
}

void RogueLiteFeatureLayer::HandleGameplayAction(const GameplayEvent& event)
{
    RogueLiteAction action{};
    bool recognized = true;
    if (event.name == "FireUpRequested")
    {
        action = RogueLiteAction::FireUp;
    }
    else if (event.name == "FireDownRequested")
    {
        action = RogueLiteAction::FireDown;
    }
    else if (event.name == "FireLeftRequested")
    {
        action = RogueLiteAction::FireLeft;
    }
    else if (event.name == "FireRightRequested")
    {
        action = RogueLiteAction::FireRight;
    }
    else if (event.name == "DashRequested")
    {
        action = RogueLiteAction::Dash;
    }
    else if (event.name == "RestartRequested")
    {
        action = RogueLiteAction::Restart;
    }
    else
    {
        recognized = false;
    }

    if (!recognized)
    {
        return;
    }

    const RogueLiteActionResult result = m_game.ApplyAction(action);
    if (!result.stateChanged)
    {
        return;
    }

    PublishActionEvents(action, result, m_game.BuildSnapshot());
}

void RogueLiteFeatureLayer::PublishActionEvents(
    const RogueLiteAction action,
    const RogueLiteActionResult& result,
    const RogueLiteSnapshot& snapshot)
{
    if (m_services == nullptr)
    {
        return;
    }

    if (result.projectileFired)
    {
        m_services->gameplay->QueueEvent(
            "roguelite",
            "PlayerShot",
            "room=" + std::to_string(snapshot.roomIndex + 1) +
                "; active_projectiles=" + std::to_string(snapshot.activeProjectileCount));
    }

    if (result.dashed)
    {
        m_services->gameplay->QueueEvent(
            "roguelite",
            "PlayerDashed",
            "player_x=" + std::to_string(snapshot.playerPosition.x) +
                "; player_y=" + std::to_string(snapshot.playerPosition.y));
    }

    if (result.restarted || action == RogueLiteAction::Restart)
    {
        m_services->gameplay->QueueEvent(
            "roguelite",
            "RunRestarted",
            "room=" + std::to_string(snapshot.roomIndex + 1) + "; hp=" + std::to_string(snapshot.playerHitPoints) +
                "; scrap=" + std::to_string(snapshot.scrap));
    }

    if (result.triggeredGameOver || snapshot.phase == RogueLiteGamePhase::GameOver)
    {
        m_services->gameplay->QueueEvent("roguelite", "RunLost", "reason=player_destroyed");
    }
}

void RogueLiteFeatureLayer::PublishUpdateEvents(
    const RogueLiteUpdateResult& result,
    const RogueLiteSnapshot& before,
    const RogueLiteSnapshot& after)
{
    if (m_services == nullptr)
    {
        return;
    }

    if (result.waveSpawned)
    {
        m_services->gameplay->QueueEvent(
            "roguelite",
            "WaveSpawned",
            "room=" + std::to_string(after.roomIndex + 1) + "; wave=" + std::to_string(after.currentWave) +
                "; total_waves=" + std::to_string(after.totalWaves));
    }

    if (result.defeatedEnemies > 0)
    {
        m_services->gameplay->QueueEvent(
            "roguelite",
            "EnemyDefeated",
            "count=" + std::to_string(result.defeatedEnemies) + "; scrap_gained=" + std::to_string(result.scrapGained));
    }

    if (result.playerDamageTaken > 0)
    {
        m_services->gameplay->QueueEvent(
            "roguelite",
            "PlayerDamaged",
            "damage=" + std::to_string(result.playerDamageTaken) +
                "; hp=" + std::to_string(after.playerHitPoints));
    }

    if (result.collectedPickups > 0)
    {
        m_services->gameplay->QueueEvent(
            "roguelite",
            "PickupCollected",
            "count=" + std::to_string(result.collectedPickups) + "; upgrade_level=" +
                std::to_string(after.upgradeLevel) + "; scrap=" + std::to_string(after.scrap));
    }

    if (result.roomCleared)
    {
        m_services->gameplay->QueueEvent(
            "roguelite",
            "RoomCleared",
            "room=" + std::to_string(after.roomIndex + 1) + "; portal_open=true");
    }

    if (result.roomAdvanced)
    {
        if (result.runWon)
        {
            m_services->gameplay->QueueEvent(
                "roguelite",
                "RunWon",
                "rooms=" + std::to_string(after.roomCount) + "; scrap=" + std::to_string(after.scrap));
        }
        else
        {
            m_services->gameplay->QueueEvent(
                "roguelite",
                "RoomEntered",
                "room=" + std::to_string(after.roomIndex + 1) + "; previous_room=" +
                    std::to_string(before.roomIndex + 1));
        }
    }

    if (result.runLost || (before.phase != RogueLiteGamePhase::GameOver && after.phase == RogueLiteGamePhase::GameOver))
    {
        m_services->gameplay->QueueEvent(
            "roguelite",
            "RunLost",
            "room=" + std::to_string(after.roomIndex + 1) + "; scrap=" + std::to_string(after.scrap));
    }
}

void RogueLiteFeatureLayer::SubmitSprite(
    const TickContext& context,
    std::string_view entityName,
    std::string_view materialName,
    const Vector2 position,
    const Vector2 size,
    const Color& tint,
    const int sortKey) const
{
    context.services.renderer->SubmitSprite(
        Sprite2DSubmission{
            kInvalidEntityId,
            std::string(entityName),
            std::string(materialName),
            position,
            0.0F,
            size,
            tint,
            sortKey});
}

void RogueLiteFeatureLayer::SubmitHudBar(
    const TickContext& context,
    const Vector2 center,
    const Vector2 size,
    const float ratio,
    const Color& fillTint,
    const Color& backTint,
    const int sortKey) const
{
    const float clampedRatio = std::clamp(ratio, 0.0F, 1.0F);
    SubmitSprite(context, "HudBarBack", "materials/roguelite.hud", center, size, backTint, sortKey);

    if (clampedRatio <= 0.0F)
    {
        return;
    }

    const float fillWidth = size.x * clampedRatio;
    SubmitSprite(
        context,
        "HudBarFill",
        "materials/roguelite.hud",
        Vector2{center.x - (size.x - fillWidth) * 0.5F, center.y},
        Vector2{fillWidth, size.y},
        fillTint,
        sortKey + 1);
}

void RogueLiteFeatureLayer::RenderRoom(const TickContext& context, const RogueLiteSnapshot& snapshot) const
{
    SubmitSprite(
        context,
        "RoomFloor",
        "materials/roguelite.room_floor",
        Vector2{0.0F, 0.0F},
        Vector2{kRoomHalfWidth * 2.0F, kRoomHalfHeight * 2.0F},
        snapshot.roomTint,
        -100);

    SubmitSprite(
        context,
        "WallNorth",
        "materials/roguelite.wall",
        Vector2{0.0F, kRoomHalfHeight + 0.25F},
        Vector2{kRoomHalfWidth * 2.2F, 0.55F},
        Color{0.70F, 0.76F, 0.84F, 1.0F},
        -90);
    SubmitSprite(
        context,
        "WallSouth",
        "materials/roguelite.wall",
        Vector2{0.0F, -kRoomHalfHeight - 0.25F},
        Vector2{kRoomHalfWidth * 2.2F, 0.55F},
        Color{0.55F, 0.60F, 0.70F, 1.0F},
        -90);
    SubmitSprite(
        context,
        "WallWest",
        "materials/roguelite.wall",
        Vector2{-kRoomHalfWidth - 0.25F, 0.0F},
        Vector2{0.55F, kRoomHalfHeight * 2.2F},
        Color{0.58F, 0.66F, 0.74F, 1.0F},
        -90);
    SubmitSprite(
        context,
        "WallEast",
        "materials/roguelite.wall",
        Vector2{kRoomHalfWidth + 0.25F, 0.0F},
        Vector2{0.55F, kRoomHalfHeight * 2.2F},
        Color{0.58F, 0.66F, 0.74F, 1.0F},
        -90);

    SubmitSprite(
        context,
        "PortalLane",
        "materials/roguelite.room_floor",
        Vector2{0.0F, kRoomHalfHeight - 0.9F},
        Vector2{3.8F, 1.2F},
        Color{0.10F, 0.16F, 0.18F, 1.0F},
        -80);

    if (snapshot.portalOpen)
    {
        SubmitSprite(
            context,
            "Portal",
            "materials/roguelite.portal",
            snapshot.portalPosition,
            Vector2{1.65F, 0.95F},
            BuildPhaseTint(snapshot.phase),
            5);
    }

    for (std::size_t index = 0; index < snapshot.pickups.size(); ++index)
    {
        const RogueLitePickupView& pickup = snapshot.pickups[index];
        SubmitSprite(
            context,
            "Pickup" + std::to_string(index),
            GetPickupMaterialName(pickup.kind),
            pickup.position,
            Vector2{pickup.radius * 2.2F, pickup.radius * 2.2F},
            pickup.kind == RogueLitePickupKind::Core ? Color{0.92F, 0.98F, 1.0F, 1.0F}
                                                     : Color{1.0F, 0.56F, 0.66F, 1.0F},
            12);
    }

    for (std::size_t index = 0; index < snapshot.enemies.size(); ++index)
    {
        const RogueLiteEnemyView& enemy = snapshot.enemies[index];
        SubmitSprite(
            context,
            "Enemy" + std::to_string(index),
            GetEnemyMaterialName(enemy.kind),
            enemy.position,
            Vector2{enemy.radius * 2.3F, enemy.radius * 2.3F},
            GetEnemyTint(enemy.kind),
            20 + static_cast<int>(index));
    }

    for (std::size_t index = 0; index < snapshot.projectiles.size(); ++index)
    {
        const RogueLiteProjectileView& projectile = snapshot.projectiles[index];
        SubmitSprite(
            context,
            projectile.fromPlayer ? "PlayerProjectile" + std::to_string(index) : "EnemyProjectile" + std::to_string(index),
            projectile.fromPlayer ? "materials/roguelite.projectile_player" : "materials/roguelite.projectile_enemy",
            projectile.position,
            Vector2{projectile.radius * 3.4F, projectile.radius * 3.4F},
            projectile.fromPlayer ? Color{0.94F, 0.98F, 1.0F, 1.0F} : Color{1.0F, 0.62F, 0.36F, 1.0F},
            30 + static_cast<int>(index));
    }

    SubmitSprite(
        context,
        "Player",
        "materials/roguelite.player",
        snapshot.playerPosition,
        Vector2{1.0F, 1.0F},
        Color{0.44F, 0.88F, 1.0F, 1.0F},
        50);
}

void RogueLiteFeatureLayer::RenderHud(const TickContext& context, const RogueLiteSnapshot& snapshot) const
{
    SubmitSprite(
        context,
        "HudTopBand",
        "materials/roguelite.hud",
        Vector2{0.0F, 7.75F},
        Vector2{22.5F, 1.6F},
        Color{0.05F, 0.07F, 0.10F, 0.92F},
        200);
    SubmitSprite(
        context,
        "HudBottomBand",
        "materials/roguelite.hud",
        Vector2{0.0F, -7.75F},
        Vector2{22.5F, 1.3F},
        Color{0.05F, 0.07F, 0.10F, 0.88F},
        200);

    const Color phaseTint = BuildPhaseTint(snapshot.phase);
    SubmitHudBar(
        context,
        Vector2{-6.0F, 7.75F},
        Vector2{4.8F, 0.32F},
        snapshot.playerMaxHitPoints > 0
            ? static_cast<float>(snapshot.playerHitPoints) / static_cast<float>(snapshot.playerMaxHitPoints)
            : 0.0F,
        Color{0.98F, 0.34F, 0.44F, 1.0F},
        Color{0.22F, 0.10F, 0.14F, 1.0F},
        205);
    SubmitHudBar(
        context,
        Vector2{0.0F, 7.75F},
        Vector2{4.8F, 0.32F},
        snapshot.totalWaves > 0 ? static_cast<float>(snapshot.currentWave) / static_cast<float>(snapshot.totalWaves) : 0.0F,
        phaseTint,
        Color{0.12F, 0.14F, 0.18F, 1.0F},
        205);
    SubmitHudBar(
        context,
        Vector2{6.0F, 7.75F},
        Vector2{4.8F, 0.32F},
        std::min(1.0F, static_cast<float>(snapshot.scrap) / 12.0F),
        Color{0.92F, 0.84F, 0.34F, 1.0F},
        Color{0.16F, 0.16F, 0.08F, 1.0F},
        205);

    for (int index = 0; index < snapshot.playerMaxHitPoints; ++index)
    {
        const bool active = index < snapshot.playerHitPoints;
        SubmitSprite(
            context,
            "HealthPip" + std::to_string(index),
            "materials/roguelite.heart",
            Vector2{-9.4F + static_cast<float>(index) * 0.65F, 7.1F},
            Vector2{0.42F, 0.42F},
            active ? Color{1.0F, 0.48F, 0.58F, 1.0F} : Color{0.32F, 0.18F, 0.22F, 1.0F},
            210);
    }

    for (int index = 0; index < snapshot.roomCount; ++index)
    {
        const bool active = index == snapshot.roomIndex;
        const bool cleared = index < snapshot.roomIndex;
        SubmitSprite(
            context,
            "RoomPip" + std::to_string(index),
            "materials/roguelite.portal",
            Vector2{-1.4F + static_cast<float>(index) * 0.72F, 7.1F},
            Vector2{0.45F, 0.30F},
            active ? phaseTint : (cleared ? Color{0.28F, 0.78F, 0.52F, 1.0F} : Color{0.20F, 0.24F, 0.28F, 1.0F}),
            210);
    }

    for (int index = 0; index < snapshot.totalWaves; ++index)
    {
        const bool active = index + 1 == snapshot.currentWave;
        const bool cleared = index + 1 < snapshot.currentWave;
        SubmitSprite(
            context,
            "WavePip" + std::to_string(index),
            "materials/roguelite.projectile_player",
            Vector2{3.8F + static_cast<float>(index) * 0.52F, 7.1F},
            Vector2{0.24F, 0.24F},
            active ? Color{0.88F, 0.96F, 1.0F, 1.0F}
                   : (cleared ? Color{0.56F, 0.72F, 0.94F, 1.0F} : Color{0.22F, 0.26F, 0.34F, 1.0F}),
            210);
    }

    for (int index = 0; index < std::max(snapshot.upgradeLevel, 1); ++index)
    {
        const bool unlocked = index < snapshot.upgradeLevel;
        SubmitSprite(
            context,
            "UpgradePip" + std::to_string(index),
            "materials/roguelite.core",
            Vector2{8.7F - static_cast<float>(index) * 0.52F, 7.1F},
            Vector2{0.28F, 0.28F},
            unlocked ? Color{0.92F, 0.98F, 1.0F, 1.0F} : Color{0.24F, 0.28F, 0.34F, 1.0F},
            210);
    }

    SubmitSprite(
        context,
        "StatusStrip",
        "materials/roguelite.hud",
        Vector2{0.0F, -7.75F},
        Vector2{12.5F, 0.22F},
        phaseTint,
        205);
}
} // namespace she
