# W13 - Polish + Feel

Owner: unassigned
Status: ready
Primary Module: `Game/Features/VerticalSlice`
Related Modules: `Engine/Renderer`, `Engine/Audio`, `Engine/UI`, `Engine/Platform`
Branch: `codex/w13/polish-feel`
Worktree: `../SHE-w13-polish-feel`
Dependencies: W12

## Goal

Improve the integrated vertical slice so it reads clearly, responds quickly, and
feels intentional during normal play without relying on debug context.

## In Scope

- player-facing feedback for collect, fail, win, restart, and round-state
  transitions
- camera framing, sprite readability, HUD clarity, and presentation timing
- input-to-response feel, audio cue timing, and momentary state transitions
- focused regression coverage for polish-sensitive gameplay flows when practical

## Out of Scope

- large authored content expansion that belongs to `W15`
- broad rebalance of the existing authored records that belongs to `W14`
- packaging, launch docs, or delivery hardening that belongs to `W16`
- new engine subsystems unless `W00` approves a shared contract change

## Files Expected To Change

- `Game/Features/VerticalSlice/VerticalSliceFeatureLayer.*`
- selected vertical-slice tests
- narrow feature-local docs or handoff notes

## Shared Interfaces To Watch

- [RuntimeServices.hpp](../../Engine/Core/Include/SHE/Core/RuntimeServices.hpp)
- [Renderer2DService.hpp](../../Engine/Renderer/Include/SHE/Renderer/Renderer2DService.hpp)
- [DebugUiService.hpp](../../Engine/UI/Include/SHE/UI/DebugUiService.hpp)
- [Main.cpp](../../Game/Source/Main.cpp)

## Acceptance Target

- the first 30 seconds of play communicate state, danger, and progress clearly
- collect/fail/win/restart transitions feel tighter than the W12 baseline
- the slice remains feature-local and does not create ad hoc shared contracts

## Test Plan

- `she_vertical_slice_smoke_tests`
- `she_vertical_slice_flow_tests`
- full `ctest --preset test-default` before handoff

## Handoff Notes For Next Codex

- call out any shared renderer/audio/ui surface requests explicitly
- separate feel changes from numeric balance changes when possible
- note which player-facing deltas were intentional so `W00` can review them
