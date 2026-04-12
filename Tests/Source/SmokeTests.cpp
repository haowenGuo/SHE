#include "SHE/AI/AuthoringAiService.hpp"
#include "SHE/Assets/AssetManager.hpp"
#include "SHE/Audio/NullAudioService.hpp"
#include "SHE/Core/Application.hpp"
#include "SHE/Data/DataService.hpp"
#include "SHE/Diagnostics/DiagnosticsService.hpp"
#include "SHE/Gameplay/GameplayService.hpp"
#include "SHE/Physics/NullPhysicsService.hpp"
#include "SHE/Platform/NullWindowService.hpp"
#include "SHE/Reflection/ReflectionService.hpp"
#include "SHE/Renderer/NullRendererService.hpp"
#include "SHE/Scene/SceneWorld.hpp"
#include "SHE/Scripting/ScriptingService.hpp"
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

    void OnAttach(she::RuntimeServices& services) override
    {
        ++attachCount;
        services.reflection->RegisterFeature(
            "SmokeTestFeature",
            "Tests/Source",
            "Verifies that AI-native services are wired into the application.");
        services.data->RegisterSchema(
            "tests.smoke.feature",
            "Schema used to confirm that the data service participates in runtime initialization.",
            {"id", "value"});
        services.scripting->RegisterScriptModule(
            "tests.smoke.module",
            "Placeholder module registered by the smoke test.");
        services.gameplay->CreateTimer("tests.smoke.timer", 0.05, false);
        services.gameplay->QueueCommand("SmokeTestCommand", "payload=ok");
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

    void OnDetach(she::RuntimeServices& services) override
    {
        ++detachCount;
        aiContextVersion = services.ai->GetContextVersion();
        aiContextLength = services.ai->ExportAuthoringContext().size();
    }

    int attachCount = 0;
    int detachCount = 0;
    int fixedCount = 0;
    int updateCount = 0;
    int renderCount = 0;
    int uiCount = 0;
    std::size_t aiContextVersion = 0;
    std::size_t aiContextLength = 0;
};

she::RuntimeServices CreateTestRuntime()
{
    she::RuntimeServices services;
    services.window = std::make_shared<she::NullWindowService>();
    services.assets = std::make_shared<she::AssetManager>();
    services.scene = std::make_shared<she::SceneWorld>();
    services.reflection = std::make_shared<she::ReflectionService>();
    services.data = std::make_shared<she::DataService>();
    services.gameplay = std::make_shared<she::GameplayService>();
    services.renderer = std::make_shared<she::NullRendererService>();
    services.physics = std::make_shared<she::NullPhysicsService>();
    services.audio = std::make_shared<she::NullAudioService>();
    services.ui = std::make_shared<she::NullUiService>();
    services.scripting = std::make_shared<she::ScriptingService>();
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
} // namespace

int main()
{
    she::ApplicationConfig config;
    config.applicationName = "SHE Smoke Tests";
    config.maxFrames = 3;

    she::RuntimeServices services = CreateTestRuntime();
    services.ai->SetAuthoringIntent("Smoke test the AI-native runtime stack.");

    she::Application app(config, services);

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

    if (layerPtr->aiContextVersion < 1 || layerPtr->aiContextLength == 0)
    {
        std::cerr << "AI authoring context export was unexpectedly empty.\n";
        return 1;
    }

    return 0;
}
