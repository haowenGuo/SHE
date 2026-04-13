# W14 - Balance + Progression

Owner: unassigned
Status: ready
Primary Module: `Game/Features/VerticalSlice/Data`
Related Modules: `Game/Features/VerticalSlice/Scripts`, `Engine/Data`, `Engine/Gameplay`
Branch: `codex/w14/balance-progression`
Worktree: `../SHE-w14-balance-progression`
Dependencies: W12

## Goal

Tune the accepted slice so difficulty, pacing, pickup value, and round flow feel
fair, legible, and intentionally paced rather than merely functional.

## In Scope

- tuning of existing pickup, wave, and round authored records
- pacing changes in existing script-driven flow and timer behavior
- difficulty smoothing, pressure curves, restart pacing, and scoring or success
  thresholds if they already live in feature-owned data or script logic
- narrowly scoped data-schema additions only when needed and clearly documented

## Out of Scope

- presentation polish that belongs to `W13`
- additive new content packs or encounter families that belong to `W15`
- launch hardening and packaging work that belongs to `W16`
- shared engine rewrites unrelated to data-driven tuning

## Files Expected To Change

- existing `Game/Features/VerticalSlice/Data/*.yml`
- `Game/Features/VerticalSlice/Scripts/arena_flow.lua`
- selected vertical-slice tests

## Shared Interfaces To Watch

- [RuntimeServices.hpp](../../Engine/Core/Include/SHE/Core/RuntimeServices.hpp)
- [DataService.hpp](../../Engine/Data/Include/SHE/Data/DataService.hpp)
- [GameplayService.hpp](../../Engine/Gameplay/Include/SHE/Gameplay/GameplayService.hpp)
- [VerticalSliceFeatureLayer.cpp](../../Game/Features/VerticalSlice/VerticalSliceFeatureLayer.cpp)

## Acceptance Target

- win/loss outcomes feel fair and learnable on the default slice
- authored records remain the primary tuning surface
- no single pickup, wave, or timer setting dominates the loop accidentally

## Test Plan

- `she_vertical_slice_flow_tests`
- full `ctest --preset test-default` before handoff

## Handoff Notes For Next Codex

- list every existing authored record that changed
- explain the intended difficulty and pacing deltas in plain language
- if a schema or gameplay contract changed, isolate and justify it explicitly
