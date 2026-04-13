#include <SDL3/SDL_main.h>

#include "RogueLite/RogueLiteFeatureLayer.hpp"

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
#include <string>
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

bool TestRogueLiteRunsAsPlayableFeature()
{
    she::ApplicationConfig config;
    config.applicationName = "RogueLite Smoke Tests";
    config.useDeterministicFrameTiming = true;
    config.maxFrames = 40;
    config.startWindowHidden = true;

    she::Application app(config, CreateRuntime());

    auto layer = std::make_unique<she::RogueLiteFeatureLayer>();
    she::RogueLiteFeatureLayer* layerPtr = layer.get();
    app.PushLayer(std::move(layer));

    const int result = app.Run();
    if (result != 0)
    {
        std::cerr << "Application returned a non-zero code for the roguelite smoke test.\n";
        return false;
    }

    const she::RogueLiteSnapshot snapshot = layerPtr->BuildSnapshot();
    bool passed = true;
    passed &= Expect(
        snapshot.phase == she::RogueLiteGamePhase::Combat,
        "Early roguelite smoke frames should still be in combat.");
    passed &= Expect(snapshot.roomIndex == 0, "Smoke run should still be in room one.");
    passed &= Expect(snapshot.playerHitPoints > 0, "Smoke run should keep the player alive.");
    passed &= Expect(snapshot.enemiesRemaining > 0, "Smoke run should still have live enemies in the room.");

    const auto& lastFrame = layerPtr->GetLastCompletedFrame();
    passed &= Expect(lastFrame.has_value(), "RogueLite feature should complete at least one render frame.");
    if (lastFrame.has_value())
    {
        passed &= Expect(lastFrame->sceneName == "RogueLiteRooms", "Rendered scene name should match the roguelite rooms.");
        passed &= Expect(lastFrame->sprites.size() > 20, "Rendered roguelite frame should include room and HUD sprites.");
        passed &= Expect(lastFrame->visiblePixelSampleCount > 0, "Rendered roguelite frame should produce visible pixels.");
    }

    const std::string& debugReport = layerPtr->GetLatestDebugReport();
    passed &= Expect(
        debugReport.find("active_scene: RogueLiteRooms") != std::string::npos,
        "UI debug report should reference the roguelite scene.");

    const std::string& gameplayDigest = layerPtr->GetLatestGameplayDigest();
    passed &= Expect(
        gameplayDigest.find("registered_commands=6") != std::string::npos,
        "Gameplay digest should report the six roguelite commands.");

    const std::string& authoringContext = layerPtr->GetLatestAuthoringContext();
    passed &= Expect(
        authoringContext.find("RogueLiteFeature") != std::string::npos,
        "Authoring context should mention the roguelite feature.");
    passed &= Expect(
        authoringContext.find("feature.roguelite.room.01") != std::string::npos,
        "Authoring context should include roguelite room data.");

    return passed;
}
} // namespace

int main(int, char**)
{
    return TestRogueLiteRunsAsPlayableFeature() ? 0 : 1;
}
