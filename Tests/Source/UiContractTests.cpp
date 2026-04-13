#include <SDL3/SDL_main.h>

#include "SHE/AI/AuthoringAiService.hpp"
#include "SHE/Assets/AssetManager.hpp"
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
    }

    return condition;
}

bool Contains(const std::string& value, const std::string_view needle)
{
    return value.find(needle) != std::string::npos;
}

bool TestDebugUiBuildsIntegratedRuntimeReport()
{
    she::ApplicationConfig config;
    config.applicationName = "SHE UI Contract Tests";
    config.windowWidth = 960;
    config.windowHeight = 540;
    config.startWindowHidden = true;

    auto window = std::make_shared<she::SdlWindowService>();
    auto assets = std::make_shared<she::AssetManager>();
    auto scene = std::make_shared<she::SceneWorld>();
    auto reflection = std::make_shared<she::ReflectionService>();
    auto data = std::make_shared<she::DataService>();
    auto gameplay = std::make_shared<she::GameplayService>();
    auto renderer = std::make_shared<she::Renderer2DService>(assets, window);
    auto physics = std::make_shared<she::Box2DPhysicsService>(scene, gameplay);
    auto scripting = std::make_shared<she::ScriptingService>();
    auto diagnostics = std::make_shared<she::DiagnosticsService>();
    auto ai = std::make_shared<she::AuthoringAiService>(
        reflection,
        data,
        gameplay,
        scripting,
        scene,
        assets,
        diagnostics);
    auto ui = std::make_shared<she::DebugUiService>(
        window,
        assets,
        scene,
        data,
        gameplay,
        renderer,
        physics,
        diagnostics,
        ai);

    window->Initialize(config);
    assets->Initialize();
    scene->Initialize();
    reflection->Initialize();
    data->Initialize();
    gameplay->Initialize();
    renderer->Initialize(config);
    physics->Initialize();
    scripting->Initialize();
    diagnostics->Initialize();
    ai->Initialize();
    ui->Initialize();

    assets->RegisterLoader(
        she::AssetLoaderDescriptor{
            "texture_placeholder",
            "texture",
            {".placeholder", ".png"},
            "UI contract placeholder texture loader."});
    const she::AssetId playerTextureId = assets->RegisterAsset(
        she::AssetMetadata{
            she::kInvalidAssetId,
            "textures/ui_contract_player",
            "Tests/Data/ui_contract_player.placeholder",
            "texture",
            "",
            "Tests/Source",
            "tests.ui.player_visual",
            {"ui", "player"},
            {
                {"usage", "ui_contract"},
            }});

    scene->SetActiveScene("UiContractScene");
    const she::EntityId cameraId = scene->CreateEntity("UiCamera");
    const she::EntityId floorId = scene->CreateEntity("UiFloor");
    const she::EntityId playerId = scene->CreateEntity("UiPlayer");
    const bool wroteCameraTransform = scene->TrySetEntityTransform(
        cameraId,
        she::SceneTransform{
            she::Vector2{0.0F, 0.0F},
            0.0F,
            she::Vector2{1.0F, 1.0F}});
    const bool wroteFloorTransform = scene->TrySetEntityTransform(
        floorId,
        she::SceneTransform{
            she::Vector2{0.0F, -1.0F},
            0.0F,
            she::Vector2{10.0F, 1.0F}});
    const bool wrotePlayerTransform = scene->TrySetEntityTransform(
        playerId,
        she::SceneTransform{
            she::Vector2{1.0F, 2.5F},
            5.0F,
            she::Vector2{1.5F, 1.5F}});

    reflection->RegisterFeature(
        "UiContractFeature",
        "Tests/Source",
        "Seeds the runtime with a feature visible to authoring and debug tools.");
    scripting->RegisterScriptModule(
        she::ScriptModuleContract{
            "tests.ui_contract",
            "Tests/Source/Scripts/tests_smoke_module.lua",
            "lua",
            "Tests/Source",
            "Placeholder scripting module for the UI contract report.",
            false,
            {},
            {}});

    const she::DataSchemaRegistrationResult schemaResult = data->RegisterSchema(
        she::DataSchemaContract{
            "tests.ui.panel",
            "Schema for a runtime inspection panel.",
            "Tests/Source",
            {
                she::DataSchemaFieldContract{"id", "scalar", true, "Stable panel identifier."},
                she::DataSchemaFieldContract{"title", "scalar", true, "Human-readable panel title."},
            }});
    const she::DataLoadResult recordResult = data->LoadRecord(
        she::DataLoadRequest{
            "tests.ui.panel",
            "tests.ui.panel.runtime",
            "Tests/Data/ui_panel.yml",
            "id: tests.ui.panel.runtime\n"
            "title: Runtime Overview\n"});

    gameplay->RegisterCommand(
        {"InspectRuntime", "Push a debug inspection command through gameplay.", "tests", "InspectRuntimeRequested"});
    gameplay->CreateTimer("tests.ui.timer", 0.25, true);
    gameplay->BeginFrame(0);
    gameplay->QueueCommand("InspectRuntime", "panel=renderer");
    gameplay->AdvanceFrame(1.0 / 60.0);
    gameplay->FlushCommands();

    const bool createdFloorBody = physics->CreateBody(floorId, she::PhysicsBodyDefinition{she::PhysicsBodyType::Static});
    const bool createdPlayerBody = physics->CreateBody(
        playerId,
        she::PhysicsBodyDefinition{
            she::PhysicsBodyType::Dynamic,
            she::Vector2{},
            0.0F,
            0.0F,
            0.0F,
            1.0F,
            false,
            false,
            true});
    const she::PhysicsColliderId floorColliderId = physics->CreateBoxCollider(
        floorId,
        she::PhysicsBoxColliderDefinition{
            she::Vector2{5.0F, 0.5F},
            she::Vector2{0.0F, 0.0F},
            0.0F,
            0.0F,
            0.4F,
            0.0F,
            false});
    const she::PhysicsColliderId playerColliderId = physics->CreateBoxCollider(
        playerId,
        she::PhysicsBoxColliderDefinition{
            she::Vector2{0.5F, 0.5F},
            she::Vector2{0.0F, 0.0F},
            0.0F,
            1.0F,
            0.2F,
            0.0F,
            false});
    physics->Step(1.0 / 60.0);

    renderer->RegisterMaterial(
        she::Material2DDescriptor{
            "materials/ui_contract.player",
            "textures/ui_contract_player",
            she::Color{0.9F, 0.95F, 1.0F, 1.0F}});
    renderer->BeginFrame(0);
    renderer->SubmitSceneSnapshot(scene->GetActiveSceneName(), scene->GetEntityCount());
    const bool submittedCamera = renderer->SubmitCamera(
        she::Camera2DSubmission{
            cameraId,
            "UiCamera",
            she::Vector2{0.0F, 0.0F},
            1.0F,
            config.windowWidth,
            config.windowHeight,
            she::Color{0.07F, 0.08F, 0.11F, 1.0F}});
    const bool submittedSprite = renderer->SubmitSprite(
        she::Sprite2DSubmission{
            playerId,
            "UiPlayer",
            "materials/ui_contract.player",
            she::Vector2{1.0F, 2.5F},
            5.0F,
            she::Vector2{1.5F, 1.5F},
            she::Color{1.0F, 1.0F, 1.0F, 1.0F},
            0});
    renderer->EndFrame();

    diagnostics->BeginFrame(0);
    diagnostics->RecordPhase("Platform", "window_events_pumped=1\nwindow_size=960x540");
    diagnostics->RecordPhase("Gameplay", gameplay->BuildGameplayDigest());
    diagnostics->RecordPhase("Presentation", "active_scene=UiContractScene\nui_frame_submitted=true");
    diagnostics->EndFrame();

    ai->SetAuthoringIntent("Inspect the integrated runtime through a stable debug surface.");
    ai->RefreshContext(0);

    ui->BeginFrame(1);
    const std::string report = ui->BuildLatestDebugReport();
    ui->EndFrame();

    bool success = true;
    success = Expect(wroteCameraTransform, "Camera transform should be writable for the UI contract setup.") && success;
    success = Expect(wroteFloorTransform, "Floor transform should be writable for the UI contract setup.") && success;
    success = Expect(wrotePlayerTransform, "Player transform should be writable for the UI contract setup.") && success;
    success = Expect(schemaResult.accepted, "UI contract schema should register successfully.") && success;
    success = Expect(recordResult.parsed && recordResult.valid, "UI contract record should parse as trusted data.") && success;
    success = Expect(createdFloorBody, "Physics setup should create the floor body.") && success;
    success = Expect(createdPlayerBody, "Physics setup should create the player body.") && success;
    success = Expect(
                  floorColliderId != she::kInvalidPhysicsColliderId && playerColliderId != she::kInvalidPhysicsColliderId,
                  "Physics setup should create both colliders.") &&
              success;
    success = Expect(submittedCamera, "Renderer setup should accept the UI contract camera submission.") && success;
    success = Expect(submittedSprite, "Renderer setup should accept the UI contract sprite submission.") && success;
    success = Expect(Contains(report, "ui_debug_report_version: 1"), "UI debug report should expose its contract version.") && success;
    success = Expect(Contains(report, "frame_index: 1"), "UI debug report should retain the requested frame index.") && success;
    success = Expect(Contains(report, "[runtime_overview]"), "UI debug report should include the runtime overview section.") && success;
    success = Expect(Contains(report, "active_scene: UiContractScene"), "UI debug report should preserve the active scene name.") && success;
    success = Expect(Contains(report, "asset_count: 1"), "UI debug report should surface the asset registry count.") && success;
    success = Expect(Contains(report, "[scene]"), "UI debug report should include the scene section.") && success;
    success = Expect(Contains(report, "name=UiPlayer"), "UI debug report should list scene entities by name.") && success;
    success = Expect(Contains(report, "[renderer_backend]"), "UI debug report should include renderer backend metadata.") && success;
    success = Expect(Contains(report, "backend_name: sdl3/"), "UI debug report should surface the SDL renderer backend.") && success;
    success = Expect(Contains(report, "[renderer]"), "UI debug report should include the last completed renderer frame.") && success;
    success = Expect(Contains(report, "scene_name: UiContractScene"), "UI debug report should retain the submitted render scene.") && success;
    success = Expect(Contains(report, "material=materials/ui_contract.player"), "UI debug report should retain sprite material details.") && success;
    success = Expect(Contains(report, "[physics]"), "UI debug report should include physics inspection content.") && success;
    success = Expect(Contains(report, "physics_debug_version: 1"), "Physics inspection should be routed into the UI debug report.") && success;
    success = Expect(Contains(report, "body_count: 2"), "Physics inspection should retain the created body count.") && success;
    success = Expect(Contains(report, "collider_count: 2"), "Physics inspection should retain the created collider count.") && success;
    success = Expect(Contains(report, "[diagnostics]"), "UI debug report should include diagnostics content.") && success;
    success = Expect(Contains(report, "diagnostics_report_version: 1"), "UI debug report should embed the latest diagnostics report.") && success;
    success = Expect(Contains(report, "InspectRuntimeRequested"), "UI debug report should expose gameplay activity through diagnostics.") && success;
    success = Expect(Contains(report, "[authoring_context]"), "UI debug report should include authoring context preview.") && success;
    success = Expect(Contains(report, "authoring_context_contract_version: 1"), "UI debug report should preview the authoring context export.") && success;

    ui->Shutdown();
    ai->Shutdown();
    diagnostics->Shutdown();
    scripting->Shutdown();
    physics->Shutdown();
    renderer->Shutdown();
    gameplay->Shutdown();
    data->Shutdown();
    reflection->Shutdown();
    scene->Shutdown();
    assets->Shutdown();
    window->Shutdown();

    return success;
}
} // namespace

int main(int, char**)
{
    return TestDebugUiBuildsIntegratedRuntimeReport() ? 0 : 1;
}
