#include "BootstrapFeatureLayer.hpp"

#include "SHE/Audio/AudioPlaybackContract.hpp"
#include "SHE/Core/Logger.hpp"

namespace she
{
namespace
{
constexpr std::string_view kBootstrapSpawnModuleName = "bootstrap.spawn_rules";

std::string BuildSpawnPayload(const bool fromTimer)
{
    if (fromTimer)
    {
        return "encounter=bootstrap_room_a; enemy=training_dummy; count=1; trigger=spawn_pulse";
    }

    return "encounter=bootstrap_room_a; enemy=training_dummy; count=1; trigger=bootstrap_intro";
}
} // namespace

BootstrapFeatureLayer::BootstrapFeatureLayer() : Layer("BootstrapFeatureLayer")
{
}

void BootstrapFeatureLayer::OnAttach(RuntimeServices& services)
{
    services.ai->SetAuthoringIntent(
        "Implement gameplay as small Game/Features modules that register schemas, script hooks, and gameplay commands.");

    services.scene->SetActiveScene("BootstrapArena");
    services.assets->RegisterLoader(
        AssetLoaderDescriptor{
            "texture_placeholder",
            "texture",
            {".placeholder", ".png"},
            "Bootstrap placeholder loader for the first renderer path."});
    services.assets->RegisterAsset(
        AssetMetadata{
            kInvalidAssetId,
            "textures/player_idle",
            "Game/Features/Bootstrap/Data/player_idle.placeholder",
            "texture",
            "",
            "Game/Features/Bootstrap",
            "feature.bootstrap.player_visual",
            {"bootstrap", "player", "sprite"},
            {
                {"usage", "player"},
                {"variant", "idle"},
            }});
    services.assets->RegisterAsset(
        AssetMetadata{
            kInvalidAssetId,
            "audio/player_fire",
            "Game/Features/Bootstrap/Data/player_fire.wav",
            "audio",
            "",
            "Game/Features/Bootstrap",
            "feature.bootstrap.player_fire_sfx",
            {"bootstrap", "player", "sfx"},
            {
                    {"trigger", "player_fire"},
                    {"usage", "combat"},
                }});
    services.assets->RegisterAsset(
        AssetMetadata{
            kInvalidAssetId,
            "audio/bootstrap_theme",
            "Game/Features/Bootstrap/Data/bootstrap_theme.wav",
            "audio",
            "",
            "Game/Features/Bootstrap",
            "feature.bootstrap.theme_music",
            {"bootstrap", "music"},
            {
                {"loop", "true"},
                {"usage", "ambient"},
            }});
    services.renderer->RegisterMaterial(
        Material2DDescriptor{
            "materials/bootstrap.player",
            "textures/player_idle",
            Color{1.0F, 1.0F, 1.0F, 1.0F}});

    services.reflection->RegisterType("component", "TransformComponent", "Spatial transform for 2D entities.");
    services.reflection->RegisterType("service", "GameplayService", "Central event, command, and timer runtime.");
    services.reflection->RegisterFeature(
        "BootstrapFeature",
        "Game/Features/Bootstrap",
        "Reference feature module that documents the AI-native gameplay workflow.");

    services.data->RegisterSchema(
        DataSchemaContract{
            "feature.bootstrap.enemy",
            "Schema for a simple enemy definition used by the bootstrap feature.",
            "Game/Features/Bootstrap",
            {
                DataSchemaFieldContract{"id", "scalar", true, "Stable enemy identifier."},
                DataSchemaFieldContract{"display_name", "scalar", true, "Human-readable enemy name."},
                DataSchemaFieldContract{"max_health", "scalar", true, "Maximum health value for the enemy."},
                DataSchemaFieldContract{"move_speed", "scalar", true, "Base move speed for the enemy."},
                DataSchemaFieldContract{"abilities", "list", false, "Optional ability identifiers for the enemy."},
            }});

    services.data->RegisterSchema(
        DataSchemaContract{
            "feature.bootstrap.encounter",
            "Schema for a room-sized encounter authored as data instead of hard-coded logic.",
            "Game/Features/Bootstrap",
            {
                DataSchemaFieldContract{"id", "scalar", true, "Stable encounter identifier."},
                DataSchemaFieldContract{"spawn_points", "list", true, "Spawn markers used by the encounter."},
                DataSchemaFieldContract{"enemy_refs", "list", true, "Enemy record references used by the encounter."},
                DataSchemaFieldContract{"reward_id", "scalar", true, "Reward granted after completion."},
                DataSchemaFieldContract{"notes", "scalar", false, "Optional designer-facing notes."},
            }});

    services.scripting->RegisterBinding({
        "gameplay.queue_command",
        "GameplayService",
        "Route script-authored actions through the stable gameplay command path."});
    services.scripting->RegisterBinding({
        "data.describe_registry",
        "DataService",
        "Allow scripts to reason about the trusted authored data catalog without bypassing IDataService."});

    services.scripting->RegisterScriptModule(
        ScriptModuleContract{
            std::string(kBootstrapSpawnModuleName),
            "Game/Features/Bootstrap/Scripts/spawn_rules.lua",
            "lua",
            "Game/Features/Bootstrap",
            "Bootstrap spawn pacing hooks authored against the stable scripting host contract.",
            true,
            {"gameplay.queue_command", "data.describe_registry"},
            {
                ScriptFunctionContract{
                    "request_spawn",
                    "Translate bootstrap spawn rules into the shared SpawnEncounter gameplay command.",
                    "SpawnEncounter"},
            }});

    services.gameplay->RegisterCommand({
        "SpawnEncounter",
        "Route authored encounter spawns through the gameplay command registry.",
        "encounter",
        "SpawnRequested",
    });

    const ScriptModuleLoadResult loadResult = services.scripting->LoadScriptModule(kBootstrapSpawnModuleName);
    m_spawnRulesLoaded = loadResult.loaded;
    if (!m_spawnRulesLoaded)
    {
        SHE_LOG_WARNING("Game", "Bootstrap spawn rules module did not load through the scripting host.");
    }

    m_timerSubscriptionId = services.gameplay->SubscribeToEvent(
        "timer",
        "TimerElapsed",
        [this](const GameplayEvent& event)
        {
            if (event.source == "timer:bootstrap.spawn_pulse")
            {
                ++m_observedSpawnPulseCount;
            }
        });
    m_spawnAudioSubscriptionId = services.gameplay->SubscribeToEvent(
        "encounter",
        "SpawnRequested",
        [gameplay = services.gameplay](const GameplayEvent&)
        {
            gameplay->QueueEvent(
                std::string(kAudioGameplayEventCategory),
                std::string(kAudioPlayRequestedEvent),
                BuildGameplayAudioPlayPayload(
                    AudioPlaybackRequest{
                        AudioPlaybackKind::Sound,
                        "audio/player_fire",
                        "sfx/player_fire",
                        "sfx",
                        false,
                        0.85F}));
        });

    services.gameplay->CreateTimer("bootstrap.spawn_pulse", 0.05, true);
    services.gameplay->QueueEvent("feature", "BootstrapAttached", "Bootstrap feature registered its contracts.");
    services.gameplay->QueueEvent(
        std::string(kAudioGameplayEventCategory),
        std::string(kAudioPlayRequestedEvent),
        BuildGameplayAudioPlayPayload(
            AudioPlaybackRequest{
                AudioPlaybackKind::Music,
                "audio/bootstrap_theme",
                "music/bootstrap",
                "music",
                true,
                0.55F}));

    m_playerEntityId = services.scene->CreateEntity("Player");
    m_cameraEntityId = services.scene->CreateEntity("MainCamera");
    services.scene->TrySetEntityTransform(
        m_playerEntityId,
        SceneTransform{
            Vector2{2.0F, 1.0F},
            0.0F,
            Vector2{2.0F, 2.0F}});
    services.scene->TrySetEntityTransform(
        m_cameraEntityId,
        SceneTransform{
            Vector2{0.0F, 0.0F},
            0.0F,
            Vector2{1.0F, 1.0F}});

    SHE_LOG_INFO("Game", "Bootstrap feature layer attached.");
}

void BootstrapFeatureLayer::OnFixedUpdate(const TickContext& context)
{
    if (!m_spawnRulesLoaded || m_queuedFirstSpawnCommand)
    {
        return;
    }

    m_queuedFirstSpawnCommand = true;
    const ScriptInvocationResult result = context.services.scripting->InvokeScriptFunction(
        kBootstrapSpawnModuleName,
        "request_spawn",
        BuildSpawnPayload(false));
    if (!result.accepted)
    {
        SHE_LOG_WARNING("Game", "Bootstrap intro spawn could not be routed through the scripting host.");
    }
}

void BootstrapFeatureLayer::OnUpdate(const TickContext& context)
{
    if (!m_loggedFeatureSummary)
    {
        m_loggedFeatureSummary = true;
        SHE_LOG_INFO(
            "Game",
            "Bootstrap feature reached the first update. AI-native services are now feeding reflection, schema, and gameplay context.");
    }

    if (context.frameIndex == 1)
    {
        SHE_LOG_INFO("Game", "Authoring context version advanced during bootstrap gameplay.");
    }

    if (m_playerEntityId == kInvalidEntityId)
    {
        SHE_LOG_WARNING("Game", "Bootstrap feature failed to create the player entity.");
    }

    if (m_observedSpawnPulseCount > m_handledSpawnPulseCount)
    {
        m_handledSpawnPulseCount = m_observedSpawnPulseCount;
        const ScriptInvocationResult result = context.services.scripting->InvokeScriptFunction(
            kBootstrapSpawnModuleName,
            "request_spawn",
            BuildSpawnPayload(true));
        if (!result.accepted)
        {
            SHE_LOG_WARNING("Game", "Bootstrap timer-driven spawn could not be routed through the scripting host.");
        }
    }
}

void BootstrapFeatureLayer::OnRender(const TickContext& context)
{
    SceneEntityView playerView;
    SceneEntityView cameraView;
    if (!context.services.scene->TryGetEntityView(m_playerEntityId, playerView) ||
        !context.services.scene->TryGetEntityView(m_cameraEntityId, cameraView))
    {
        return;
    }

    const WindowState windowState = context.services.window->GetWindowState();
    context.services.renderer->SubmitCamera(
        Camera2DSubmission{
            m_cameraEntityId,
            cameraView.entityName,
            cameraView.transform.position,
            1.0F,
            windowState.width,
            windowState.height,
            Color{0.08F, 0.10F, 0.14F, 1.0F}});
    context.services.renderer->SubmitSprite(
        Sprite2DSubmission{
            m_playerEntityId,
            playerView.entityName,
            "materials/bootstrap.player",
            playerView.transform.position,
            playerView.transform.rotationDegrees,
            playerView.transform.scale,
            Color{1.0F, 1.0F, 1.0F, 1.0F},
            0});
}

void BootstrapFeatureLayer::OnDetach(RuntimeServices& services)
{
    if (m_spawnAudioSubscriptionId != 0)
    {
        services.gameplay->UnsubscribeFromEvent(m_spawnAudioSubscriptionId);
        m_spawnAudioSubscriptionId = 0;
    }

    if (m_timerSubscriptionId != 0)
    {
        services.gameplay->UnsubscribeFromEvent(m_timerSubscriptionId);
        m_timerSubscriptionId = 0;
    }

    m_spawnRulesLoaded = false;

    SHE_LOG_INFO("Game", "Bootstrap feature layer detached.");
}
} // namespace she

