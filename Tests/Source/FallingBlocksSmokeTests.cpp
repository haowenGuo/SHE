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

bool TestFallingBlocksRunsAsPlayableFeature()
{
    she::ApplicationConfig config;
    config.applicationName = "Falling Blocks Smoke Tests";
    config.useDeterministicFrameTiming = true;
    config.maxFrames = 30;
    config.startWindowHidden = true;

    she::Application app(config, CreateRuntime());

    auto layer = std::make_unique<she::FallingBlocksFeatureLayer>();
    she::FallingBlocksFeatureLayer* layerPtr = layer.get();
    app.PushLayer(std::move(layer));

    const int result = app.Run();
    if (result != 0)
    {
        std::cerr << "Application returned a non-zero code for the falling-block smoke test.\n";
        return false;
    }

    const she::FallingBlocksSnapshot snapshot = layerPtr->BuildSnapshot();
    bool passed = true;
    passed &= Expect(
        snapshot.phase == she::FallingBlocksGamePhase::Playing,
        "Falling-block smoke run should still be in the playing state.");
    passed &= Expect(
        snapshot.activePiece.kind != she::FallingBlocksPieceKind::None,
        "Falling-block smoke run should keep an active piece.");
    passed &= Expect(
        snapshot.nextPiece != she::FallingBlocksPieceKind::None,
        "Falling-block smoke run should expose the next piece preview.");
    passed &= Expect(snapshot.score >= 0, "Falling-block smoke run should expose score.");
    passed &= Expect(snapshot.linesCleared == 0, "Early smoke frames should not have cleared lines yet.");

    const auto& lastFrame = layerPtr->GetLastCompletedFrame();
    passed &= Expect(lastFrame.has_value(), "Falling-block feature should complete at least one render frame.");
    if (lastFrame.has_value())
    {
        passed &= Expect(lastFrame->sceneName == "FallingBlocksBoard", "Rendered scene name should match the falling-block board.");
        passed &= Expect(lastFrame->sprites.size() > 40, "Rendered frame should include board and HUD sprites.");
        passed &= Expect(lastFrame->visiblePixelSampleCount > 0, "Rendered frame should produce visible pixels.");
    }

    const std::string& debugReport = layerPtr->GetLatestDebugReport();
    passed &= Expect(
        debugReport.find("active_scene: FallingBlocksBoard") != std::string::npos,
        "UI debug report should reference the falling-block scene.");

    const std::string& gameplayDigest = layerPtr->GetLatestGameplayDigest();
    passed &= Expect(
        gameplayDigest.find("registered_commands=7") != std::string::npos,
        "Gameplay digest should report the seven falling-block commands.");

    const std::string& authoringContext = layerPtr->GetLatestAuthoringContext();
    passed &= Expect(
        authoringContext.find("FallingBlocksFeature") != std::string::npos,
        "Authoring context should mention the falling-block feature.");
    passed &= Expect(
        authoringContext.find("textures/falling_blocks/piece_i") != std::string::npos,
        "Authoring context should include feature-owned falling-block assets.");

    return passed;
}
} // namespace

int main(int, char**)
{
    return TestFallingBlocksRunsAsPlayableFeature() ? 0 : 1;
}
