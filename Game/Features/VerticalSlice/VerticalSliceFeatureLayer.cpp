#include "VerticalSliceFeatureLayer.hpp"

#include "SHE/Audio/AudioPlaybackContract.hpp"
#include "SHE/Core/Logger.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cctype>
#include <iterator>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace she
{
namespace
{
constexpr std::string_view kSceneName = "VerticalSliceArena";
constexpr std::string_view kScriptModuleName = "vertical_slice.arena_flow";
constexpr std::string_view kRoundTickTimerName = "vertical_slice.round_tick";
constexpr std::string_view kWaveTickTimerName = "vertical_slice.wave_tick";
constexpr std::string_view kStartRoundCommandName = "StartVerticalSliceRound";
constexpr std::string_view kRestartRoundCommandName = "RestartVerticalSliceRound";
constexpr std::string_view kSpawnPickupCommandName = "SpawnVerticalSlicePickup";
constexpr std::string_view kSpawnWaveCommandName = "SpawnVerticalSliceWave";
constexpr std::string_view kRoundMusicAssetName = "audio/vertical_slice.theme";
constexpr std::string_view kCollectSfxAssetName = "audio/vertical_slice.collect";
constexpr std::string_view kFailSfxAssetName = "audio/vertical_slice.fail";
constexpr std::string_view kRoundMusicChannelName = "music/vertical_slice";
constexpr std::string_view kPlayerMaterialName = "materials/vertical_slice.player";
constexpr std::string_view kPickupMaterialName = "materials/vertical_slice.pickup";
constexpr std::string_view kHazardMaterialName = "materials/vertical_slice.hazard";
constexpr std::string_view kBackdropMaterialName = "materials/vertical_slice.backdrop";
constexpr std::string_view kHudMaterialName = "materials/vertical_slice.hud";
constexpr std::string_view kTimerMaterialName = "materials/vertical_slice.timer";
constexpr double kRoundDurationSeconds = 24.0;
constexpr double kWaveTimerSeconds = 6.0;
constexpr int kTargetPickupCount = 3;
constexpr float kArenaHalfWidth = 7.5F;
constexpr float kArenaHalfHeight = 4.0F;
constexpr float kPlayerMoveSpeed = 5.25F;
constexpr float kPlayerHalfSize = 0.45F;

struct PickupDefinition
{
    const char* recordId = "";
    Vector2 position{};
    Color tint{};
};

struct WaveDefinition
{
    const char* recordId = "";
    const char* entityName = "";
    Vector2 position{};
    Vector2 velocity{};
    Vector2 size{1.0F, 1.0F};
};

constexpr PickupDefinition kPickupDefinitions[] = {
    {"feature.vertical_slice.pickup.alpha", Vector2{-5.8F, 2.3F}, Color{0.45F, 1.0F, 0.76F, 1.0F}},
    {"feature.vertical_slice.pickup.beta", Vector2{5.5F, 1.8F}, Color{0.56F, 0.94F, 0.82F, 1.0F}},
    {"feature.vertical_slice.pickup.gamma", Vector2{0.0F, -0.2F}, Color{0.92F, 1.0F, 0.60F, 1.0F}},
};

constexpr WaveDefinition kWaveDefinitions[] = {
    {"feature.vertical_slice.wave.opening", "SweepDrone", Vector2{-6.5F, 2.7F}, Vector2{3.35F, 0.0F}, Vector2{1.0F, 1.0F}},
    {"feature.vertical_slice.wave.flank", "FlankDrone", Vector2{4.7F, -3.0F}, Vector2{0.0F, 2.9F}, Vector2{0.95F, 0.95F}},
    {"feature.vertical_slice.wave.finale", "PincerDrone", Vector2{-5.0F, -2.2F}, Vector2{2.45F, 2.15F}, Vector2{0.9F, 0.9F}},
};

[[nodiscard]] float ClampFloat(const float value, const float minValue, const float maxValue)
{
    return std::clamp(value, minValue, maxValue);
}

[[nodiscard]] std::string TrimCopy(std::string_view value)
{
    std::size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0)
    {
        ++begin;
    }

    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
    {
        --end;
    }

    return std::string(value.substr(begin, end - begin));
}

[[nodiscard]] std::unordered_map<std::string, std::string> ParsePayload(std::string_view payload)
{
    std::unordered_map<std::string, std::string> fields;
    while (!payload.empty())
    {
        const std::size_t separator = payload.find(';');
        const std::string_view segment = separator == std::string_view::npos ? payload : payload.substr(0, separator);
        payload = separator == std::string_view::npos ? std::string_view{} : payload.substr(separator + 1);
        const std::size_t assignment = segment.find('=');
        if (assignment == std::string_view::npos)
        {
            continue;
        }

        const std::string key = TrimCopy(segment.substr(0, assignment));
        const std::string value = TrimCopy(segment.substr(assignment + 1));
        if (!key.empty())
        {
            fields.insert_or_assign(key, value);
        }
    }

    return fields;
}

template <typename IntegerType>
[[nodiscard]] bool TryParseInteger(
    const std::unordered_map<std::string, std::string>& fields,
    std::string_view fieldName,
    IntegerType& outValue)
{
    const auto field = fields.find(std::string(fieldName));
    if (field == fields.end())
    {
        return false;
    }

    try
    {
        outValue = static_cast<IntegerType>(std::stoull(field->second));
        return true;
    }
    catch (...)
    {
        return false;
    }
}

[[nodiscard]] bool TryParseFloat(
    const std::unordered_map<std::string, std::string>& fields,
    std::string_view fieldName,
    float& outValue)
{
    const auto field = fields.find(std::string(fieldName));
    if (field == fields.end())
    {
        return false;
    }

    try
    {
        outValue = std::stof(field->second);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

[[nodiscard]] std::string BuildRoundPayload(std::string_view reason)
{
    return "reason=" + std::string(reason);
}

[[nodiscard]] std::string BuildPickupPayload(const PickupDefinition& definition)
{
    std::ostringstream stream;
    stream << "record_id=" << definition.recordId << "; x=" << definition.position.x << "; y=" << definition.position.y;
    return stream.str();
}

[[nodiscard]] std::string BuildWavePayload(const WaveDefinition& definition)
{
    std::ostringstream stream;
    stream << "record_id=" << definition.recordId << "; entity_name=" << definition.entityName << "; x="
           << definition.position.x << "; y=" << definition.position.y << "; velocity_x=" << definition.velocity.x
           << "; velocity_y=" << definition.velocity.y << "; size_x=" << definition.size.x << "; size_y="
           << definition.size.y;
    return stream.str();
}

[[nodiscard]] bool IsTimerEvent(const GameplayEvent& event, std::string_view timerName)
{
    return event.source == "timer:" + std::string(timerName);
}

[[nodiscard]] bool IsMoveDown(const RuntimeServices& services, const KeyCode primary, const KeyCode secondary)
{
    return services.window->GetKeyState(primary).isDown || services.window->GetKeyState(secondary).isDown;
}

[[nodiscard]] Color BuildStateBackdropColor(const VerticalSliceRoundState state)
{
    switch (state)
    {
    case VerticalSliceRoundState::Booting:
        return Color{0.10F, 0.10F, 0.14F, 1.0F};
    case VerticalSliceRoundState::Playing:
        return Color{0.05F, 0.08F, 0.11F, 1.0F};
    case VerticalSliceRoundState::Won:
        return Color{0.08F, 0.18F, 0.10F, 1.0F};
    case VerticalSliceRoundState::Lost:
        return Color{0.18F, 0.07F, 0.08F, 1.0F};
    }

    return Color{0.08F, 0.08F, 0.08F, 1.0F};
}

[[nodiscard]] Color BuildTimerColor(const double ratio)
{
    const float clamped = ClampFloat(static_cast<float>(ratio), 0.0F, 1.0F);
    return Color{
        ClampFloat(1.0F - (1.0F - clamped) * 0.25F, 0.0F, 1.0F),
        ClampFloat(0.25F + clamped * 0.75F, 0.0F, 1.0F),
        ClampFloat(0.18F + clamped * 0.22F, 0.0F, 1.0F),
        1.0F};
}

[[nodiscard]] Vector2 ComputeHazardBounds(const Vector2& size)
{
    return Vector2{
        std::max(0.45F, size.x * 0.5F),
        std::max(0.45F, size.y * 0.5F)};
}
} // namespace

const char* GetVerticalSliceRoundStateName(const VerticalSliceRoundState state)
{
    switch (state)
    {
    case VerticalSliceRoundState::Booting:
        return "booting";
    case VerticalSliceRoundState::Playing:
        return "playing";
    case VerticalSliceRoundState::Won:
        return "won";
    case VerticalSliceRoundState::Lost:
        return "lost";
    }

    return "unknown";
}

VerticalSliceFeatureLayer::VerticalSliceFeatureLayer() : Layer("VerticalSliceFeatureLayer")
{
}

void VerticalSliceFeatureLayer::OnAttach(RuntimeServices& services)
{
    m_services = &services;
    m_roundState = VerticalSliceRoundState::Booting;
    m_remainingRoundSeconds = kRoundDurationSeconds;
    m_visualTimeSeconds = 0.0;
    m_collectedPickups = 0;
    m_spawnedWaveCount = 0;
    m_nextPickupIndex = 0;
    m_nextWaveIndex = 0;
    m_pendingPickupSpawn = false;
    m_pendingWaveSpawn = false;
    m_latestStatus = "Booting vertical slice.";
    m_latestDebugReport.clear();
    m_latestAuthoringContext.clear();
    m_latestGameplayDigest.clear();
    m_lastCompletedFrame.reset();

    services.ai->SetAuthoringIntent(
        "Ship a compact, playable vertical slice in Game/Features/VerticalSlice using gameplay commands, timers, "
        "feature-owned data, script-routed spawns, physics collisions, audio feedback, and the shared debug report.");

    RegisterFeatureContracts(services);
    RegisterSliceData(services);
    RegisterVisualAssets(services);
    RegisterScriptContracts(services);
    RegisterGameplayContracts(services);
    CreatePersistentScene(services);
    SubscribeToRuntimeEvents(services);
    QueueRoundSequence(false, "boot");

    SHE_LOG_INFO(
        "Game",
        "Vertical slice attached. Controls: WASD or arrows to move, collect three cores, avoid red drones, R to "
        "restart, Esc to quit.");
}

void VerticalSliceFeatureLayer::OnFixedUpdate(const TickContext& context)
{
    m_visualTimeSeconds += context.deltaSeconds;

    if (m_roundState == VerticalSliceRoundState::Playing)
    {
        UpdatePlayerMovement(context.deltaSeconds);
        UpdateHazards(context.deltaSeconds);
        UpdatePickupAnimation(context.deltaSeconds);
    }

    UpdateHud();
}

void VerticalSliceFeatureLayer::OnUpdate(const TickContext& context)
{
    if (!m_loggedControls)
    {
        m_loggedControls = true;
        SHE_LOG_INFO(
            "Game",
            "Vertical slice live: secure three signal cores before the round clock expires. R restarts after a win or "
            "loss.");
    }

    if (context.services.window->GetKeyState(KeyCode::Escape).pressed)
    {
        context.services.window->RequestClose();
        return;
    }

    if (m_roundState == VerticalSliceRoundState::Playing)
    {
        if (m_pendingPickupSpawn)
        {
            m_pendingPickupSpawn = !QueueNextPickupScript();
        }

        if (m_pendingWaveSpawn)
        {
            m_pendingWaveSpawn = !QueueNextWaveScript();
        }
    }

    if (m_roundState != VerticalSliceRoundState::Playing &&
        (context.services.window->GetKeyState(KeyCode::R).pressed ||
         context.services.window->GetKeyState(KeyCode::Enter).pressed))
    {
        QueueRoundSequence(true, "manual_restart");
    }
}

void VerticalSliceFeatureLayer::OnRender(const TickContext& context)
{
    const WindowState windowState = context.services.window->GetWindowState();
    const Color clearColor = BuildStateBackdropColor(m_roundState);
    context.services.renderer->SubmitCamera(
        Camera2DSubmission{
            m_cameraEntityId,
            "VerticalSliceCamera",
            Vector2{0.0F, 0.0F},
            1.0F,
            windowState.width,
            windowState.height,
            clearColor});

    SubmitEntitySprite(context, m_arenaBackdropEntityId, kBackdropMaterialName, Color{1.0F, 1.0F, 1.0F, 1.0F}, -100);
    SubmitEntitySprite(context, m_timerTrackEntityId, kHudMaterialName, Color{0.20F, 0.22F, 0.24F, 1.0F}, -90);
    SubmitEntitySprite(
        context,
        m_timerFillEntityId,
        kTimerMaterialName,
        BuildTimerColor(m_remainingRoundSeconds / kRoundDurationSeconds),
        -89);

    for (int index = 0; index < kTargetPickupCount; ++index)
    {
        const bool filled = index < m_collectedPickups;
        const Color tint = filled ? Color{0.84F, 1.0F, 0.66F, 1.0F} : Color{0.26F, 0.28F, 0.31F, 1.0F};
        SubmitEntitySprite(context, m_scorePipEntityIds[index], kHudMaterialName, tint, -88 + index);
    }

    Color beaconTint{0.48F, 0.53F, 0.60F, 1.0F};
    if (m_roundState == VerticalSliceRoundState::Playing)
    {
        beaconTint = Color{0.45F, 0.88F, 1.0F, 1.0F};
    }
    else if (m_roundState == VerticalSliceRoundState::Won)
    {
        beaconTint = Color{1.0F, 0.90F, 0.42F, 1.0F};
    }
    else if (m_roundState == VerticalSliceRoundState::Lost)
    {
        beaconTint = Color{1.0F, 0.38F, 0.38F, 1.0F};
    }
    SubmitEntitySprite(context, m_stateBeaconEntityId, kHudMaterialName, beaconTint, -84);

    SubmitEntitySprite(
        context,
        m_playerEntityId,
        kPlayerMaterialName,
        m_roundState == VerticalSliceRoundState::Won ? Color{1.0F, 0.96F, 0.60F, 1.0F}
                                                    : Color{1.0F, 1.0F, 1.0F, 1.0F},
        10);

    if (m_activePickupEntityId != kInvalidEntityId)
    {
        const float pickupPulse = 0.82F + 0.18F * std::sin(static_cast<float>(m_visualTimeSeconds * 4.0));
        SubmitEntitySprite(
            context,
            m_activePickupEntityId,
            kPickupMaterialName,
            Color{pickupPulse, pickupPulse, pickupPulse, 1.0F},
            2);
    }

    for (std::size_t index = 0; index < m_hazards.size(); ++index)
    {
        const float tintShift = static_cast<float>(index) * 0.08F;
        SubmitEntitySprite(
            context,
            m_hazards[index].entityId,
            kHazardMaterialName,
            Color{1.0F, ClampFloat(0.72F - tintShift, 0.22F, 1.0F), 0.60F, 1.0F},
            20 + static_cast<int>(index));
    }
}

void VerticalSliceFeatureLayer::OnUi(const TickContext& context)
{
    m_latestDebugReport = context.services.ui->BuildLatestDebugReport();
    m_latestGameplayDigest = context.services.gameplay->BuildGameplayDigest();
    const auto lastFrame = context.services.renderer->GetLastCompletedFrame();
    if (lastFrame.has_value())
    {
        m_lastCompletedFrame = lastFrame;
    }
}

void VerticalSliceFeatureLayer::OnDetach(RuntimeServices& services)
{
    if (m_collisionSubscriptionId != 0)
    {
        services.gameplay->UnsubscribeFromEvent(m_collisionSubscriptionId);
        m_collisionSubscriptionId = 0;
    }

    if (m_timerSubscriptionId != 0)
    {
        services.gameplay->UnsubscribeFromEvent(m_timerSubscriptionId);
        m_timerSubscriptionId = 0;
    }

    if (m_spawnWaveSubscriptionId != 0)
    {
        services.gameplay->UnsubscribeFromEvent(m_spawnWaveSubscriptionId);
        m_spawnWaveSubscriptionId = 0;
    }

    if (m_spawnPickupSubscriptionId != 0)
    {
        services.gameplay->UnsubscribeFromEvent(m_spawnPickupSubscriptionId);
        m_spawnPickupSubscriptionId = 0;
    }

    if (m_restartRoundSubscriptionId != 0)
    {
        services.gameplay->UnsubscribeFromEvent(m_restartRoundSubscriptionId);
        m_restartRoundSubscriptionId = 0;
    }

    if (m_startRoundSubscriptionId != 0)
    {
        services.gameplay->UnsubscribeFromEvent(m_startRoundSubscriptionId);
        m_startRoundSubscriptionId = 0;
    }

    CacheReports(services);
    m_services = nullptr;

    SHE_LOG_INFO("Game", "Vertical slice detached.");
}

VerticalSliceSnapshot VerticalSliceFeatureLayer::BuildSnapshot() const
{
    VerticalSliceSnapshot snapshot;
    snapshot.roundState = m_roundState;
    snapshot.collectedPickups = m_collectedPickups;
    snapshot.targetPickups = kTargetPickupCount;
    snapshot.activeHazardCount = m_hazards.size();
    snapshot.spawnedWaveCount = m_spawnedWaveCount;
    snapshot.remainingSeconds = m_remainingRoundSeconds;
    snapshot.roundStarted = m_roundStarted;
    snapshot.scriptModuleLoaded = m_scriptModuleLoaded;
    snapshot.playerEntityId = m_playerEntityId;
    snapshot.cameraEntityId = m_cameraEntityId;
    snapshot.activePickupEntityId = m_activePickupEntityId;
    snapshot.activePickupRecordId = m_activePickupRecordId;
    snapshot.latestStatus = m_latestStatus;
    snapshot.hazardEntityIds.reserve(m_hazards.size());
    for (const HazardInstance& hazard : m_hazards)
    {
        snapshot.hazardEntityIds.push_back(hazard.entityId);
    }

    return snapshot;
}

const std::string& VerticalSliceFeatureLayer::GetLatestDebugReport() const
{
    return m_latestDebugReport;
}

const std::string& VerticalSliceFeatureLayer::GetLatestAuthoringContext() const
{
    return m_latestAuthoringContext;
}

const std::string& VerticalSliceFeatureLayer::GetLatestGameplayDigest() const
{
    return m_latestGameplayDigest;
}

const std::optional<RenderFrameSnapshot>& VerticalSliceFeatureLayer::GetLastCompletedFrame() const
{
    return m_lastCompletedFrame;
}

void VerticalSliceFeatureLayer::RegisterFeatureContracts(RuntimeServices& services)
{
    services.scene->SetActiveScene(std::string(kSceneName));
    services.reflection->RegisterType(
        "feature_component",
        "VerticalSliceHazard",
        "Authoring-visible hazard actor used by the W12 vertical slice.");
    services.reflection->RegisterType(
        "feature_component",
        "VerticalSlicePickup",
        "Collectible signal core tracked by the W12 vertical slice.");
    services.reflection->RegisterFeature(
        "VerticalSliceFeature",
        "Game/Features/VerticalSlice",
        "Playable W12 slice that exercises gameplay, scripting, data, scene, renderer, physics, audio, UI, and AI "
        "context as one small loop.");
}

void VerticalSliceFeatureLayer::RegisterSliceData(RuntimeServices& services)
{
    services.data->RegisterSchema(
        DataSchemaContract{
            "feature.vertical_slice.round",
            "Round-level tuning contract for the W12 vertical slice.",
            "Game/Features/VerticalSlice",
            {
                DataSchemaFieldContract{"id", "scalar", true, "Stable round identifier."},
                DataSchemaFieldContract{"objective", "scalar", true, "Designer-facing goal text."},
                DataSchemaFieldContract{"round_seconds", "scalar", true, "Round clock in seconds."},
                DataSchemaFieldContract{"wave_interval_seconds", "scalar", true, "Hazard escalation cadence."},
                DataSchemaFieldContract{"pickup_target", "scalar", true, "Number of cores required to win."},
            }});
    services.data->RegisterSchema(
        DataSchemaContract{
            "feature.vertical_slice.pickup",
            "Pickup spawn contract for authored signal-core placements.",
            "Game/Features/VerticalSlice",
            {
                DataSchemaFieldContract{"id", "scalar", true, "Stable pickup identifier."},
                DataSchemaFieldContract{"spawn", "map", true, "World-space spawn marker."},
                DataSchemaFieldContract{"reward", "scalar", true, "Reward granted on collect."},
                DataSchemaFieldContract{"lane", "scalar", true, "Arena lane name."},
            }});
    services.data->RegisterSchema(
        DataSchemaContract{
            "feature.vertical_slice.wave",
            "Hazard wave contract for the vertical slice patrol drones.",
            "Game/Features/VerticalSlice",
            {
                DataSchemaFieldContract{"id", "scalar", true, "Stable wave identifier."},
                DataSchemaFieldContract{"spawn", "map", true, "Drone spawn transform."},
                DataSchemaFieldContract{"velocity", "map", true, "Initial patrol velocity."},
                DataSchemaFieldContract{"size", "map", true, "Sprite and collider size."},
                DataSchemaFieldContract{"cadence", "scalar", true, "High-level authored cadence label."},
            }});

    static_cast<void>(services.data->LoadRecord(
        DataLoadRequest{
            "feature.vertical_slice.round",
            "feature.vertical_slice.round.main",
            "Game/Features/VerticalSlice/Data/round_main.yml",
            "id: feature.vertical_slice.round.main\n"
            "objective: secure_three_signal_cores\n"
            "round_seconds: 24\n"
            "wave_interval_seconds: 6\n"
            "pickup_target: 3\n"}));
    static_cast<void>(services.data->LoadRecord(
        DataLoadRequest{
            "feature.vertical_slice.pickup",
            "feature.vertical_slice.pickup.alpha",
            "Game/Features/VerticalSlice/Data/pickup_alpha.yml",
            "id: feature.vertical_slice.pickup.alpha\n"
            "spawn:\n"
            "  x: -5.8\n"
            "  y: 2.3\n"
            "reward: signal_core\n"
            "lane: north\n"}));
    static_cast<void>(services.data->LoadRecord(
        DataLoadRequest{
            "feature.vertical_slice.pickup",
            "feature.vertical_slice.pickup.beta",
            "Game/Features/VerticalSlice/Data/pickup_beta.yml",
            "id: feature.vertical_slice.pickup.beta\n"
            "spawn:\n"
            "  x: 5.5\n"
            "  y: 1.8\n"
            "reward: signal_core\n"
            "lane: east\n"}));
    static_cast<void>(services.data->LoadRecord(
        DataLoadRequest{
            "feature.vertical_slice.pickup",
            "feature.vertical_slice.pickup.gamma",
            "Game/Features/VerticalSlice/Data/pickup_gamma.yml",
            "id: feature.vertical_slice.pickup.gamma\n"
            "spawn:\n"
            "  x: 0.0\n"
            "  y: -0.2\n"
            "reward: signal_core\n"
            "lane: center\n"}));
    static_cast<void>(services.data->LoadRecord(
        DataLoadRequest{
            "feature.vertical_slice.wave",
            "feature.vertical_slice.wave.opening",
            "Game/Features/VerticalSlice/Data/wave_opening.yml",
            "id: feature.vertical_slice.wave.opening\n"
            "spawn:\n"
            "  x: -6.5\n"
            "  y: 2.7\n"
            "velocity:\n"
            "  x: 3.35\n"
            "  y: 0.0\n"
            "size:\n"
            "  x: 1.0\n"
            "  y: 1.0\n"
            "cadence: opening\n"}));
    static_cast<void>(services.data->LoadRecord(
        DataLoadRequest{
            "feature.vertical_slice.wave",
            "feature.vertical_slice.wave.flank",
            "Game/Features/VerticalSlice/Data/wave_flank.yml",
            "id: feature.vertical_slice.wave.flank\n"
            "spawn:\n"
            "  x: 4.7\n"
            "  y: -3.0\n"
            "velocity:\n"
            "  x: 0.0\n"
            "  y: 2.9\n"
            "size:\n"
            "  x: 0.95\n"
            "  y: 0.95\n"
            "cadence: flank\n"}));
    static_cast<void>(services.data->LoadRecord(
        DataLoadRequest{
            "feature.vertical_slice.wave",
            "feature.vertical_slice.wave.finale",
            "Game/Features/VerticalSlice/Data/wave_finale.yml",
            "id: feature.vertical_slice.wave.finale\n"
            "spawn:\n"
            "  x: -5.0\n"
            "  y: -2.2\n"
            "velocity:\n"
            "  x: 2.45\n"
            "  y: 2.15\n"
            "size:\n"
            "  x: 0.9\n"
            "  y: 0.9\n"
            "cadence: finale\n"}));
}

void VerticalSliceFeatureLayer::RegisterVisualAssets(RuntimeServices& services)
{
    services.assets->RegisterLoader(
        AssetLoaderDescriptor{
            "texture_placeholder",
            "texture",
            {".placeholder", ".png"},
            "Placeholder texture loader for renderer-generated feature materials."});

    services.assets->RegisterAsset(
        AssetMetadata{
            kInvalidAssetId,
            "textures/vertical_slice/player",
            "Game/Features/VerticalSlice/Data/player.placeholder",
            "texture",
            "",
            "Game/Features/VerticalSlice",
            "feature.vertical_slice.visual.player",
            {"vertical_slice", "player", "sprite"},
            {
                {"usage", "player"},
                {"palette", "teal"},
            }});
    services.assets->RegisterAsset(
        AssetMetadata{
            kInvalidAssetId,
            "textures/vertical_slice/pickup",
            "Game/Features/VerticalSlice/Data/pickup.placeholder",
            "texture",
            "",
            "Game/Features/VerticalSlice",
            "feature.vertical_slice.visual.pickup",
            {"vertical_slice", "pickup", "sprite"},
            {
                {"usage", "objective"},
            }});
    services.assets->RegisterAsset(
        AssetMetadata{
            kInvalidAssetId,
            "textures/vertical_slice/hazard",
            "Game/Features/VerticalSlice/Data/hazard.placeholder",
            "texture",
            "",
            "Game/Features/VerticalSlice",
            "feature.vertical_slice.visual.hazard",
            {"vertical_slice", "hazard", "sprite"},
            {
                {"usage", "hazard"},
            }});
    services.assets->RegisterAsset(
        AssetMetadata{
            kInvalidAssetId,
            "textures/vertical_slice/backdrop",
            "Game/Features/VerticalSlice/Data/backdrop.placeholder",
            "texture",
            "",
            "Game/Features/VerticalSlice",
            "feature.vertical_slice.visual.backdrop",
            {"vertical_slice", "arena", "sprite"},
            {
                {"usage", "backdrop"},
            }});
    services.assets->RegisterAsset(
        AssetMetadata{
            kInvalidAssetId,
            "textures/vertical_slice/hud",
            "Game/Features/VerticalSlice/Data/hud.placeholder",
            "texture",
            "",
            "Game/Features/VerticalSlice",
            "feature.vertical_slice.visual.hud",
            {"vertical_slice", "hud", "sprite"},
            {
                {"usage", "hud"},
            }});
    services.assets->RegisterAsset(
        AssetMetadata{
            kInvalidAssetId,
            "textures/vertical_slice/timer",
            "Game/Features/VerticalSlice/Data/timer.placeholder",
            "texture",
            "",
            "Game/Features/VerticalSlice",
            "feature.vertical_slice.visual.timer",
            {"vertical_slice", "hud", "timer"},
            {
                {"usage", "timer"},
            }});
    services.assets->RegisterAsset(
        AssetMetadata{
            kInvalidAssetId,
            std::string(kRoundMusicAssetName),
            "Game/Features/VerticalSlice/Data/theme.wav",
            "audio",
            "",
            "Game/Features/VerticalSlice",
            "feature.vertical_slice.audio.theme",
            {"vertical_slice", "music"},
            {
                {"loop", "true"},
            }});
    services.assets->RegisterAsset(
        AssetMetadata{
            kInvalidAssetId,
            std::string(kCollectSfxAssetName),
            "Game/Features/VerticalSlice/Data/collect.wav",
            "audio",
            "",
            "Game/Features/VerticalSlice",
            "feature.vertical_slice.audio.collect",
            {"vertical_slice", "sfx"},
            {
                {"usage", "reward"},
            }});
    services.assets->RegisterAsset(
        AssetMetadata{
            kInvalidAssetId,
            std::string(kFailSfxAssetName),
            "Game/Features/VerticalSlice/Data/fail.wav",
            "audio",
            "",
            "Game/Features/VerticalSlice",
            "feature.vertical_slice.audio.fail",
            {"vertical_slice", "sfx"},
            {
                {"usage", "failure"},
            }});

    services.renderer->RegisterMaterial(
        Material2DDescriptor{
            std::string(kPlayerMaterialName),
            "textures/vertical_slice/player",
            Color{0.66F, 0.98F, 1.0F, 1.0F}});
    services.renderer->RegisterMaterial(
        Material2DDescriptor{
            std::string(kPickupMaterialName),
            "textures/vertical_slice/pickup",
            Color{0.66F, 1.0F, 0.76F, 1.0F}});
    services.renderer->RegisterMaterial(
        Material2DDescriptor{
            std::string(kHazardMaterialName),
            "textures/vertical_slice/hazard",
            Color{1.0F, 0.60F, 0.60F, 1.0F}});
    services.renderer->RegisterMaterial(
        Material2DDescriptor{
            std::string(kBackdropMaterialName),
            "textures/vertical_slice/backdrop",
            Color{0.20F, 0.28F, 0.32F, 1.0F}});
    services.renderer->RegisterMaterial(
        Material2DDescriptor{
            std::string(kHudMaterialName),
            "textures/vertical_slice/hud",
            Color{0.82F, 0.86F, 0.92F, 1.0F}});
    services.renderer->RegisterMaterial(
        Material2DDescriptor{
            std::string(kTimerMaterialName),
            "textures/vertical_slice/timer",
            Color{0.94F, 0.96F, 0.98F, 1.0F}});
}

void VerticalSliceFeatureLayer::RegisterScriptContracts(RuntimeServices& services)
{
    services.scripting->RegisterBinding(
        {"gameplay.queue_command", "GameplayService", "Route authored vertical-slice actions through gameplay commands."});
    services.scripting->RegisterBinding(
        {"data.describe_registry", "DataService", "Expose the authored slice registry summary to scripts."});

    services.scripting->RegisterScriptModule(
        ScriptModuleContract{
            std::string(kScriptModuleName),
            "Game/Features/VerticalSlice/Scripts/arena_flow.lua",
            "lua",
            "Game/Features/VerticalSlice",
            "Feature-local script contract that routes round, pickup, and wave authoring decisions through gameplay "
            "commands.",
            true,
            {"gameplay.queue_command", "data.describe_registry"},
            {
                ScriptFunctionContract{
                    "open_round",
                    "Open the slice round through the gameplay command registry.",
                    std::string(kStartRoundCommandName)},
                ScriptFunctionContract{
                    "restart_round",
                    "Restart the slice after a win or loss.",
                    std::string(kRestartRoundCommandName)},
                ScriptFunctionContract{
                    "queue_pickup",
                    "Spawn the next authored pickup through a command-routed path.",
                    std::string(kSpawnPickupCommandName)},
                ScriptFunctionContract{
                    "queue_wave",
                    "Spawn the next authored hazard wave through a command-routed path.",
                    std::string(kSpawnWaveCommandName)},
            }});

    const ScriptModuleLoadResult loadResult = services.scripting->LoadScriptModule(kScriptModuleName);
    m_scriptModuleLoaded = loadResult.loaded;
    if (!m_scriptModuleLoaded)
    {
        SHE_LOG_WARNING("Game", "Vertical slice script module did not load cleanly; gameplay will fall back to direct commands.");
    }
}

void VerticalSliceFeatureLayer::RegisterGameplayContracts(RuntimeServices& services)
{
    services.gameplay->RegisterCommand(
        {std::string(kStartRoundCommandName), "Open the W12 vertical slice round.", "slice", "StartRoundRequested"});
    services.gameplay->RegisterCommand(
        {std::string(kRestartRoundCommandName), "Restart the W12 vertical slice round.", "slice", "RestartRoundRequested"});
    services.gameplay->RegisterCommand(
        {std::string(kSpawnPickupCommandName), "Spawn the next W12 signal core.", "slice", "SpawnPickupRequested"});
    services.gameplay->RegisterCommand(
        {std::string(kSpawnWaveCommandName), "Spawn the next W12 hazard wave.", "slice", "SpawnWaveRequested"});
}

void VerticalSliceFeatureLayer::CreatePersistentScene(RuntimeServices& services)
{
    m_cameraEntityId = services.scene->CreateEntity("VerticalSliceCamera");
    m_arenaBackdropEntityId = services.scene->CreateEntity("ArenaBackdrop");
    m_playerEntityId = services.scene->CreateEntity("PlayerRunner");
    m_timerTrackEntityId = services.scene->CreateEntity("TimerTrack");
    m_timerFillEntityId = services.scene->CreateEntity("TimerFill");
    m_stateBeaconEntityId = services.scene->CreateEntity("StateBeacon");

    for (int index = 0; index < kTargetPickupCount; ++index)
    {
        m_scorePipEntityIds[index] = services.scene->CreateEntity("ScorePip" + std::to_string(index + 1));
    }

    services.scene->TrySetEntityTransform(
        m_cameraEntityId,
        SceneTransform{
            Vector2{0.0F, 0.0F},
            0.0F,
            Vector2{1.0F, 1.0F}});
    services.scene->TrySetEntityTransform(
        m_arenaBackdropEntityId,
        SceneTransform{
            Vector2{0.0F, 0.0F},
            0.0F,
            Vector2{16.8F, 9.2F}});
    services.scene->TrySetEntityTransform(
        m_playerEntityId,
        SceneTransform{
            Vector2{0.0F, -2.6F},
            0.0F,
            Vector2{0.9F, 0.9F}});
    services.scene->TrySetEntityTransform(
        m_timerTrackEntityId,
        SceneTransform{
            Vector2{0.0F, 4.75F},
            0.0F,
            Vector2{6.4F, 0.34F}});
    services.scene->TrySetEntityTransform(
        m_timerFillEntityId,
        SceneTransform{
            Vector2{0.0F, 4.75F},
            0.0F,
            Vector2{6.0F, 0.22F}});
    services.scene->TrySetEntityTransform(
        m_stateBeaconEntityId,
        SceneTransform{
            Vector2{7.0F, 4.72F},
            0.0F,
            Vector2{0.5F, 0.5F}});

    for (int index = 0; index < kTargetPickupCount; ++index)
    {
        services.scene->TrySetEntityTransform(
            m_scorePipEntityIds[index],
            SceneTransform{
                Vector2{-7.15F + static_cast<float>(index) * 0.72F, 4.72F},
                0.0F,
                Vector2{0.42F, 0.42F}});
    }

    services.physics->CreateBody(
        m_playerEntityId,
        PhysicsBodyDefinition{
            PhysicsBodyType::Kinematic,
            Vector2{},
            0.0F,
            0.0F,
            0.0F,
            0.0F,
            true,
            false,
            true});
    static_cast<void>(services.physics->CreateBoxCollider(
        m_playerEntityId,
        PhysicsBoxColliderDefinition{
            Vector2{kPlayerHalfSize, kPlayerHalfSize},
            Vector2{},
            0.0F,
            1.0F,
            0.0F,
            0.0F,
            false}));

    UpdateHud();
}

void VerticalSliceFeatureLayer::SubscribeToRuntimeEvents(RuntimeServices& services)
{
    m_startRoundSubscriptionId = services.gameplay->SubscribeToEvent(
        "slice",
        "StartRoundRequested",
        [this](const GameplayEvent& event)
        {
            const auto fields = ParsePayload(event.payload);
            const auto reason = fields.find("reason");
            ResetRoundState(false, reason == fields.end() ? "scripted_start" : reason->second);
        });
    m_restartRoundSubscriptionId = services.gameplay->SubscribeToEvent(
        "slice",
        "RestartRoundRequested",
        [this](const GameplayEvent& event)
        {
            const auto fields = ParsePayload(event.payload);
            const auto reason = fields.find("reason");
            ResetRoundState(true, reason == fields.end() ? "restart" : reason->second);
        });
    m_spawnPickupSubscriptionId = services.gameplay->SubscribeToEvent(
        "slice",
        "SpawnPickupRequested",
        [this](const GameplayEvent& event)
        {
            HandleSpawnPickupRequested(event);
        });
    m_spawnWaveSubscriptionId = services.gameplay->SubscribeToEvent(
        "slice",
        "SpawnWaveRequested",
        [this](const GameplayEvent& event)
        {
            HandleSpawnWaveRequested(event);
        });
    m_timerSubscriptionId = services.gameplay->SubscribeToEvent(
        "timer",
        "TimerElapsed",
        [this](const GameplayEvent& event)
        {
            HandleTimerElapsed(event);
        });
    m_collisionSubscriptionId = services.gameplay->SubscribeToEvent(
        "physics",
        "CollisionStarted",
        [this](const GameplayEvent& event)
        {
            HandleCollisionStarted(event);
        });
}

void VerticalSliceFeatureLayer::QueueRoundSequence(const bool restart, std::string reason)
{
    if (m_services == nullptr)
    {
        return;
    }

    m_pendingPickupSpawn = false;
    m_pendingWaveSpawn = false;
    m_nextPickupIndex = 0;
    m_nextWaveIndex = 0;

    const std::string payload = BuildRoundPayload(reason);
    const std::string functionName = restart ? "restart_round" : "open_round";
    const std::string_view fallbackCommand = restart ? kRestartRoundCommandName : kStartRoundCommandName;
    const ScriptInvocationResult invocation =
        m_services->scripting->InvokeScriptFunction(kScriptModuleName, functionName, payload);
    if (!invocation.accepted || !invocation.queuedGameplayCommand)
    {
        m_services->gameplay->QueueCommand(std::string(fallbackCommand), payload);
    }

    static_cast<void>(QueueNextPickupScript());
    static_cast<void>(QueueNextWaveScript());
}

bool VerticalSliceFeatureLayer::QueueNextPickupScript()
{
    if (m_services == nullptr || m_nextPickupIndex >= std::size(kPickupDefinitions))
    {
        return false;
    }

    const PickupDefinition& definition = kPickupDefinitions[m_nextPickupIndex++];
    const std::string payload = BuildPickupPayload(definition);
    const ScriptInvocationResult invocation =
        m_services->scripting->InvokeScriptFunction(kScriptModuleName, "queue_pickup", payload);
    if (!invocation.accepted || !invocation.queuedGameplayCommand)
    {
        m_services->gameplay->QueueCommand(std::string(kSpawnPickupCommandName), payload);
    }

    return true;
}

bool VerticalSliceFeatureLayer::QueueNextWaveScript()
{
    if (m_services == nullptr || m_nextWaveIndex >= std::size(kWaveDefinitions))
    {
        return false;
    }

    const WaveDefinition& definition = kWaveDefinitions[m_nextWaveIndex++];
    const std::string payload = BuildWavePayload(definition);
    const ScriptInvocationResult invocation =
        m_services->scripting->InvokeScriptFunction(kScriptModuleName, "queue_wave", payload);
    if (!invocation.accepted || !invocation.queuedGameplayCommand)
    {
        m_services->gameplay->QueueCommand(std::string(kSpawnWaveCommandName), payload);
    }

    return true;
}

void VerticalSliceFeatureLayer::ResetRoundState(const bool restart, const std::string& reason)
{
    if (m_services == nullptr)
    {
        return;
    }

    DestroyActivePickup();
    DestroyHazards();

    m_roundState = VerticalSliceRoundState::Playing;
    m_roundStarted = true;
    m_collectedPickups = 0;
    m_spawnedWaveCount = 0;
    m_remainingRoundSeconds = kRoundDurationSeconds;
    m_visualTimeSeconds = 0.0;
    m_activePickupRecordId.clear();
    m_pendingPickupSpawn = false;
    m_pendingWaveSpawn = false;
    m_latestStatus = restart ? "Round restarted." : "Round started.";

    m_services->scene->TrySetEntityTransform(
        m_playerEntityId,
        SceneTransform{
            Vector2{0.0F, -2.6F},
            0.0F,
            Vector2{0.9F, 0.9F}});

    m_services->gameplay->CreateTimer(std::string(kRoundTickTimerName), 1.0, true);
    m_services->gameplay->CreateTimer(std::string(kWaveTickTimerName), kWaveTimerSeconds, true);
    m_services->gameplay->QueueEvent(
        "slice",
        restart ? "RoundRestarted" : "RoundStarted",
        "reason=" + reason + "; target_pickups=" + std::to_string(kTargetPickupCount));
    QueueMusicStop();
    QueueMusicStart();
    UpdateHud();
}

void VerticalSliceFeatureLayer::HandleSpawnPickupRequested(const GameplayEvent& event)
{
    if (m_services == nullptr || m_roundState != VerticalSliceRoundState::Playing)
    {
        return;
    }

    const auto fields = ParsePayload(event.payload);
    float x = 0.0F;
    float y = 0.0F;
    if (!TryParseFloat(fields, "x", x) || !TryParseFloat(fields, "y", y))
    {
        SHE_LOG_WARNING("Game", "Vertical slice pickup payload was missing spawn coordinates.");
        return;
    }

    DestroyActivePickup();

    const auto recordField = fields.find("record_id");
    m_activePickupRecordId = recordField == fields.end() ? "feature.vertical_slice.pickup.unknown" : recordField->second;
    m_activePickupBasePosition = Vector2{x, y};
    m_activePickupEntityId = m_services->scene->CreateEntity("SignalCore");
    m_services->scene->TrySetEntityTransform(
        m_activePickupEntityId,
        SceneTransform{
            m_activePickupBasePosition,
            0.0F,
            Vector2{0.85F, 0.85F}});
    m_services->physics->CreateBody(
        m_activePickupEntityId,
        PhysicsBodyDefinition{
            PhysicsBodyType::Static,
            Vector2{},
            0.0F,
            0.0F,
            0.0F,
            0.0F,
            true,
            false,
            true});
    static_cast<void>(m_services->physics->CreateBoxCollider(
        m_activePickupEntityId,
        PhysicsBoxColliderDefinition{
            Vector2{0.42F, 0.42F},
            Vector2{},
            0.0F,
            1.0F,
            0.0F,
            0.0F,
            true}));
    m_services->gameplay->QueueEvent(
        "slice",
        "PickupSpawned",
        "record_id=" + m_activePickupRecordId + "; collected=" + std::to_string(m_collectedPickups));
    m_latestStatus = "Signal core spawned.";
}

void VerticalSliceFeatureLayer::HandleSpawnWaveRequested(const GameplayEvent& event)
{
    if (m_services == nullptr || m_roundState != VerticalSliceRoundState::Playing)
    {
        return;
    }

    const auto fields = ParsePayload(event.payload);
    float x = 0.0F;
    float y = 0.0F;
    float velocityX = 0.0F;
    float velocityY = 0.0F;
    float sizeX = 1.0F;
    float sizeY = 1.0F;
    if (!TryParseFloat(fields, "x", x) || !TryParseFloat(fields, "y", y) ||
        !TryParseFloat(fields, "velocity_x", velocityX) || !TryParseFloat(fields, "velocity_y", velocityY) ||
        !TryParseFloat(fields, "size_x", sizeX) || !TryParseFloat(fields, "size_y", sizeY))
    {
        SHE_LOG_WARNING("Game", "Vertical slice wave payload was missing authored movement fields.");
        return;
    }

    const auto nameField = fields.find("entity_name");
    const auto recordField = fields.find("record_id");
    const std::string entityName = nameField == fields.end() ? "HazardDrone" : nameField->second;

    HazardInstance hazard;
    hazard.entityId = m_services->scene->CreateEntity(entityName);
    hazard.velocity = Vector2{velocityX, velocityY};
    hazard.size = Vector2{sizeX, sizeY};
    hazard.recordId = recordField == fields.end() ? "feature.vertical_slice.wave.unknown" : recordField->second;

    m_services->scene->TrySetEntityTransform(
        hazard.entityId,
        SceneTransform{
            Vector2{x, y},
            0.0F,
            hazard.size});
    m_services->physics->CreateBody(
        hazard.entityId,
        PhysicsBodyDefinition{
            PhysicsBodyType::Kinematic,
            Vector2{},
            0.0F,
            0.0F,
            0.0F,
            0.0F,
            true,
            false,
            true});
    static_cast<void>(m_services->physics->CreateBoxCollider(
        hazard.entityId,
        PhysicsBoxColliderDefinition{
            Vector2{hazard.size.x * 0.45F, hazard.size.y * 0.45F},
            Vector2{},
            0.0F,
            1.0F,
            0.0F,
            0.0F,
            true}));

    m_hazards.push_back(std::move(hazard));
    ++m_spawnedWaveCount;
    m_services->gameplay->QueueEvent(
        "slice",
        "WaveSpawned",
        "record_id=" + m_hazards.back().recordId + "; hazard_count=" + std::to_string(m_hazards.size()));
    m_latestStatus = "Hazard wave entered the arena.";
}

void VerticalSliceFeatureLayer::HandleTimerElapsed(const GameplayEvent& event)
{
    if (m_roundState != VerticalSliceRoundState::Playing)
    {
        return;
    }

    if (IsTimerEvent(event, kRoundTickTimerName))
    {
        m_remainingRoundSeconds = std::max(0.0, m_remainingRoundSeconds - 1.0);
        m_services->gameplay->QueueEvent(
            "slice",
            "RoundClockAdvanced",
            "seconds_remaining=" + std::to_string(static_cast<int>(m_remainingRoundSeconds)));
        if (m_remainingRoundSeconds <= 0.0)
        {
            FinishRound(false, "timer_expired");
        }
        return;
    }

    if (IsTimerEvent(event, kWaveTickTimerName) && m_nextWaveIndex < std::size(kWaveDefinitions))
    {
        m_pendingWaveSpawn = true;
        m_services->gameplay->QueueEvent(
            "slice",
            "WaveQueued",
            "next_wave_record=" + std::string(kWaveDefinitions[m_nextWaveIndex].recordId));
    }
}

void VerticalSliceFeatureLayer::HandleCollisionStarted(const GameplayEvent& event)
{
    if (m_roundState != VerticalSliceRoundState::Playing || m_services == nullptr)
    {
        return;
    }

    const auto fields = ParsePayload(event.payload);
    std::uint64_t entityA = 0;
    std::uint64_t entityB = 0;
    if (!TryParseInteger(fields, "entity_a", entityA) || !TryParseInteger(fields, "entity_b", entityB))
    {
        return;
    }

    if (entityA != m_playerEntityId && entityB != m_playerEntityId)
    {
        return;
    }

    const EntityId otherEntity =
        static_cast<EntityId>(entityA == m_playerEntityId ? entityB : entityA);
    if (otherEntity == m_activePickupEntityId && m_activePickupEntityId != kInvalidEntityId)
    {
        DestroyActivePickup();
        ++m_collectedPickups;
        QueueSfx(kCollectSfxAssetName, "sfx/vertical_slice.collect");
        m_services->gameplay->QueueEvent(
            "slice",
            "PickupCollected",
            "collected=" + std::to_string(m_collectedPickups) + "; target=" + std::to_string(kTargetPickupCount));
        m_latestStatus = "Signal core secured.";

        if (m_collectedPickups >= kTargetPickupCount)
        {
            FinishRound(true, "all_cores_secured");
        }
        else
        {
            m_pendingPickupSpawn = true;
        }

        UpdateHud();
        return;
    }

    const auto hazardIt = std::find_if(
        m_hazards.begin(),
        m_hazards.end(),
        [otherEntity](const HazardInstance& hazard)
        {
            return hazard.entityId == otherEntity;
        });
    if (hazardIt != m_hazards.end())
    {
        FinishRound(false, "hazard_contact");
    }
}

void VerticalSliceFeatureLayer::FinishRound(const bool won, const std::string& reason)
{
    if (m_roundState != VerticalSliceRoundState::Playing || m_services == nullptr)
    {
        return;
    }

    m_roundState = won ? VerticalSliceRoundState::Won : VerticalSliceRoundState::Lost;
    m_pendingPickupSpawn = false;
    m_pendingWaveSpawn = false;
    for (HazardInstance& hazard : m_hazards)
    {
        hazard.velocity = Vector2{};
    }

    m_latestStatus = won ? "Round won. Press R to run it again." : "Round lost. Press R to retry.";
    QueueMusicStop();
    QueueSfx(won ? kCollectSfxAssetName : kFailSfxAssetName, won ? "sfx/vertical_slice.win" : "sfx/vertical_slice.fail");
    m_services->gameplay->QueueEvent(
        "slice",
        won ? "RoundWon" : "RoundLost",
        "reason=" + reason + "; collected=" + std::to_string(m_collectedPickups) + "; target=" +
            std::to_string(kTargetPickupCount));
    UpdateHud();
}

void VerticalSliceFeatureLayer::UpdatePlayerMovement(const double deltaSeconds)
{
    if (m_services == nullptr)
    {
        return;
    }

    SceneEntityView playerView;
    if (!m_services->scene->TryGetEntityView(m_playerEntityId, playerView))
    {
        return;
    }

    Vector2 movement{};
    if (IsMoveDown(*m_services, KeyCode::A, KeyCode::Left))
    {
        movement.x -= 1.0F;
    }
    if (IsMoveDown(*m_services, KeyCode::D, KeyCode::Right))
    {
        movement.x += 1.0F;
    }
    if (IsMoveDown(*m_services, KeyCode::W, KeyCode::Up))
    {
        movement.y += 1.0F;
    }
    if (IsMoveDown(*m_services, KeyCode::S, KeyCode::Down))
    {
        movement.y -= 1.0F;
    }

    const float magnitude = std::sqrt(movement.x * movement.x + movement.y * movement.y);
    if (magnitude > 0.0001F)
    {
        movement.x /= magnitude;
        movement.y /= magnitude;
    }

    SceneTransform transform = playerView.transform;
    transform.position.x = ClampFloat(
        transform.position.x + movement.x * kPlayerMoveSpeed * static_cast<float>(deltaSeconds),
        -kArenaHalfWidth + kPlayerHalfSize,
        kArenaHalfWidth - kPlayerHalfSize);
    transform.position.y = ClampFloat(
        transform.position.y + movement.y * kPlayerMoveSpeed * static_cast<float>(deltaSeconds),
        -kArenaHalfHeight + kPlayerHalfSize,
        kArenaHalfHeight - kPlayerHalfSize);
    transform.rotationDegrees = movement.x * -9.0F;
    m_services->scene->TrySetEntityTransform(m_playerEntityId, transform);
}

void VerticalSliceFeatureLayer::UpdateHazards(const double deltaSeconds)
{
    if (m_services == nullptr)
    {
        return;
    }

    for (HazardInstance& hazard : m_hazards)
    {
        SceneEntityView hazardView;
        if (!m_services->scene->TryGetEntityView(hazard.entityId, hazardView))
        {
            continue;
        }

        SceneTransform transform = hazardView.transform;
        transform.position.x += hazard.velocity.x * static_cast<float>(deltaSeconds);
        transform.position.y += hazard.velocity.y * static_cast<float>(deltaSeconds);
        const Vector2 bounds = ComputeHazardBounds(hazard.size);

        if (transform.position.x < -kArenaHalfWidth + bounds.x || transform.position.x > kArenaHalfWidth - bounds.x)
        {
            hazard.velocity.x *= -1.0F;
            transform.position.x = ClampFloat(transform.position.x, -kArenaHalfWidth + bounds.x, kArenaHalfWidth - bounds.x);
        }

        if (transform.position.y < -kArenaHalfHeight + bounds.y || transform.position.y > kArenaHalfHeight - bounds.y)
        {
            hazard.velocity.y *= -1.0F;
            transform.position.y = ClampFloat(transform.position.y, -kArenaHalfHeight + bounds.y, kArenaHalfHeight - bounds.y);
        }

        transform.rotationDegrees += static_cast<float>(deltaSeconds * 115.0 * (hazard.velocity.x >= 0.0F ? 1.0 : -1.0));
        m_services->scene->TrySetEntityTransform(hazard.entityId, transform);
    }
}

void VerticalSliceFeatureLayer::UpdatePickupAnimation(const double)
{
    if (m_services == nullptr || m_activePickupEntityId == kInvalidEntityId)
    {
        return;
    }

    const float bobOffset = 0.14F * std::sin(static_cast<float>(m_visualTimeSeconds * 4.5));
    const float rotation = std::fmod(static_cast<float>(m_visualTimeSeconds * 130.0), 360.0F);
    m_services->scene->TrySetEntityTransform(
        m_activePickupEntityId,
        SceneTransform{
            Vector2{m_activePickupBasePosition.x, m_activePickupBasePosition.y + bobOffset},
            rotation,
            Vector2{0.85F, 0.85F}});
}

void VerticalSliceFeatureLayer::UpdateHud()
{
    if (m_services == nullptr)
    {
        return;
    }

    const float timerRatio = ClampFloat(static_cast<float>(m_remainingRoundSeconds / kRoundDurationSeconds), 0.0F, 1.0F);
    const float timerWidth = std::max(0.22F, 6.0F * timerRatio);
    m_services->scene->TrySetEntityTransform(
        m_timerFillEntityId,
        SceneTransform{
            Vector2{-3.0F + timerWidth * 0.5F, 4.75F},
            0.0F,
            Vector2{timerWidth, 0.22F}});

    for (int index = 0; index < kTargetPickupCount; ++index)
    {
        const float scale = index < m_collectedPickups ? 0.48F : 0.40F;
        m_services->scene->TrySetEntityTransform(
            m_scorePipEntityIds[index],
            SceneTransform{
                Vector2{-7.15F + static_cast<float>(index) * 0.72F, 4.72F},
                0.0F,
                Vector2{scale, scale}});
    }

    const float beaconRotation =
        m_roundState == VerticalSliceRoundState::Playing ? std::fmod(static_cast<float>(m_visualTimeSeconds * 55.0), 360.0F) : 0.0F;
    m_services->scene->TrySetEntityTransform(
        m_stateBeaconEntityId,
        SceneTransform{
            Vector2{7.0F, 4.72F},
            beaconRotation,
            Vector2{0.5F, 0.5F}});
}

void VerticalSliceFeatureLayer::SubmitEntitySprite(
    const TickContext& context,
    const EntityId entityId,
    const std::string_view materialName,
    const Color& tint,
    const int sortKey) const
{
    if (entityId == kInvalidEntityId)
    {
        return;
    }

    SceneEntityView entityView;
    if (!context.services.scene->TryGetEntityView(entityId, entityView))
    {
        return;
    }

    context.services.renderer->SubmitSprite(
        Sprite2DSubmission{
            entityId,
            entityView.entityName,
            std::string(materialName),
            entityView.transform.position,
            entityView.transform.rotationDegrees,
            entityView.transform.scale,
            tint,
            sortKey});
}

void VerticalSliceFeatureLayer::DestroyActivePickup()
{
    if (m_services == nullptr || m_activePickupEntityId == kInvalidEntityId)
    {
        return;
    }

    m_services->physics->DestroyBody(m_activePickupEntityId);
    static_cast<void>(m_services->scene->DestroyEntity(m_activePickupEntityId));
    m_activePickupEntityId = kInvalidEntityId;
    m_activePickupRecordId.clear();
}

void VerticalSliceFeatureLayer::DestroyHazards()
{
    if (m_services == nullptr)
    {
        return;
    }

    for (const HazardInstance& hazard : m_hazards)
    {
        m_services->physics->DestroyBody(hazard.entityId);
        static_cast<void>(m_services->scene->DestroyEntity(hazard.entityId));
    }

    m_hazards.clear();
}

void VerticalSliceFeatureLayer::QueueMusicStart()
{
    if (m_services == nullptr)
    {
        return;
    }

    m_services->gameplay->QueueEvent(
        std::string(kAudioGameplayEventCategory),
        std::string(kAudioPlayRequestedEvent),
        BuildGameplayAudioPlayPayload(
            AudioPlaybackRequest{
                AudioPlaybackKind::Music,
                std::string(kRoundMusicAssetName),
                std::string(kRoundMusicChannelName),
                "music",
                true,
                0.42F}));
}

void VerticalSliceFeatureLayer::QueueMusicStop()
{
    if (m_services == nullptr)
    {
        return;
    }

    m_services->gameplay->QueueEvent(
        std::string(kAudioGameplayEventCategory),
        std::string(kAudioStopRequestedEvent),
        BuildGameplayAudioStopPayload(
            AudioStopRequest{
                std::string(kRoundMusicChannelName),
                "music",
                std::string(kRoundMusicAssetName)}));
}

void VerticalSliceFeatureLayer::QueueSfx(const std::string_view assetName, std::string channelName)
{
    if (m_services == nullptr)
    {
        return;
    }

    m_services->gameplay->QueueEvent(
        std::string(kAudioGameplayEventCategory),
        std::string(kAudioPlayRequestedEvent),
        BuildGameplayAudioPlayPayload(
            AudioPlaybackRequest{
                AudioPlaybackKind::Sound,
                std::string(assetName),
                std::move(channelName),
                "sfx",
                false,
                0.88F}));
}

void VerticalSliceFeatureLayer::CacheReports(RuntimeServices& services)
{
    m_latestDebugReport = services.ui->BuildLatestDebugReport();
    m_latestAuthoringContext = services.ai->ExportAuthoringContext();
    m_latestGameplayDigest = services.gameplay->BuildGameplayDigest();
    m_lastCompletedFrame = services.renderer->GetLastCompletedFrame();
}
} // namespace she
