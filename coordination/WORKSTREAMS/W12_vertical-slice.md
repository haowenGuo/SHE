# W12 - Vertical Slice Game

Owner: unassigned
Status: ready
Primary Module: `Game/Features/*`
Related Modules: `Engine/Gameplay`, `Engine/Scripting`, `Engine/UI`, `Engine/Renderer`, `Engine/Physics`, `Engine/Audio`
Branch: `codex/w12/vertical-slice`
Worktree: `../SHE-w12-vertical-slice`
Dependencies: W01-W11

## Goal

Deliver one small playable game slice that proves the integrated engine stack
works coherently as a whole rather than only as isolated module tests.

## In Scope

- one compact gameplay loop with win/fail or clear player feedback
- feature-owned data, assets, and scripts that ride the accepted engine
  contracts
- renderer, physics, audio, scripting, and debug-surface integration in the
  slice path
- focused slice-level smoke/contract coverage

## Out of Scope

- broad engine rewrites unrelated to the slice
- full editor workflows
- large content pipelines or multiple disconnected game modes

## Files Expected To Change

- `Game/Features/*`
- selected tests and bootstrap wiring files
- focused docs or handoff notes for the slice

## Shared Interfaces To Watch

- [RuntimeServices.hpp](../../Engine/Core/Include/SHE/Core/RuntimeServices.hpp)
- [BootstrapFeatureLayer.cpp](../../Game/Features/Bootstrap/BootstrapFeatureLayer.cpp)
- [ScriptingService.hpp](../../Engine/Scripting/Include/SHE/Scripting/ScriptingService.hpp)
- [DebugUiService.hpp](../../Engine/UI/Include/SHE/UI/DebugUiService.hpp)

## Acceptance Target

- the repository runs one coherent playable slice on the accepted W01-W11
  engine baseline
- slice gameplay uses shared engine contracts rather than slice-only backdoors
- debug and diagnostics surfaces remain useful while the slice is running

## Test Plan

- vertical-slice smoke test
- focused feature contract or regression test for the slice's main loop

## Handoff Notes For Next Codex

- document which engine surfaces the slice actually exercised
- call out any new shared contract additions explicitly
- keep the slice small enough that W00 can integrate it without reopening the
  whole engine architecture
