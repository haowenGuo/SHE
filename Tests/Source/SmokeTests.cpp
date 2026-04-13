#include <SDL3/SDL_main.h>

#include "SHE/AI/AuthoringAiService.hpp"
#include "SHE/Assets/AssetManager.hpp"
#include "SHE/Audio/NullAudioService.hpp"
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
#include "SHE/UI/NullUiService.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace
{
bool ContainsText(const std::string& text, const std::string& needle)
{
    return text.find(needle) != std::string::npos;
}

bool ContainsSchema(const std::vector<she::DataSchemaContract>& schemas, const std::string& schemaName)
{
    for (const auto& schema : schemas)
    {
        if (schema.schemaName == schemaName)
        {
            return true;
        }
    }

    return false;
}

std::vector<std::string> CollectMissingNeedles(
    const std::string& text,
    const std::initializer_list<std::string_view> needles)
{
    std::vector<std::string> missing;
    for (const std::string_view needle : needles)
    {
        if (!ContainsText(text, std::string{needle}))
        {
            missing.emplace_back(needle);
        }
    }

    return missing;
}

void PrintMissingNeedles(const std::vector<std::string>& missing)
{
    for (const auto& needle : missing)
    {
        std::cerr << "Missing expected text: " << needle << '\n';
    }
}

class CountingLayer final : public she::Layer
{
public:
    CountingLayer() : she::Layer("CountingLayer")
    {
    }

    void OnAttach(she::RuntimeServices& services) override
    {
        ++attachCount;
        services.assets->RegisterLoader(
            she::AssetLoaderDescriptor{
                "texture_placeholder",
                "texture",
                {".placeholder", ".png"},
                "Smoke-test placeholder loader for texture-like assets."});
        services.assets->RegisterLoader(
            she::AssetLoaderDescriptor{
                "audio_placeholder",
                "audio",
                {".placeholder", ".wav"},
                "Smoke-test placeholder loader for audio-like assets."});
        const she::AssetId playerTextureId = services.assets->RegisterAsset(
            she::AssetMetadata{
                she::kInvalidAssetId,
                "textures/player_idle",
                "Tests/Source/player_idle.placeholder",
                "texture",
                "",
                "Tests/Source",
                "tests.smoke.player_visual",
                {"hero", "sprite"},
                {
                    {"usage", "player"},
                    {"variant", "idle"},
                }});
        const she::AssetId playerFireId = services.assets->RegisterAsset(
            she::AssetMetadata{
                she::kInvalidAssetId,
                "audio/player_fire",
                "Tests/Source/player_fire.placeholder",
                "audio",
                "",
                "Tests/Source",
                "tests.smoke.player_fire_sfx",
                {"combat", "sfx"},
                {
                    {"trigger", "player_fire"},
                    {"usage", "combat"},
                }});
        observedAssetCountAfterAttach = services.assets->GetAssetCount();
        hasTextureLoaderAfterAttach = services.assets->HasLoader("texture_placeholder");
        hasAudioLoaderAfterAttach = services.assets->HasLoader("audio_placeholder");
        const auto textureLoader = services.assets->ResolveLoader(playerTextureId);
        const auto audioLoader = services.assets->ResolveLoader(playerFireId);
        textureLoaderResolvedAfterAttach =
            textureLoader.has_value() && textureLoader->loaderKey == "texture_placeholder";
        audioLoaderResolvedAfterAttach =
            audioLoader.has_value() && audioLoader->loaderKey == "audio_placeholder";
        playerTextureAssetId = playerTextureId;
        const she::AssetHandle smokeHandle = services.assets->AcquireAsset(playerTextureId);
        liveHandleCountDuringAttach = services.assets->GetLiveHandleCount(playerTextureId);
        services.assets->ReleaseAsset(smokeHandle);
        liveHandleCountAfterRelease = services.assets->GetLiveHandleCount(playerTextureId);

        services.renderer->RegisterMaterial(
            she::Material2DDescriptor{
                "materials/tests.player",
                "textures/player_idle",
                she::Color{1.0F, 1.0F, 1.0F, 1.0F}});
        hasPlayerMaterialAfterAttach = services.renderer->HasMaterial("materials/tests.player");
        const auto resolvedPlayerMaterial = services.renderer->ResolveMaterial("materials/tests.player");
        playerMaterialResolvedAfterAttach =
            resolvedPlayerMaterial.has_value() && resolvedPlayerMaterial->textureAssetId == playerTextureId &&
            resolvedPlayerMaterial->resolvedLoaderKey == "texture_placeholder";
        const she::RendererBackendInfo backendInfo = services.renderer->GetBackendInfo();
        rendererBackendReadyAfterAttach =
            backendInfo.backendName.starts_with("sdl3/") && backendInfo.supportsMaterialLookup &&
            backendInfo.supportsFrameCapture;
        rendererBackendName = backendInfo.backendName;

        services.reflection->RegisterFeature(
            "SmokeTestFeature",
            "Tests/Source",
            "Verifies that AI-native services are wired into the application.");
        const she::DataSchemaRegistrationResult featureSchemaResult = services.data->RegisterSchema(
            she::DataSchemaContract{
                "tests.smoke.feature",
                "Schema used to confirm that the data service participates in runtime initialization.",
                "Tests/Source",
                {
                    she::DataSchemaFieldContract{"id", "scalar", true, "Stable test record identifier."},
                    she::DataSchemaFieldContract{"value", "scalar", true, "Test payload field."},
                }});
        const she::DataSchemaRegistrationResult encounterSchemaResult = services.data->RegisterSchema(
            she::DataSchemaContract{
                "tests.smoke.encounter",
                "Schema used to confirm that AI context exports a structured data registry.",
                "Tests/Source",
                {
                    she::DataSchemaFieldContract{"encounter_id", "scalar", true, "Stable encounter identifier."},
                    she::DataSchemaFieldContract{"enemy_id", "scalar", true, "Enemy identifier."},
                    she::DataSchemaFieldContract{"spawn_count", "scalar", true, "Spawned enemy count."},
                }});

        schemasAcceptedAfterAttach = featureSchemaResult.accepted && encounterSchemaResult.accepted;
        observedSchemaCountAfterAttach = services.data->GetSchemaCount();
        hasFeatureSchemaAfterAttach = services.data->HasSchema("tests.smoke.feature");
        hasEncounterSchemaAfterAttach = services.data->HasSchema("tests.smoke.encounter");

        const auto schemasAfterAttach = services.data->ListSchemas();
        hasListedFeatureSchemaAfterAttach = ContainsSchema(schemasAfterAttach, "tests.smoke.feature");
        hasListedEncounterSchemaAfterAttach = ContainsSchema(schemasAfterAttach, "tests.smoke.encounter");

        const she::DataLoadResult featureLoadResult = services.data->LoadRecord(
            she::DataLoadRequest{
                "tests.smoke.feature",
                "tests.smoke.feature.player",
                "Tests/Source/smoke_feature.yml",
                "id: tests.smoke.feature.player\n"
                "value: ready\n"});
        const she::DataLoadResult encounterLoadResult = services.data->LoadRecord(
            she::DataLoadRequest{
                "tests.smoke.encounter",
                "tests.smoke.encounter.alpha",
                "Tests/Source/smoke_encounter.yml",
                "encounter_id: tests.smoke.encounter.alpha\n"
                "enemy_id: training_dummy\n"
                "notes: missing_spawn_count\n"});

        observedRecordCountAfterAttach = services.data->GetRecordCount();
        observedTrustedRecordCountAfterAttach = services.data->GetTrustedRecordCount();
        hasTrustedFeatureRecordAfterAttach = services.data->HasTrustedRecord("tests.smoke.feature.player");
        hasTrustedEncounterRecordAfterAttach = services.data->HasTrustedRecord("tests.smoke.encounter.alpha");
        featureLoadParsed = featureLoadResult.parsed;
        featureLoadValid = featureLoadResult.valid;
        encounterLoadParsed = encounterLoadResult.parsed;
        encounterLoadValid = encounterLoadResult.valid;
        encounterIssueCount = encounterLoadResult.issues.size();
        services.scripting->RegisterScriptModule(
            "tests.smoke.module",
            "Placeholder module registered by the smoke test.");
        services.gameplay->RegisterCommand(
            {"SmokeTestCommand", "Routes the smoke test command through the gameplay event bus.", "tests", "SmokeCommandRequested"});
        services.gameplay->RegisterCommand(
            {"SmokeFrameCommand", "Confirms per-frame command routing remains visible in diagnostics.", "tests", "SmokeFrameRequested"});
        services.gameplay->CreateTimer("tests.smoke.timer", 0.05, false);
        services.gameplay->QueueCommand("SmokeTestCommand", "payload=ok");

        services.scene->SetActiveScene("SmokeScene");
        cameraEntityId = services.scene->CreateEntity("SmokeCamera");
        playerEntityId = services.scene->CreateEntity("SmokePlayer");
        cameraTransformWritten = services.scene->TrySetEntityTransform(
            cameraEntityId,
            she::SceneTransform{
                she::Vector2{0.0F, 0.0F},
                0.0F,
                she::Vector2{1.0F, 1.0F}});
        playerTransformWritten = services.scene->TrySetEntityTransform(
            playerEntityId,
            she::SceneTransform{
                she::Vector2{4.0F, 2.0F},
                12.0F,
                she::Vector2{2.0F, 3.0F}});
    }

    void OnFixedUpdate(const she::TickContext&) override
    {
        ++fixedCount;
    }

    void OnUpdate(const she::TickContext& context) override
    {
        ++updateCount;
        const std::string frameLabel = "frame=" + std::to_string(context.frameIndex);
        context.services.gameplay->QueueEvent("smoke", "UpdateObserved", frameLabel);
        context.services.gameplay->QueueCommand("SmokeFrameCommand", frameLabel);
    }

    void OnRender(const she::TickContext& context) override
    {
        ++renderCount;

        she::SceneEntityView cameraView;
        she::SceneEntityView playerView;
        observedCameraEntityInRender = context.services.scene->TryGetEntityView(cameraEntityId, cameraView);
        observedPlayerEntityInRender = context.services.scene->TryGetEntityView(playerEntityId, playerView);

        if (!observedCameraEntityInRender || !observedPlayerEntityInRender)
        {
            return;
        }

        const she::WindowState windowState = context.services.window->GetWindowState();
        acceptedCameraSubmission = context.services.renderer->SubmitCamera(
                                       she::Camera2DSubmission{
                                           cameraEntityId,
                                           cameraView.entityName,
                                           cameraView.transform.position,
                                           1.0F,
                                           windowState.width,
                                           windowState.height,
                                           she::Color{0.05F, 0.08F, 0.12F, 1.0F}}) ||
                                   acceptedCameraSubmission;
        acceptedSpriteSubmission = context.services.renderer->SubmitSprite(
                                       she::Sprite2DSubmission{
                                           playerEntityId,
                                           playerView.entityName,
                                           "materials/tests.player",
                                           playerView.transform.position,
                                           playerView.transform.rotationDegrees,
                                           playerView.transform.scale,
                                           she::Color{1.0F, 0.95F, 0.95F, 1.0F},
                                           10}) ||
                                   acceptedSpriteSubmission;
        liveTextureHandleCountDuringRender = context.services.assets->GetLiveHandleCount(playerTextureAssetId);
    }

    void OnUi(const she::TickContext&) override
    {
        ++uiCount;
    }

    void OnDetach(she::RuntimeServices& services) override
    {
        ++detachCount;
        aiContextVersion = services.ai->GetContextVersion();
        aiContext = services.ai->ExportAuthoringContext();
        aiContextLength = aiContext.size();
        latestDiagnosticsReport = services.diagnostics->BuildLatestReport();
        observedSchemaCount = services.data->ListSchemas().size();
        observedRecordCount = services.data->GetRecordCount();
        observedTrustedRecordCount = services.data->GetTrustedRecordCount();
        lastCompletedRenderFrame = services.renderer->GetLastCompletedFrame();
        liveTextureHandleCountAfterRender = services.assets->GetLiveHandleCount(playerTextureAssetId);
    }

    int attachCount = 0;
    int detachCount = 0;
    int fixedCount = 0;
    int updateCount = 0;
    int renderCount = 0;
    int uiCount = 0;
    std::size_t aiContextVersion = 0;
    std::size_t aiContextLength = 0;
    std::size_t observedAssetCountAfterAttach = 0;
    she::AssetId playerTextureAssetId = she::kInvalidAssetId;
    she::EntityId playerEntityId = she::kInvalidEntityId;
    she::EntityId cameraEntityId = she::kInvalidEntityId;
    std::size_t liveHandleCountDuringAttach = 0;
    std::size_t liveHandleCountAfterRelease = 0;
    std::size_t liveTextureHandleCountDuringRender = 0;
    std::size_t liveTextureHandleCountAfterRender = 0;
    std::size_t observedSchemaCountAfterAttach = 0;
    std::size_t observedRecordCountAfterAttach = 0;
    std::size_t observedTrustedRecordCountAfterAttach = 0;
    std::size_t observedSchemaCount = 0;
    std::size_t observedRecordCount = 0;
    std::size_t observedTrustedRecordCount = 0;
    bool schemasAcceptedAfterAttach = false;
    bool hasFeatureSchemaAfterAttach = false;
    bool hasEncounterSchemaAfterAttach = false;
    bool hasListedFeatureSchemaAfterAttach = false;
    bool hasListedEncounterSchemaAfterAttach = false;
    bool hasTextureLoaderAfterAttach = false;
    bool hasAudioLoaderAfterAttach = false;
    bool textureLoaderResolvedAfterAttach = false;
    bool audioLoaderResolvedAfterAttach = false;
    bool hasPlayerMaterialAfterAttach = false;
    bool playerMaterialResolvedAfterAttach = false;
    bool rendererBackendReadyAfterAttach = false;
    bool cameraTransformWritten = false;
    bool playerTransformWritten = false;
    bool observedCameraEntityInRender = false;
    bool observedPlayerEntityInRender = false;
    bool acceptedCameraSubmission = false;
    bool acceptedSpriteSubmission = false;
    bool hasTrustedFeatureRecordAfterAttach = false;
    bool hasTrustedEncounterRecordAfterAttach = false;
    bool featureLoadParsed = false;
    bool featureLoadValid = false;
    bool encounterLoadParsed = false;
    bool encounterLoadValid = false;
    std::size_t encounterIssueCount = 0;
    std::string rendererBackendName;
    std::string aiContext;
    std::string latestDiagnosticsReport;
    std::optional<she::RenderFrameSnapshot> lastCompletedRenderFrame;
};

she::RuntimeServices CreateTestRuntime()
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

int main(int, char**)
{
    she::ApplicationConfig config;
    config.applicationName = "SHE Smoke Tests";
    config.maxFrames = 3;
    config.startWindowHidden = true;

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

    if (layerPtr->observedAssetCountAfterAttach != 2 || !layerPtr->hasTextureLoaderAfterAttach ||
        !layerPtr->hasAudioLoaderAfterAttach || !layerPtr->textureLoaderResolvedAfterAttach ||
        !layerPtr->audioLoaderResolvedAfterAttach || layerPtr->liveHandleCountDuringAttach != 1 ||
        layerPtr->liveHandleCountAfterRelease != 0)
    {
        std::cerr << "Asset service did not preserve the expected registry and handle lifetime baseline.\n";
        std::cerr << "Observed asset count after attach: " << layerPtr->observedAssetCountAfterAttach << '\n';
        std::cerr << "Has texture loader: " << (layerPtr->hasTextureLoaderAfterAttach ? "true" : "false") << '\n';
        std::cerr << "Has audio loader: " << (layerPtr->hasAudioLoaderAfterAttach ? "true" : "false") << '\n';
        std::cerr << "Texture loader resolved: " << (layerPtr->textureLoaderResolvedAfterAttach ? "true" : "false") << '\n';
        std::cerr << "Audio loader resolved: " << (layerPtr->audioLoaderResolvedAfterAttach ? "true" : "false") << '\n';
        std::cerr << "Live handle count during attach: " << layerPtr->liveHandleCountDuringAttach << '\n';
        std::cerr << "Live handle count after release: " << layerPtr->liveHandleCountAfterRelease << '\n';
        return 1;
    }

    if (!layerPtr->rendererBackendReadyAfterAttach || !layerPtr->rendererBackendName.starts_with("sdl3/") ||
        !layerPtr->hasPlayerMaterialAfterAttach || !layerPtr->playerMaterialResolvedAfterAttach ||
        !layerPtr->cameraTransformWritten || !layerPtr->playerTransformWritten)
    {
        std::cerr << "Renderer bootstrap did not expose the expected backend/material baseline.\n";
        std::cerr << "Renderer backend ready: " << (layerPtr->rendererBackendReadyAfterAttach ? "true" : "false") << '\n';
        std::cerr << "Renderer backend name: " << layerPtr->rendererBackendName << '\n';
        std::cerr << "Has player material: " << (layerPtr->hasPlayerMaterialAfterAttach ? "true" : "false") << '\n';
        std::cerr << "Player material resolved: " << (layerPtr->playerMaterialResolvedAfterAttach ? "true" : "false") << '\n';
        std::cerr << "Camera transform written: " << (layerPtr->cameraTransformWritten ? "true" : "false") << '\n';
        std::cerr << "Player transform written: " << (layerPtr->playerTransformWritten ? "true" : "false") << '\n';
        return 1;
    }

    if (!layerPtr->observedCameraEntityInRender || !layerPtr->observedPlayerEntityInRender ||
        !layerPtr->acceptedCameraSubmission || !layerPtr->acceptedSpriteSubmission ||
        layerPtr->liveTextureHandleCountDuringRender != 1 || layerPtr->liveTextureHandleCountAfterRender != 0 ||
        !layerPtr->lastCompletedRenderFrame.has_value())
    {
        std::cerr << "Renderer did not preserve the expected frame ownership and scene submission path.\n";
        std::cerr << "Observed camera entity in render: " << (layerPtr->observedCameraEntityInRender ? "true" : "false") << '\n';
        std::cerr << "Observed player entity in render: " << (layerPtr->observedPlayerEntityInRender ? "true" : "false") << '\n';
        std::cerr << "Accepted camera submission: " << (layerPtr->acceptedCameraSubmission ? "true" : "false") << '\n';
        std::cerr << "Accepted sprite submission: " << (layerPtr->acceptedSpriteSubmission ? "true" : "false") << '\n';
        std::cerr << "Live texture handles during render: " << layerPtr->liveTextureHandleCountDuringRender << '\n';
        std::cerr << "Live texture handles after render: " << layerPtr->liveTextureHandleCountAfterRender << '\n';
        std::cerr << "Completed frame captured: " << (layerPtr->lastCompletedRenderFrame.has_value() ? "true" : "false") << '\n';
        return 1;
    }

    const she::RenderFrameSnapshot& renderFrame = *layerPtr->lastCompletedRenderFrame;
    if (!renderFrame.backendName.starts_with("sdl3/") || renderFrame.sceneName != "SmokeScene" ||
        renderFrame.sceneEntityCount != 2 || renderFrame.surfaceWidth != config.windowWidth ||
        renderFrame.surfaceHeight != config.windowHeight || !renderFrame.presentedToSurface ||
        renderFrame.visiblePixelSampleCount == 0 || !renderFrame.camera.has_value() ||
        renderFrame.sprites.size() != 1 || renderFrame.camera->cameraName != "SmokeCamera" ||
        renderFrame.sprites.front().entityName != "SmokePlayer" ||
        renderFrame.sprites.front().material.materialName != "materials/tests.player" ||
        renderFrame.sprites.front().material.textureAssetName != "textures/player_idle" ||
        renderFrame.sprites.front().material.resolvedLoaderKey != "texture_placeholder")
    {
        std::cerr << "Completed render frame snapshot was missing expected camera/material details.\n";
        std::cerr << "Backend: " << renderFrame.backendName << '\n';
        std::cerr << "Scene name: " << renderFrame.sceneName << '\n';
        std::cerr << "Scene entity count: " << renderFrame.sceneEntityCount << '\n';
        std::cerr << "Surface: " << renderFrame.surfaceWidth << 'x' << renderFrame.surfaceHeight << '\n';
        std::cerr << "Presented to surface: " << (renderFrame.presentedToSurface ? "true" : "false") << '\n';
        std::cerr << "Visible pixel sample count: " << renderFrame.visiblePixelSampleCount << '\n';
        std::cerr << "Has camera: " << (renderFrame.camera.has_value() ? "true" : "false") << '\n';
        std::cerr << "Sprite count: " << renderFrame.sprites.size() << '\n';
        if (renderFrame.camera.has_value())
        {
            std::cerr << "Camera name: " << renderFrame.camera->cameraName << '\n';
        }
        if (!renderFrame.sprites.empty())
        {
            std::cerr << "Sprite entity name: " << renderFrame.sprites.front().entityName << '\n';
            std::cerr << "Sprite material: " << renderFrame.sprites.front().material.materialName << '\n';
            std::cerr << "Sprite texture asset: " << renderFrame.sprites.front().material.textureAssetName << '\n';
            std::cerr << "Sprite loader: " << renderFrame.sprites.front().material.resolvedLoaderKey << '\n';
        }
        return 1;
    }

    if (!layerPtr->schemasAcceptedAfterAttach || layerPtr->observedSchemaCountAfterAttach != 2 ||
        !layerPtr->hasFeatureSchemaAfterAttach || !layerPtr->hasEncounterSchemaAfterAttach ||
        !layerPtr->hasListedFeatureSchemaAfterAttach || !layerPtr->hasListedEncounterSchemaAfterAttach)
    {
        std::cerr << "Data service did not retain both smoke-test schemas after attach.\n";
        std::cerr << "Schemas accepted after attach: " << (layerPtr->schemasAcceptedAfterAttach ? "true" : "false") << '\n';
        std::cerr << "Observed schema count after attach: " << layerPtr->observedSchemaCountAfterAttach << '\n';
        std::cerr << "Has tests.smoke.feature: " << (layerPtr->hasFeatureSchemaAfterAttach ? "true" : "false") << '\n';
        std::cerr << "Has tests.smoke.encounter: " << (layerPtr->hasEncounterSchemaAfterAttach ? "true" : "false") << '\n';
        std::cerr << "Listed tests.smoke.feature: " << (layerPtr->hasListedFeatureSchemaAfterAttach ? "true" : "false") << '\n';
        std::cerr << "Listed tests.smoke.encounter: " << (layerPtr->hasListedEncounterSchemaAfterAttach ? "true" : "false") << '\n';
        return 1;
    }

    if (layerPtr->observedRecordCountAfterAttach != 2 || layerPtr->observedTrustedRecordCountAfterAttach != 1 ||
        !layerPtr->hasTrustedFeatureRecordAfterAttach || layerPtr->hasTrustedEncounterRecordAfterAttach ||
        !layerPtr->featureLoadParsed || !layerPtr->featureLoadValid ||
        !layerPtr->encounterLoadParsed || layerPtr->encounterLoadValid || layerPtr->encounterIssueCount == 0)
    {
        std::cerr << "Data contract baseline did not preserve trusted/untrusted record visibility.\n";
        std::cerr << "Observed record count after attach: " << layerPtr->observedRecordCountAfterAttach << '\n';
        std::cerr << "Observed trusted record count after attach: " << layerPtr->observedTrustedRecordCountAfterAttach << '\n';
        std::cerr << "Has trusted feature record: " << (layerPtr->hasTrustedFeatureRecordAfterAttach ? "true" : "false") << '\n';
        std::cerr << "Has trusted encounter record: " << (layerPtr->hasTrustedEncounterRecordAfterAttach ? "true" : "false") << '\n';
        std::cerr << "Feature load parsed: " << (layerPtr->featureLoadParsed ? "true" : "false") << '\n';
        std::cerr << "Feature load valid: " << (layerPtr->featureLoadValid ? "true" : "false") << '\n';
        std::cerr << "Encounter load parsed: " << (layerPtr->encounterLoadParsed ? "true" : "false") << '\n';
        std::cerr << "Encounter load valid: " << (layerPtr->encounterLoadValid ? "true" : "false") << '\n';
        std::cerr << "Encounter issue count: " << layerPtr->encounterIssueCount << '\n';
        return 1;
    }

    if (layerPtr->aiContextVersion < 1 || layerPtr->aiContextLength == 0)
    {
        std::cerr << "AI authoring context export was unexpectedly empty.\n";
        return 1;
    }

    if (layerPtr->aiContextVersion != config.maxFrames)
    {
        std::cerr << "AI context version did not advance once per frame.\n";
        return 1;
    }

    const auto missingDiagnosticsNeedles = CollectMissingNeedles(
        layerPtr->latestDiagnosticsReport,
        {"diagnostics_report_version: 1",
         "contains_gameplay_activity: true",
         "[phase_2]",
         "SmokeFrameCommand",
         "UpdateObserved"});
    if (!missingDiagnosticsNeedles.empty())
    {
        std::cerr << "Latest diagnostics report did not capture stable gameplay details.\n";
        PrintMissingNeedles(missingDiagnosticsNeedles);
        return 1;
    }

    const auto missingAiContextNeedles = CollectMissingNeedles(
        layerPtr->aiContext,
        {"authoring_context_contract_version: 1",
         "[module_counts]",
         "[asset_registry]",
         "asset_registry_version: 1",
         "asset_count: 2",
         "loader_count: 2",
         "textures/player_idle",
         "audio/player_fire",
         "[schema_catalog]",
         "registered_schemas: 2",
         "data_records: 2",
         "trusted_data_records: 1",
         "[data_registry]",
         "data_registry_version: 1",
         "record_count: 2",
         "trusted_record_count: 1",
         "tests.smoke.encounter.alpha",
         "[gameplay_state]",
         "[latest_frame_report]"});
    if (!missingAiContextNeedles.empty())
    {
        std::cerr << "AI context export is missing required structured sections.\n";
        std::cerr << "Observed schema count: " << layerPtr->observedSchemaCount << '\n';
        std::cerr << "Observed record count: " << layerPtr->observedRecordCount << '\n';
        std::cerr << "Observed trusted record count: " << layerPtr->observedTrustedRecordCount << '\n';
        PrintMissingNeedles(missingAiContextNeedles);
        return 1;
    }

    return 0;
}
