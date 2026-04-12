# W05 - Scene + ECS

Owner: unassigned
Status: backlog
Primary Module: `Engine/Scene`
Related Modules: `Engine/Gameplay`, `Engine/Data`, `Engine/Reflection`, `Game/Features/*`
Branch: `codex/w05/scene-ecs`
Worktree: `../SHE-w05-scene`
Dependencies: W01, W02

## Goal

Introduce the engine's stable world model so gameplay, rendering, and physics
have a common place to meet.

## In Scope

- entity identity and lifetime rules
- component storage and query conventions
- transform ownership conventions
- scene update hooks and world access boundaries

## Out of Scope

- editor scene graph tooling
- full scene serialization pipeline

## Files Expected To Change

- `Engine/Scene/*`
- selected scene-facing tests
- minimal integration points in shared runtime files

## Shared Interfaces To Watch

- [RuntimeServices.hpp](../../Engine/Core/Include/SHE/Core/RuntimeServices.hpp)
- [Application.cpp](../../Engine/Core/Source/Application.cpp)
- [GameplayService.hpp](../../Engine/Gameplay/Include/SHE/Gameplay/GameplayService.hpp)

## Acceptance Target

- entities and components can be created, queried, and destroyed through a
  documented scene boundary
- world update order is explicit enough for renderer and physics workstreams to
  build on

## Test Plan

- scene lifetime smoke test
- component query/update smoke test

## Handoff Notes For Next Codex

- call out any new lifetime or ownership rule explicitly
- highlight every shared-file change for W00
