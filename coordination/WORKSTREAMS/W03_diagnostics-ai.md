# W03 - Diagnostics + AI Context

Owner: unassigned
Status: ready
Primary Module: `Engine/Diagnostics`, `Engine/AI`
Related Modules: `Engine/Reflection`, `Engine/Data`, `Engine/Gameplay`, `Tests`
Branch: `codex/w03/diagnostics-ai`
Worktree: `../SHE-w03-diagnostics`
Dependencies: W00

## Goal

Make the engine's authoring context and diagnostics useful enough that another
Codex can continue development with minimal guesswork.

## In Scope

- richer frame reports
- command/event visibility
- clearer authoring context sections
- tests for exported context quality

## Out of Scope

- online LLM calls
- editor UI
- gameplay execution changes that belong to W01

## Files Expected To Change

- `Engine/Diagnostics/*`
- `Engine/AI/*`
- `docs/AI_CONTEXT.md`
- `Tests/Source/SmokeTests.cpp`

## Shared Interfaces To Watch

- [RuntimeServices.hpp](../../Engine/Core/Include/SHE/Core/RuntimeServices.hpp)
- [Application.cpp](../../Engine/Core/Source/Application.cpp)

## Acceptance Target

- latest frame report contains enough detail to explain gameplay activity
- authoring context clearly exposes module, schema, and gameplay state
- output is stable enough for Codex-driven workflows

## Test Plan

- context export smoke test
- diagnostics report content test
- context version progression test

## Handoff Notes For Next Codex

Prefer stable field names and predictable output structure over clever formatting.
