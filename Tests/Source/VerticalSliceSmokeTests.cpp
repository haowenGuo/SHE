#include <SDL3/SDL_main.h>

#include "VerticalSlice/VerticalSliceFeatureLayer.hpp"

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

bool TestVerticalSliceRunsAsAPlayableFeature()
{
    she::ApplicationConfig config;
    config.applicationName = "Vertical Slice Smoke Tests";
    config.useDeterministicFrameTiming = true;
    config.maxFrames = 80;
    config.startWindowHidden = true;

    she::Application app(config, CreateRuntime());

    auto layer = std::make_unique<she::VerticalSliceFeatureLayer>();
    she::VerticalSliceFeatureLayer* layerPtr = layer.get();
    app.PushLayer(std::move(layer));

    const int result = app.Run();
    if (result != 0)
    {
        std::cerr << "Application returned a non-zero code for the vertical slice smoke test.\n";
        return false;
    }

    const she::VerticalSliceSnapshot snapshot = layerPtr->BuildSnapshot();
    bool passed = true;
    passed &= Expect(snapshot.scriptModuleLoaded, "Vertical slice should load its feature-local script module.");
    passed &= Expect(snapshot.roundStarted, "Vertical slice should start the round automatically.");
    passed &= Expect(
        snapshot.roundState == she::VerticalSliceRoundState::Playing,
        "Vertical slice smoke run should still be in the playing state after a few headless frames.");
    passed &= Expect(snapshot.activePickupEntityId != she::kInvalidEntityId, "Vertical slice should spawn an active pickup.");
    passed &= Expect(snapshot.activeHazardCount >= 1, "Vertical slice should spawn at least one hazard.");
    passed &= Expect(
        snapshot.remainingSeconds < 24.0 && snapshot.remainingSeconds > 0.0,
        "Vertical slice round clock should advance during the smoke run.");

    const auto& lastFrame = layerPtr->GetLastCompletedFrame();
    passed &= Expect(lastFrame.has_value(), "Vertical slice should complete at least one render frame.");
    if (lastFrame.has_value())
    {
        passed &= Expect(lastFrame->sceneName == "VerticalSliceArena", "Rendered scene name should match the slice arena.");
        passed &= Expect(lastFrame->sprites.size() >= 6, "Rendered slice frame should include world and HUD sprites.");
        passed &= Expect(lastFrame->visiblePixelSampleCount > 0, "Rendered slice frame should produce visible pixels.");
    }

    const std::string& debugReport = layerPtr->GetLatestDebugReport();
    passed &= Expect(
        debugReport.find("active_scene: VerticalSliceArena") != std::string::npos,
        "UI debug report should reference the vertical slice arena.");
    passed &= Expect(
        debugReport.find("entity_count:") != std::string::npos,
        "UI debug report should include the scene entity count.");

    const std::string& gameplayDigest = layerPtr->GetLatestGameplayDigest();
    passed &= Expect(
        gameplayDigest.find("registered_commands=4") != std::string::npos,
        "Gameplay digest should report the four registered slice commands.");
    passed &= Expect(
        gameplayDigest.find("timers=2") != std::string::npos,
        "Gameplay digest should report the active slice timers.");

    const std::string& authoringContext = layerPtr->GetLatestAuthoringContext();
    passed &= Expect(
        authoringContext.find("VerticalSliceFeature") != std::string::npos,
        "Authoring context should mention the vertical slice feature.");
    passed &= Expect(
        authoringContext.find("feature.vertical_slice.pickup.alpha") != std::string::npos,
        "Authoring context should include vertical slice pickup records.");
    passed &= Expect(
        authoringContext.find("vertical_slice.arena_flow") != std::string::npos,
        "Authoring context should include the vertical slice script module.");

    return passed;
}
} // namespace

int main(int, char**)
{
    return TestVerticalSliceRunsAsAPlayableFeature() ? 0 : 1;
}
