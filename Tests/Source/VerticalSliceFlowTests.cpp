#include <SDL3/SDL_main.h>

#include "VerticalSlice/VerticalSliceFeatureLayer.hpp"

#include "SHE/AI/AuthoringAiService.hpp"
#include "SHE/Assets/AssetManager.hpp"
#include "SHE/Audio/MiniaudioAudioService.hpp"
#include "SHE/Core/ApplicationConfig.hpp"
#include "SHE/Data/DataService.hpp"
#include "SHE/Diagnostics/DiagnosticsService.hpp"
#include "SHE/Gameplay/GameplayService.hpp"
#include "SHE/Physics/Box2DPhysicsService.hpp"
#include "SHE/Platform/SdlWindowService.hpp"
#include "SHE/Reflection/ReflectionService.hpp"
#include "SHE/Renderer/Renderer2DService.hpp"
#include "SHE/Scene/SceneWorld.hpp"
#include "SHE/Scripting/ScriptingService.hpp"
#include "SHE/UI/NullUiService.hpp"

#include <iostream>
#include <memory>
#include <string_view>

namespace
{
bool Expect(const bool condition, const std::string_view message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        return false;
    }

    return true;
}

she::RuntimeServices CreateRuntime()
{
    she::RuntimeServices services;
    services.window = std::make_shared<she::SdlWindowService>();
    services.assets = std::make_shared<she::AssetManager>();
    services.scene = std::make_shared<she::SceneWorld>();
    services.reflection = std::make_shared<she::ReflectionService>();
    services.data = std::make_shared<she::DataService>();
    services.gameplay = std::make_shared<she::GameplayService>();
    services.renderer = std::make_shared<she::Renderer2DService>(services.assets, services.window);
    services.physics = std::make_shared<she::Box2DPhysicsService>(services.scene, services.gameplay);
    she::AudioRuntimeOptions audioOptions;
    audioOptions.preferPlaybackDevice = false;
    services.audio = std::make_shared<she::MiniaudioAudioService>(services.assets, services.gameplay, audioOptions);
    services.ui = std::make_shared<she::NullUiService>();
    services.scripting = std::make_shared<she::ScriptingService>(services.gameplay);
    services.diagnostics = std::make_shared<she::DiagnosticsService>();
    services.ai = std::make_shared<she::AuthoringAiService>(
        services.reflection,
        services.data,
        services.gameplay,
        services.scripting,
        services.scene,
        services.assets,
        services.diagnostics);
    return services;
}

void InitializeServices(she::RuntimeServices& services, const she::ApplicationConfig& config)
{
    services.window->Initialize(config);
    services.assets->Initialize();
    services.scene->Initialize();
    services.reflection->Initialize();
    services.data->Initialize();
    services.gameplay->Initialize();
    services.renderer->Initialize(config);
    services.physics->Initialize();
    services.audio->Initialize();
    services.ui->Initialize();
    services.scripting->Initialize();
    services.diagnostics->Initialize();
    services.ai->Initialize();
}

void ShutdownServices(she::RuntimeServices& services)
{
    services.ai->Shutdown();
    services.diagnostics->Shutdown();
    services.scripting->Shutdown();
    services.ui->Shutdown();
    services.audio->Shutdown();
    services.physics->Shutdown();
    services.renderer->Shutdown();
    services.gameplay->Shutdown();
    services.data->Shutdown();
    services.reflection->Shutdown();
    services.scene->Shutdown();
    services.assets->Shutdown();
    services.window->Shutdown();
}

std::string BuildCollisionPayload(const she::EntityId entityA, const she::EntityId entityB)
{
    return "entity_a=" + std::to_string(entityA) + "; entity_name_a=test; collider_a=1; sensor_a=false; entity_b=" +
           std::to_string(entityB) + "; entity_name_b=test; collider_b=2; sensor_b=true";
}

bool TestVerticalSliceSupportsWinLoseAndRestart()
{
    she::ApplicationConfig config;
    config.applicationName = "Vertical Slice Flow Tests";
    config.startWindowHidden = true;
    config.maxFrames = 1;

    she::RuntimeServices services = CreateRuntime();
    InitializeServices(services, config);

    she::VerticalSliceFeatureLayer layer;
    layer.OnAttach(services);

    bool passed = true;
    services.gameplay->BeginFrame(1);
    services.gameplay->FlushCommands();

    she::VerticalSliceSnapshot snapshot = layer.BuildSnapshot();
    passed &= Expect(
        snapshot.roundState == she::VerticalSliceRoundState::Playing,
        "Vertical slice should be in the playing state after initial command flush.");
    passed &= Expect(snapshot.activePickupEntityId != she::kInvalidEntityId, "Initial round should spawn a pickup.");
    passed &= Expect(snapshot.activeHazardCount >= 1, "Initial round should spawn at least one hazard.");

    for (int pickupIndex = 0; pickupIndex < 3; ++pickupIndex)
    {
        snapshot = layer.BuildSnapshot();
        services.gameplay->QueueEvent(
            "physics",
            "CollisionStarted",
            BuildCollisionPayload(snapshot.playerEntityId, snapshot.activePickupEntityId));
        services.gameplay->FlushCommands();
        layer.OnUpdate(she::TickContext{config.fixedTimeStep, static_cast<she::FrameIndex>(pickupIndex + 2), false, services});
        services.gameplay->FlushCommands();
    }

    snapshot = layer.BuildSnapshot();
    passed &= Expect(snapshot.roundState == she::VerticalSliceRoundState::Won, "Collecting all pickups should win the round.");
    passed &= Expect(snapshot.collectedPickups == 3, "Winning the round should collect all three pickups.");

    services.gameplay->QueueCommand("RestartVerticalSliceRound", "reason=test_restart");
    services.gameplay->QueueCommand(
        "SpawnVerticalSlicePickup",
        "record_id=feature.vertical_slice.pickup.alpha; x=-5.8; y=2.3");
    services.gameplay->QueueCommand(
        "SpawnVerticalSliceWave",
        "record_id=feature.vertical_slice.wave.opening; entity_name=SweepDrone; x=-6.5; y=2.7; velocity_x=3.35; "
        "velocity_y=0.0; size_x=1.0; size_y=1.0");
    services.gameplay->FlushCommands();

    snapshot = layer.BuildSnapshot();
    passed &= Expect(
        snapshot.roundState == she::VerticalSliceRoundState::Playing,
        "Restart command should return the slice to the playing state.");
    passed &= Expect(snapshot.collectedPickups == 0, "Restart should clear collected pickups.");
    passed &= Expect(snapshot.activePickupEntityId != she::kInvalidEntityId, "Restart should respawn a pickup.");

    services.gameplay->QueueEvent(
        "physics",
        "CollisionStarted",
        BuildCollisionPayload(snapshot.playerEntityId, snapshot.hazardEntityIds.front()));
    services.gameplay->FlushCommands();

    snapshot = layer.BuildSnapshot();
    passed &= Expect(snapshot.roundState == she::VerticalSliceRoundState::Lost, "Player-hazard contact should lose the round.");
    passed &= Expect(
        snapshot.latestStatus.find("lost") != std::string::npos,
        "Losing the round should publish a loss-facing status string.");

    layer.OnDetach(services);
    ShutdownServices(services);
    return passed;
}
} // namespace

int main(int, char**)
{
    return TestVerticalSliceSupportsWinLoseAndRestart() ? 0 : 1;
}
