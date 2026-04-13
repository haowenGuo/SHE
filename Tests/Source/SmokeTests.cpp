#include <SDL3/SDL_main.h>

#include "SHE/AI/AuthoringAiService.hpp"
#include "SHE/Assets/AssetManager.hpp"
#include "SHE/Audio/NullAudioService.hpp"
#include "SHE/Core/Application.hpp"
#include "SHE/Data/DataService.hpp"
#include "SHE/Diagnostics/DiagnosticsService.hpp"
#include "SHE/Gameplay/GameplayService.hpp"
#include "SHE/Physics/NullPhysicsService.hpp"
#include "SHE/Platform/SdlWindowService.hpp"
#include "SHE/Reflection/ReflectionService.hpp"
#include "SHE/Renderer/NullRendererService.hpp"
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
        aiContext = services.ai->ExportAuthoringContext();
        aiContextLength = aiContext.size();
        latestDiagnosticsReport = services.diagnostics->BuildLatestReport();
        observedSchemaCount = services.data->ListSchemas().size();
        observedRecordCount = services.data->GetRecordCount();
        observedTrustedRecordCount = services.data->GetTrustedRecordCount();
    }

    int attachCount = 0;
    int detachCount = 0;
    int fixedCount = 0;
    int updateCount = 0;
    int renderCount = 0;
    int uiCount = 0;
    std::size_t aiContextVersion = 0;
    std::size_t aiContextLength = 0;
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
    bool hasTrustedFeatureRecordAfterAttach = false;
    bool hasTrustedEncounterRecordAfterAttach = false;
    bool featureLoadParsed = false;
    bool featureLoadValid = false;
    bool encounterLoadParsed = false;
    bool encounterLoadValid = false;
    std::size_t encounterIssueCount = 0;
    std::string aiContext;
    std::string latestDiagnosticsReport;
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
