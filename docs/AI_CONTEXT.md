# AI Context Contract

This file describes what the engine should always be able to export for Codex.

## Required Sections

The authoring context should contain:

1. project intent
2. active scene and entity count
3. asset count
4. asset registry and loader summary
5. registered engine/gameplay types
6. registered feature modules
7. registered schemas
8. data registry summary and status
9. current gameplay digest
10. registered script modules
11. latest frame diagnostics report

## Current Exporter

The current exporter lives in:

- [AuthoringAiService.cpp](../Engine/AI/Source/AuthoringAiService.cpp)

and gathers data from:

- [ReflectionService.cpp](../Engine/Reflection/Source/ReflectionService.cpp)
- [DataService.cpp](../Engine/Data/Source/DataService.cpp)
- [GameplayService.cpp](../Engine/Gameplay/Source/GameplayService.cpp)
- [ScriptingService.cpp](../Engine/Scripting/Source/ScriptingService.cpp)
- [DiagnosticsService.cpp](../Engine/Diagnostics/Source/DiagnosticsService.cpp)

## Stability Rules

1. Context output should prefer stable labels over transient memory details.
2. New engine subsystems should extend the context exporter, not bypass it.
3. Game features should register metadata instead of forcing AI tools to scrape
   random files.
4. Data changes should be discoverable through schema registration.
5. The data contract should stay owned by `IDataService`; AI context should
   summarize it rather than replacing it with AI-only formatting.

## Stable Output Shape

The first stable W03 export uses the following outer structure:

- `authoring_context_contract_version`
- `context_version`
- `frame_index`
- `[project]`
- `[runtime_state]`
- `[module_counts]`
- `[reflection_catalog]`
- `[asset_registry]`
- `[schema_catalog]`
- `[data_registry]`
- `[gameplay_state]`
- `[script_catalog]`
- `[latest_frame_report]`

Content-heavy sections use:

- `line_count`
- `content:`

with indented body lines so later Codex sessions can parse the outer section
layout without guessing where nested catalogs or reports begin.

`[asset_registry]`, `[schema_catalog]`, and `[data_registry]` should be built
from stable service contracts (`IAssetService` and `IDataService`) rather than
from ad hoc file scraping. That keeps asset/schema/registry semantics owned by
their modules while still making the result consumable by Codex.

For the first-wave closeout, the expected data baseline is:

- `DataSchemaFieldContract`
- `DataSchemaRegistrationResult`
- `DataRegistryEntry`
- `RegisterSchema(DataSchemaContract)`
- `LoadRecord(const DataLoadRequest&)`
- `HasSchema(std::string_view)`
- `HasTrustedRecord(std::string_view)`
- `GetSchemaCount()`
- `GetRecordCount()`
- `GetTrustedRecordCount()`
- `ListSchemas()`
- `ListRecords()`
- `DescribeSchemas()`
- `DescribeRegistry()`

The AI exporter should build its schema and registry sections on top of those
contracts instead of narrowing the data module back to a schema-only view or
inventing an AI-only replacement contract.

## Diagnostics Report Shape

`[latest_frame_report]` now wraps a stable diagnostics report with:

- `diagnostics_report_version`
- `captured_frames`
- `frame_index`
- `phase_count`
- `contains_gameplay_activity`
- `[frame_summary]`
- one `[phase_<index>]` section per recorded phase

This keeps tracing readable while making command and event capture visible from
the frame narrative instead of only from ad hoc logs.
