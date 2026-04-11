#include "BootstrapLayer.hpp"

#include "SHE/Assets/AssetManager.hpp"
#include "SHE/Audio/NullAudioService.hpp"
#include "SHE/Core/Application.hpp"
#include "SHE/Physics/NullPhysicsService.hpp"
#include "SHE/Platform/NullWindowService.hpp"
#include "SHE/Renderer/NullRendererService.hpp"
#include "SHE/Scene/SceneWorld.hpp"
#include "SHE/UI/NullUiService.hpp"

#include <memory>

namespace
{
she::RuntimeServices CreateBootstrapRuntime()
{
    she::RuntimeServices services;
    services.window = std::make_shared<she::NullWindowService>();
    services.assets = std::make_shared<she::AssetManager>();
    services.scene = std::make_shared<she::SceneWorld>();
    services.renderer = std::make_shared<she::NullRendererService>();
    services.physics = std::make_shared<she::NullPhysicsService>();
    services.audio = std::make_shared<she::NullAudioService>();
    services.ui = std::make_shared<she::NullUiService>();
    return services;
}
} // namespace

int main()
{
    she::ApplicationConfig config;
    config.applicationName = "SHE Game";
    config.maxFrames = 4;

    she::Application app(config, CreateBootstrapRuntime());
    app.PushLayer(std::make_unique<she::BootstrapLayer>());
    return app.Run();
}

