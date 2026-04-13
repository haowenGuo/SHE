#include <SDL3/SDL_main.h>

#include "Bootstrap/BootstrapFeatureLayer.hpp"

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

bool TestBootstrapFeatureRegistersAndUsesItsScriptModule()
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

    she::ApplicationConfig config;
    config.applicationName = "Bootstrap Script Registration Tests";
    config.startWindowHidden = true;
    config.maxFrames = 1;

    InitializeServices(services, config);

    she::BootstrapFeatureLayer layer;
    layer.OnAttach(services);

    bool passed = true;
    passed &= Expect(
        services.scripting->HasBinding("gameplay.queue_command"),
        "Bootstrap feature should register the gameplay.queue_command script binding.");
    passed &= Expect(
        services.scripting->HasScriptModule("bootstrap.spawn_rules"),
        "Bootstrap feature should register the bootstrap.spawn_rules module.");
    passed &= Expect(
        services.scripting->GetLoadedModuleCount() == 1,
        "Bootstrap feature should load its script module during OnAttach.");

    const auto modules = services.scripting->ListModules();
    passed &= Expect(modules.size() == 1, "Bootstrap feature should register exactly one script module.");
    if (!modules.empty())
    {
        passed &= Expect(
            modules.front().contract.sourcePath == "Game/Features/Bootstrap/Scripts/spawn_rules.lua",
            "Bootstrap script module should point at the feature-local Lua file.");
        passed &= Expect(
            modules.front().contract.requiredBindings.size() == 2,
            "Bootstrap script module should declare both required host bindings.");
        passed &= Expect(
            modules.front().contract.exportedFunctions.size() == 1 &&
                modules.front().contract.exportedFunctions.front().routedCommandName == "SpawnEncounter",
            "Bootstrap script module should route request_spawn through SpawnEncounter.");
        passed &= Expect(
            modules.front().loadState == she::ScriptModuleLoadState::Loaded,
            "Bootstrap script module should report the Loaded state.");
    }

    std::size_t spawnRequestedEvents = 0;
    std::string lastSpawnPayload;
    const std::size_t spawnSubscriptionId = services.gameplay->SubscribeToEvent(
        "encounter",
        "SpawnRequested",
        [&spawnRequestedEvents, &lastSpawnPayload](const she::GameplayEvent& event)
        {
            ++spawnRequestedEvents;
            lastSpawnPayload = event.payload;
        });

    services.gameplay->BeginFrame(1);
    layer.OnFixedUpdate(she::TickContext{config.fixedTimeStep, 1, true, services});
    services.gameplay->FlushCommands();

    passed &= Expect(
        spawnRequestedEvents == 1,
        "Bootstrap fixed update should invoke the script module and route one SpawnRequested event.");
    passed &= Expect(
        lastSpawnPayload.find("trigger=bootstrap_intro") != std::string::npos,
        "Bootstrap intro spawn should preserve the script-routed payload details.");

    services.gameplay->UnsubscribeFromEvent(spawnSubscriptionId);
    layer.OnDetach(services);
    ShutdownServices(services);
    return passed;
}
} // namespace

int main(int, char**)
{
    return TestBootstrapFeatureRegistersAndUsesItsScriptModule() ? 0 : 1;
}
