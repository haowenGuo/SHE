#include <SDL3/SDL_main.h>

#include "FallingBlocks/FallingBlocksFeatureLayer.hpp"

#include "SHE/AI/AuthoringAiService.hpp"
#include "SHE/Assets/AssetManager.hpp"
#include "SHE/Audio/MiniaudioAudioService.hpp"
#include "SHE/Core/Application.hpp"
#include "SHE/Data/DataService.hpp"
#include "SHE/Diagnostics/DiagnosticsService.hpp"
#include "SHE/Gameplay/GameplayService.hpp"
#include "SHE/Physics/Box2DPhysicsService.hpp"
#include "SHE/Platform/SdlWindowService.hpp"
#include "SHE/Reflection/ReflectionService.hpp"
#include "SHE/Renderer/Renderer2DService.hpp"
#include "SHE/Scene/SceneWorld.hpp"
#include "SHE/Scripting/ScriptingService.hpp"
#include "SHE/UI/DebugUiService.hpp"

#include <memory>

namespace
{
she::RuntimeServices CreateBootstrapRuntime()
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
    services.audio = std::make_shared<she::MiniaudioAudioService>(services.assets, services.gameplay);
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
    services.ui = std::make_shared<she::DebugUiService>(
        services.window,
        services.assets,
        services.scene,
        services.data,
        services.gameplay,
        services.renderer,
        services.physics,
        services.diagnostics,
        services.ai);
    return services;
}
} // namespace

int main(int, char**)
{
    she::ApplicationConfig config;
    config.applicationName = "SHE Falling Blocks";
    config.useDeterministicFrameTiming = false;
    config.maxFrames = 0;

    she::Application app(config, CreateBootstrapRuntime());
    app.PushLayer(std::make_unique<she::FallingBlocksFeatureLayer>());
    return app.Run();
}
