# W02 Data Contract Layer Handoff

## 1. Summary Of What Changed

W02 promoted `Engine/Data` from a schema name registry into a first-pass data
contract layer.

Delivered in this slice:

- structured schema registration contracts
- constrained YAML load requests
- machine-readable validation issues and load results
- registry queries for trusted and untrusted data records
- data-side exporter requirements documented for W03
- focused data contract tests

The current loader validates top-level YAML mapping fields against declared
field contracts and supports `scalar`, `list`, and `map` value kinds.

## 2. Changed Files

- `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`
- `Engine/Data/Include/SHE/Data/DataService.hpp`
- `Engine/Data/Source/DataService.cpp`
- `Game/Features/Bootstrap/BootstrapFeatureLayer.cpp`
- `Tools/Sandbox/Source/SandboxLayer.cpp`
- `Tests/CMakeLists.txt`
- `Tests/Source/SmokeTests.cpp`
- `Tests/Source/DataContractTests.cpp`
- `docs/SCHEMAS/README.md`
- `Game/Features/Bootstrap/Data/bootstrap_enemy.schema.yml`
- `Game/Features/Bootstrap/Data/bootstrap_encounter.schema.yml`

## 3. Changed Interfaces

Shared interface change:

- `IDataService` in `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`

New public data contract types:

- `DataSchemaFieldContract`
- `DataSchemaContract`
- `DataSchemaRegistrationResult`
- `DataLoadRequest`
- `DataLoadResult`
- `DataRegistryEntry`
- `DataValidationIssue`
- `DataIssueSeverity`

New or changed `IDataService` methods:

- `RegisterSchema(DataSchemaContract)`
- `LoadRecord(const DataLoadRequest&)`
- `HasTrustedRecord(std::string_view)`
- `GetRecordCount()`
- `GetTrustedRecordCount()`
- `ListSchemas()`
- `ListRecords()`
- `DescribeRegistry()`

Exporter ownership note:

- W02 no longer owns the final `Engine/AI/*` exporter wiring.
- W03 should consume the data-side contract below and decide the final exporter
  shape.

## 4. Tests Run And Results

Configured:

- `cmake --preset default`

Built:

- `cmake --build --preset build-default`

Tested:

- `ctest --preset test-default`

Result:

- `she_smoke_tests` passed
- `she_gameplay_contract_tests` passed
- `she_data_contract_tests` passed

## 5. Unresolved Risks

1. The first YAML loader is intentionally narrow. It validates top-level mapping
   shape and field kinds, not full YAML typing, nested object schemas, or rich
   coercion rules.
2. Feature-local `.schema.yml` examples now document the contract shape, but
   runtime schema registration is still code-driven rather than file-loaded.
3. `RuntimeServices.hpp` changed, so W00 must review the shared interface impact
   before integration.
4. W00/main task tracking still showed W02 as `ready / unassigned` when this
   slice started, not `in_progress`.

## 6. Data-Side Requirements For W03 Exporter

W03 should treat these as the minimum stable data-side signals to export for AI
context:

Required counters:

- `schema_count`: total registered schemas from `IDataService::GetSchemaCount()`
- `data_record_count`: total registry entries from `IDataService::GetRecordCount()`
- `trusted_data_record_count`: trusted registry entries from
  `IDataService::GetTrustedRecordCount()`

Required schema fields per schema:

- `schemaName`
- `description`
- `owningSystem`
- `fields[].fieldName`
- `fields[].valueKind`
- `fields[].required`
- `fields[].description`

Required registry fields per record:

- `recordId`
- `schemaName`
- `sourcePath`
- `trusted`
- `discoveredFields`
- `issueCount`

Required semantics:

1. `trusted=true` means the record parsed and produced no error-severity
   validation issues.
2. untrusted records must remain visible in exporter output; they are part of
   the debugging surface, not garbage to hide.
3. exporter labels should stay stable and schema-first rather than being shaped
   around one feature type such as only enemies or encounters.
4. exporter may render catalogs as text or structure, but it should preserve the
   distinction between schema contract metadata and runtime registry status.
5. validation issue details are available in `DataLoadResult`; if W03 chooses to
   include issue summaries, they should preserve machine-readable codes.

## 7. Exact Next Steps For The Next Codex

1. Report the shared `IDataService` contract change to W00 and request
   integration review.
2. Decide whether schema definitions should stay code-registered or gain an
   engine-supported file loading path.
3. Extend validation from field presence/kind to scalar typing and nested field
   contracts when W00 confirms the public shape is acceptable.
4. If W03 wants richer AI summaries, consider consuming `ListSchemas()` and
   `ListRecords()` directly for structured export rather than only string
   catalogs.

## 8. Current Status Recommendation

`ready_for_integration`
