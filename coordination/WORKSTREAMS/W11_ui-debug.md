# W11 - UI + Debug Tools

Owner: unassigned
Status: backlog
Primary Module: `Engine/UI`
Related Modules: `Engine/Diagnostics`, `Engine/Renderer`, `Tools/Sandbox`, `Game/Features/*`
Branch: `codex/w11/ui-debug`
Worktree: `../SHE-w11-ui-debug`
Dependencies: W03, W08, W09

## Goal

Deliver the first useful runtime debug surfaces so developers can inspect the
engine without reading raw logs alone.

## In Scope

- debug overlay or panel bootstrap
- runtime counters and trace display
- scene, renderer, and physics inspection hooks
- sandbox-facing debug integration

## Out of Scope

- full editor shell
- shipping in-game UI systems

## Files Expected To Change

- `Engine/UI/*`
- selected sandbox/debug integration files
- UI/debug smoke tests

## Shared Interfaces To Watch

- [RuntimeServices.hpp](../../Engine/Core/Include/SHE/Core/RuntimeServices.hpp)
- [DiagnosticsService.hpp](../../Engine/Diagnostics/Include/SHE/Diagnostics/DiagnosticsService.hpp)

## Acceptance Target

- the runtime exposes at least one useful inspection surface for engine state
- debug tooling stays clearly separated from core runtime ownership

## Test Plan

- debug UI bootstrap smoke test
- diagnostics surface integration smoke test

## Handoff Notes For Next Codex

- document what runtime state the UI expects to inspect
- note every renderer or diagnostics dependency introduced
