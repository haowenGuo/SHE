# W15 - Content Expansion

Owner: unassigned
Status: ready
Primary Module: `Game/Features/VerticalSlice`
Related Modules: `Game/Features/VerticalSlice/Data`, `Game/Features/VerticalSlice/Scripts`, `Engine/Assets`, `Engine/Audio`
Branch: `codex/w15/content-expansion`
Worktree: `../SHE-w15-content-expansion`
Dependencies: W12

## Goal

Expand the accepted slice with additive authored content so it gains more
variation, replay value, and expressive range without reopening engine work.

## In Scope

- new authored round, wave, pickup, hazard, or encounter definitions
- additive scripts, data files, placeholder assets, and audio references that
  deepen the existing slice loop
- narrow wiring changes needed to expose new feature-owned content in the slice
- focused docs describing new content paths and how they are exercised

## Out of Scope

- broad retuning of the existing baseline records that belongs to `W14`
- player-feel and presentation cleanup that belongs to `W13`
- packaging or delivery hardening that belongs to `W16`
- engine-contract churn unless `W00` approves it explicitly

## Files Expected To Change

- new files under `Game/Features/VerticalSlice/Data`
- `Game/Features/VerticalSlice/Scripts/arena_flow.lua`
- `Game/Features/VerticalSlice/VerticalSliceFeatureLayer.*`
- `Game/Features/VerticalSlice/README.md`
- selected vertical-slice tests

## Shared Interfaces To Watch

- [RuntimeServices.hpp](../../Engine/Core/Include/SHE/Core/RuntimeServices.hpp)
- [AssetManager.hpp](../../Engine/Assets/Include/SHE/Assets/AssetManager.hpp)
- [VerticalSliceFeatureLayer.cpp](../../Game/Features/VerticalSlice/VerticalSliceFeatureLayer.cpp)
- [Main.cpp](../../Game/Source/Main.cpp)

## Acceptance Target

- the slice contains materially more authored variation than the W12 baseline
- new content rides accepted data/script/asset contracts rather than shortcuts
- the added content stays reviewable as a bounded feature-local extension

## Test Plan

- extend vertical-slice smoke or flow coverage for new content paths
- full `ctest --preset test-default` before handoff

## Handoff Notes For Next Codex

- distinguish additive files from modifications to existing baseline records
- call out any coordination with `W14` when touching existing content data
- summarize which new encounters or authored paths were added
