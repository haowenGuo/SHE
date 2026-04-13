#include "SHE/Data/DataService.hpp"

#include <algorithm>
#include <iostream>
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

template <typename TResult>
bool HasIssueCode(const TResult& result, const std::string_view code)
{
    return std::any_of(
        result.issues.begin(),
        result.issues.end(),
        [code](const she::DataValidationIssue& issue)
        {
            return issue.code == code;
        });
}

she::DataSchemaContract BuildEnemySchema()
{
    return she::DataSchemaContract{
        "feature.bootstrap.enemy",
        "Enemy records used by bootstrap gameplay and future feature tests.",
        "Tests/Source",
        {
            she::DataSchemaFieldContract{"id", "scalar", true, "Stable enemy identifier."},
            she::DataSchemaFieldContract{"display_name", "scalar", true, "Human-readable name."},
            she::DataSchemaFieldContract{"max_health", "scalar", true, "Maximum health value."},
            she::DataSchemaFieldContract{"move_speed", "scalar", true, "Movement speed."},
            she::DataSchemaFieldContract{"abilities", "list", false, "Optional ability identifiers."},
        }};
}

she::DataSchemaContract BuildEncounterSchema()
{
    return she::DataSchemaContract{
        "feature.bootstrap.encounter",
        "Encounter records that reference enemy data and reward outputs.",
        "Tests/Source",
        {
            she::DataSchemaFieldContract{"id", "scalar", true, "Stable encounter identifier."},
            she::DataSchemaFieldContract{"spawn_points", "list", true, "Encounter spawn markers."},
            she::DataSchemaFieldContract{"enemy_refs", "list", true, "Enemy record references."},
            she::DataSchemaFieldContract{"reward_id", "scalar", true, "Reward granted after the encounter."},
        }};
}

she::DataSchemaContract BuildQuestSchema()
{
    return she::DataSchemaContract{
        "feature.bootstrap.quest",
        "Quest records that can later be driven by script-backed content.",
        "Tests/Source",
        {
            she::DataSchemaFieldContract{"id", "scalar", true, "Stable quest identifier."},
            she::DataSchemaFieldContract{"title", "scalar", true, "Displayed quest title."},
            she::DataSchemaFieldContract{"objectives", "list", true, "Objective identifiers."},
            she::DataSchemaFieldContract{"metadata", "map", false, "Optional script-facing metadata."},
        }};
}

bool TestSchemaRegistrationContracts()
{
    she::DataService service;
    service.Initialize();

    bool passed = true;

    const she::DataSchemaRegistrationResult enemyResult = service.RegisterSchema(BuildEnemySchema());
    const she::DataSchemaRegistrationResult encounterResult = service.RegisterSchema(BuildEncounterSchema());
    const she::DataSchemaRegistrationResult questResult = service.RegisterSchema(BuildQuestSchema());

    passed &= Expect(enemyResult.accepted, "Enemy schema should register successfully.");
    passed &= Expect(encounterResult.accepted, "Encounter schema should register successfully.");
    passed &= Expect(questResult.accepted, "Quest schema should register successfully.");
    passed &= Expect(service.GetSchemaCount() == 3, "Three schema contracts should be available.");

    const she::DataSchemaRegistrationResult invalidResult = service.RegisterSchema(
        she::DataSchemaContract{
            "",
            "",
            "",
            {
                she::DataSchemaFieldContract{"", "banana", true, ""},
            }});

    passed &= Expect(!invalidResult.accepted, "Invalid schema registration should be rejected.");
    passed &= Expect(HasIssueCode(invalidResult, "schema_name_missing"), "Rejected schema should report missing name.");
    passed &= Expect(
        HasIssueCode(invalidResult, "schema_owner_missing"),
        "Rejected schema should report missing owner.");
    passed &= Expect(
        HasIssueCode(invalidResult, "schema_field_name_missing"),
        "Rejected schema should report empty field names.");
    passed &= Expect(
        service.GetSchemaCount() == 3,
        "Rejected schema contracts should not change the registered schema count.");

    service.Shutdown();
    return passed;
}

bool TestYamlLoadingAndValidation()
{
    she::DataService service;
    service.Initialize();
    const she::DataSchemaRegistrationResult enemyResult = service.RegisterSchema(BuildEnemySchema());
    const she::DataSchemaRegistrationResult encounterResult = service.RegisterSchema(BuildEncounterSchema());
    const she::DataSchemaRegistrationResult questResult = service.RegisterSchema(BuildQuestSchema());

    bool passed = true;
    passed &= Expect(enemyResult.accepted, "Enemy schema should register before load validation.");
    passed &= Expect(encounterResult.accepted, "Encounter schema should register before load validation.");
    passed &= Expect(questResult.accepted, "Quest schema should register before load validation.");

    const she::DataLoadResult validEnemy = service.LoadRecord(
        she::DataLoadRequest{
            "feature.bootstrap.enemy",
            "bootstrap.enemy.training_dummy",
            "Tests/Data/training_dummy.enemy.yml",
            "id: training_dummy\n"
            "display_name: Training Dummy\n"
            "max_health: 50\n"
            "move_speed: 1.5\n"
            "abilities:\n"
            "  - taunt\n"
            "  - soak_hit\n"});

    passed &= Expect(validEnemy.parsed, "Valid enemy YAML should parse.");
    passed &= Expect(validEnemy.valid, "Valid enemy YAML should satisfy the schema contract.");
    passed &= Expect(
        service.HasTrustedRecord("bootstrap.enemy.training_dummy"),
        "Trusted registry queries should report the accepted enemy record.");

    const she::DataLoadResult invalidEncounter = service.LoadRecord(
        she::DataLoadRequest{
            "feature.bootstrap.encounter",
            "bootstrap.encounter.room_a",
            "Tests/Data/room_a.encounter.yml",
            "id: room_a\n"
            "spawn_points: arena_entry\n"
            "enemy_refs:\n"
            "  - training_dummy\n"
            "bonus_reward: secret_cache\n"});

    passed &= Expect(invalidEncounter.parsed, "Invalid encounter YAML should still parse structurally.");
    passed &= Expect(!invalidEncounter.valid, "Invalid encounter YAML should not become trusted data.");
    passed &= Expect(
        HasIssueCode(invalidEncounter, "field_kind_mismatch"),
        "Invalid encounter YAML should report list/scalar kind mismatches.");
    passed &= Expect(
        HasIssueCode(invalidEncounter, "required_field_missing"),
        "Invalid encounter YAML should report missing required fields.");
    passed &= Expect(
        HasIssueCode(invalidEncounter, "field_not_declared"),
        "Invalid encounter YAML should report undeclared fields.");
    passed &= Expect(
        service.GetRecordCount() == 2,
        "Both valid and invalid load attempts should be queryable in the data registry.");
    passed &= Expect(
        service.GetTrustedRecordCount() == 1,
        "Only valid load results should count as trusted records.");

    const std::string schemaCatalog = service.DescribeSchemas();
    const std::string registryCatalog = service.DescribeRegistry();
    passed &= Expect(
        schemaCatalog.find("feature.bootstrap.enemy") != std::string::npos,
        "Schema catalog should include enemy schema descriptions.");
    passed &= Expect(
        schemaCatalog.find("trusted_records=1/1") != std::string::npos,
        "Schema catalog should summarize trusted record status.");
    passed &= Expect(
        schemaCatalog.find("feature.bootstrap.encounter") != std::string::npos,
        "Schema catalog should include encounter schema descriptions.");
    passed &= Expect(
        registryCatalog.find("bootstrap.enemy.training_dummy") != std::string::npos,
        "Registry catalog should include valid record identifiers.");
    passed &= Expect(
        registryCatalog.find("trusted=false") != std::string::npos,
        "Registry catalog should keep invalid records visible for diagnostics.");

    service.Shutdown();
    return passed;
}
} // namespace

int main()
{
    const bool registrationPassed = TestSchemaRegistrationContracts();
    const bool loadingPassed = TestYamlLoadingAndValidation();
    return registrationPassed && loadingPassed ? 0 : 1;
}
