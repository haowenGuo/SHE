# W06 - Asset Pipeline

Owner: unassigned
Status: backlog
Primary Module: `Engine/Assets`
Related Modules: `Engine/Data`, `Engine/Renderer`, `Engine/Audio`, `Game/Features/*`
Branch: `codex/w06/asset-pipeline`
Worktree: `../SHE-w06-assets`
Dependencies: W02, W05

## Goal

Introduce stable asset identifiers, metadata, and loader contracts that runtime
systems can depend on safely.

## In Scope

- asset identifier model
- metadata and registry conventions
- loader registration and lookup
- asset handle lifetime rules

## Out of Scope

- editor asset browser UI
- advanced import pipeline tooling

## Files Expected To Change

- `Engine/Assets/*`
- selected data-facing integration files
- asset-focused tests

## Shared Interfaces To Watch

- [RuntimeServices.hpp](../../Engine/Core/Include/SHE/Core/RuntimeServices.hpp)
- [DataService.hpp](../../Engine/Data/Include/SHE/Data/DataService.hpp)

## Acceptance Target

- renderer and audio workstreams can request assets through documented stable
  contracts
- asset metadata is structured enough for Codex to inspect and extend

## Test Plan

- asset registry smoke test
- loader lookup and handle lifetime smoke test

## Handoff Notes For Next Codex

- list any new asset handle invariants
- mention all downstream module assumptions that were introduced
