#include "SHE/Assets/AssetManager.hpp"
#include "SHE/Audio/NullAudioService.hpp"
#include "SHE/Core/Application.hpp"
#include "SHE/Physics/NullPhysicsService.hpp"
#include "SHE/Platform/NullWindowService.hpp"
#include "SHE/Renderer/NullRendererService.hpp"
#include "SHE/Scene/SceneWorld.hpp"
#include "SHE/UI/NullUiService.hpp"

#include <iostream>
#include <memory>

namespace
{
class CountingLayer final : public she::Layer
{
public:
    CountingLayer() : she::Layer("CountingLayer")
    {
    }

    void OnAttach(she::RuntimeServices&) override
    {
        ++attachCount;
    }

    void OnDetach(she::RuntimeServices&) override
    {
        ++detachCount;
    }

    void OnFixedUpdate(const she::TickContext&) override
    {
        ++fixedCount;
    }

    void OnUpdate(const she::TickContext&) override
    {
        ++updateCount;
    }

    void OnRender(const she::TickContext&) override
    {
        ++renderCount;
    }

    void OnUi(const she::TickContext&) override
    {
        ++uiCount;
    }

    int attachCount = 0;
    int detachCount = 0;
    int fixedCount = 0;
    int updateCount = 0;
    int renderCount = 0;
    int uiCount = 0;
};

she::RuntimeServices CreateTestRuntime()
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
    config.applicationName = "SHE Smoke Tests";
    config.maxFrames = 3;

    she::Application app(config, CreateTestRuntime());

    auto layer = std::make_unique<CountingLayer>();
    CountingLayer* layerPtr = layer.get();
    app.PushLayer(std::move(layer));

    const int result = app.Run();

    if (result != 0)
    {
        std::cerr << "Application returned non-zero from smoke test.\n";
        return 1;
    }

    if (layerPtr->attachCount != 1 || layerPtr->detachCount != 1)
    {
        std::cerr << "Layer attach/detach counts were unexpected.\n";
        return 1;
    }

    if (layerPtr->fixedCount < 1 || layerPtr->updateCount < 1 || layerPtr->renderCount < 1 || layerPtr->uiCount < 1)
    {
        std::cerr << "Layer did not observe all expected runtime phases.\n";
        return 1;
    }

    return 0;
}
