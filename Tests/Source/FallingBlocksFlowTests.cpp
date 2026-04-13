#include <SDL3/SDL_main.h>

#include "FallingBlocks/FallingBlocksFeatureLayer.hpp"

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

bool TestFallingBlocksRespondsToGameplayCommands()
{
    she::ApplicationConfig config;
    config.applicationName = "Falling Blocks Flow Tests";
    config.startWindowHidden = true;
    config.maxFrames = 1;

    she::RuntimeServices services = CreateRuntime();
    InitializeServices(services, config);

    she::FallingBlocksFeatureLayer layer;
    layer.OnAttach(services);

    services.gameplay->BeginFrame(1);
    services.gameplay->FlushCommands();

    const she::FallingBlocksSnapshot initialSnapshot = layer.BuildSnapshot();
    services.gameplay->QueueCommand("FallingBlocksHardDrop", "source=test");
    services.gameplay->FlushCommands();

    const she::FallingBlocksSnapshot droppedSnapshot = layer.BuildSnapshot();
    bool passed = true;
    passed &= Expect(
        droppedSnapshot.score > initialSnapshot.score,
        "Hard-drop command should increase score.");
    passed &= Expect(
        droppedSnapshot.activePiece.kind != she::FallingBlocksPieceKind::None,
        "Hard-drop command should leave a fresh active piece in play.");

    services.gameplay->QueueCommand("RestartFallingBlocksGame", "source=test_restart");
    services.gameplay->FlushCommands();

    const she::FallingBlocksSnapshot restartedSnapshot = layer.BuildSnapshot();
    passed &= Expect(
        restartedSnapshot.phase == she::FallingBlocksGamePhase::Playing,
        "Restart command should restore the playing state.");
    passed &= Expect(restartedSnapshot.score == 0, "Restart command should reset score.");
    passed &= Expect(restartedSnapshot.linesCleared == 0, "Restart command should reset line count.");

    layer.OnDetach(services);
    ShutdownServices(services);
    return passed;
}
} // namespace

int main(int, char**)
{
    return TestFallingBlocksRespondsToGameplayCommands() ? 0 : 1;
}
