# W08 - Renderer2D

Owner: unassigned
Status: backlog
Primary Module: `Engine/Renderer`
Related Modules: `Engine/Scene`, `Engine/Assets`, `Engine/Platform`, `Game/Features/*`
Branch: `codex/w08/renderer2d`
Worktree: `../SHE-w08-renderer`
Dependencies: W05, W06, W07

## Goal

Deliver the first real 2D renderer so the engine can draw scenes instead of
only running a null backend.

## In Scope

- renderer backend bootstrap
- camera and sprite submission path
- texture/material resource lookup
- frame begin/end ownership rules

## Out of Scope

- advanced post-processing
- editor rendering passes

## Files Expected To Change

- `Engine/Renderer/*`
- selected scene/asset-facing contracts
- renderer smoke tests

## Shared Interfaces To Watch

- [RuntimeServices.hpp](../../Engine/Core/Include/SHE/Core/RuntimeServices.hpp)
- [Application.cpp](../../Engine/Core/Source/Application.cpp)
- [DataService.hpp](../../Engine/Data/Include/SHE/Data/DataService.hpp)

## Acceptance Target

- a scene can submit a basic camera and sprite path through a documented render
  contract
- renderer frame lifecycle is explicit enough for UI and diagnostics overlays

## Test Plan

- renderer bootstrap smoke test
- basic sprite submission smoke test

## Handoff Notes For Next Codex

- call out every assumption about scene data and asset handles
- note any required shader or backend setup steps
