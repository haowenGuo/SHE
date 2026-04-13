# W11 Handoff - Debug Surface Bootstrap

Date: 2026-04-13
Workstream: W11 UI + Debug Tools
Branch: codex/w11/ui-debug

## What Landed

W11 replaced the old `NullUiService` runtime wiring in game, sandbox, and test
executables with a real `DebugUiService` that publishes a stable text debug
report each frame.

The new report intentionally stays read-only and contract-driven:

- runtime overview counts
- window state and pointer data
- scene entity listing
- renderer backend info plus the latest completed render frame snapshot
- physics inspection content routed through a stable `IPhysicsService`
  `BuildDebugSummary()` contract
- latest diagnostics report embedding
- authoring-context preview from `IAIService`

This gives sandbox and tests a usable inspection surface without coupling UI
directly to renderer, physics, or diagnostics internals.

## Runtime State The UI Expects

`DebugUiService` expects these services to be assigned before construction:

- `window`
- `assets`
- `scene`
- `data`
- `gameplay`
- `renderer`
- `physics`
- `diagnostics`
- `ai`

It assumes:

- renderer has already published at least one completed frame before the most
  useful report appears
- diagnostics has ended at least one frame before its report is meaningful
- AI context has refreshed at least once before the authoring preview is
  meaningful

Because `Application` calls `ui->BeginFrame()` before renderer present,
diagnostics end, and AI refresh for the current frame, the UI report describes
the latest completed frame rather than partially-built current-frame state.

## Added Contracts

Shared interface additions:

- `IPhysicsService::BuildDebugSummary()`
- `IUiService::BuildLatestDebugReport()`

Concrete implementations:

- `Box2DPhysicsService` now exports body/collider/step inspection text
- `NullPhysicsService` exports an explicit empty physics summary
- `NullUiService` exports an explicit unavailable debug report
- `DebugUiService` aggregates the integrated runtime state into the new report

## Sandbox Integration

`SandboxLayer` now logs the first meaningful debug report during `OnUi()` once
frame 1 is reached, so the sandbox executable immediately proves the surface is
alive without requiring another tool to query it.

## Tests

Added:

- `she_ui_contract_tests`

Updated:

- `she_smoke_tests`

Verified command set:

- `cmake --preset default`
- `cmake --build --preset build-default`
- `ctest --preset test-default --output-on-failure`

## Next Good Steps

1. Render the same report through an actual ImGui backend instead of logs/text
   export only.
2. Promote physics and diagnostics summaries from string blocks to richer
   structured panel models when W11 needs filtering or per-item widgets.
3. Add input-driven panel toggles once a shared debug-key policy exists.
