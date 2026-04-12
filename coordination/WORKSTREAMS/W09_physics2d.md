# W09 - Physics2D

Owner: unassigned
Status: backlog
Primary Module: `Engine/Physics`
Related Modules: `Engine/Gameplay`, `Engine/Scene`, `Game/Features/*`
Branch: `codex/w09/physics2d`
Worktree: `../SHE-w09-physics`
Dependencies: W01, W05

## Goal

Introduce a Box2D-backed fixed-step physics runtime that integrates cleanly
with gameplay events and the scene world model.

## In Scope

- physics world ownership
- body and collider lifecycle
- fixed-step simulation integration
- collision callback bridge into gameplay-facing events

## Out of Scope

- full editor visualization tooling
- advanced joints and authoring UI

## Files Expected To Change

- `Engine/Physics/*`
- selected gameplay/scene-facing contracts
- physics smoke tests

## Shared Interfaces To Watch

- [Application.cpp](../../Engine/Core/Source/Application.cpp)
- [GameplayService.hpp](../../Engine/Gameplay/Include/SHE/Gameplay/GameplayService.hpp)

## Acceptance Target

- fixed-step physics updates run through a documented path
- collision results can reach gameplay through explicit contracts

## Test Plan

- body lifecycle smoke test
- fixed-step and collision callback smoke test

## Handoff Notes For Next Codex

- highlight fixed-step sequencing assumptions
- report every gameplay event or scene contract added for physics
