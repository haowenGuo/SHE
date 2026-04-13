#include "BootstrapFeatureLayer.hpp"

#include "SHE/Core/Logger.hpp"

namespace she
{
BootstrapFeatureLayer::BootstrapFeatureLayer() : Layer("BootstrapFeatureLayer")
{
}

void BootstrapFeatureLayer::OnAttach(RuntimeServices& services)
{
    services.ai->SetAuthoringIntent(
        "Implement gameplay as small Game/Features modules that register schemas, script hooks, and gameplay commands.");

    services.scene->SetActiveScene("BootstrapArena");
    services.assets->RegisterAsset(
        "textures/player_idle",
        "Game/Features/Bootstrap/Data/player_idle.placeholder");
    services.assets->RegisterAsset(
        "audio/player_fire",
        "Game/Features/Bootstrap/Data/player_fire.placeholder");

    services.reflection->RegisterType("component", "TransformComponent", "Spatial transform for 2D entities.");
    services.reflection->RegisterType("service", "GameplayService", "Central event, command, and timer runtime.");
    services.reflection->RegisterFeature(
        "BootstrapFeature",
        "Game/Features/Bootstrap",
        "Reference feature module that documents the AI-native gameplay workflow.");

    services.data->RegisterSchema(
        "feature.bootstrap.enemy",
        "Schema for a simple enemy definition used by the bootstrap feature.",
        {"id", "display_name", "max_health", "move_speed"});

    services.data->RegisterSchema(
        "feature.bootstrap.encounter",
        "Schema for a room-sized encounter authored as data instead of hard-coded logic.",
        {"id", "spawn_points", "enemy_refs", "reward_id"});

    services.scripting->RegisterScriptModule(
        "bootstrap.spawn_rules",
        "Future Lua module for spawn pacing and wave customization.");

    services.gameplay->RegisterCommand({
        "SpawnEncounter",
        "Route authored encounter spawns through the gameplay command registry.",
        "encounter",
        "SpawnRequested",
    });

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

    services.gameplay->CreateTimer("bootstrap.spawn_pulse", 0.05, true);
    services.gameplay->QueueEvent("feature", "BootstrapAttached", "Bootstrap feature registered its contracts.");

    m_playerEntityId = services.scene->CreateEntity("Player");
    services.scene->CreateEntity("MainCamera");

    SHE_LOG_INFO("Game", "Bootstrap feature layer attached.");
}

void BootstrapFeatureLayer::OnFixedUpdate(const TickContext& context)
{
    if (!m_queuedFirstSpawnCommand)
    {
        m_queuedFirstSpawnCommand = true;
        context.services.gameplay->QueueCommand(
            "SpawnEncounter",
            "encounter=bootstrap_room_a; enemy=training_dummy; count=1");
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
        context.services.gameplay->QueueCommand(
            "SpawnEncounter",
            "encounter=bootstrap_room_a; enemy=training_dummy; count=1; trigger=spawn_pulse");
    }
}

void BootstrapFeatureLayer::OnDetach(RuntimeServices& services)
{
    if (m_timerSubscriptionId != 0)
    {
        services.gameplay->UnsubscribeFromEvent(m_timerSubscriptionId);
        m_timerSubscriptionId = 0;
    }

    SHE_LOG_INFO("Game", "Bootstrap feature layer detached.");
}
} // namespace she

