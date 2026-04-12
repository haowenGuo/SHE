# W01 - Gameplay Core

Owner: unassigned
Status: ready
Primary Module: `Engine/Gameplay`
Related Modules: `Engine/Core`, `Game/Features/Bootstrap`, `Tests`
Branch: `codex/w01/gameplay-core`
Worktree: `../SHE-w01-gameplay`
Dependencies: W00

## Goal

Turn the current gameplay service into a real gameplay substrate with:

- command registry
- command executor
- event bus
- timer dispatch rules

## In Scope

- `Engine/Gameplay/*`
- minimal `RuntimeServices` updates if strictly necessary
- tests for gameplay flow
- bootstrap feature updates to demonstrate the new command/event path

## Out of Scope

- Lua integration
- YAML parsing
- ECS replacement
- rendering and physics backend work

## Files Expected To Change

- `Engine/Gameplay/Include/SHE/Gameplay/*`
- `Engine/Gameplay/Source/*`
- `Game/Features/Bootstrap/*`
- `Tests/Source/SmokeTests.cpp`

## Shared Interfaces To Watch

- [RuntimeServices.hpp](../../Engine/Core/Include/SHE/Core/RuntimeServices.hpp)
- [Application.cpp](../../Engine/Core/Source/Application.cpp)

## Acceptance Target

- commands can be registered or routed through a formal execution path
- events have a structured dispatch model
- timers can emit gameplay events through the same system
- diagnostics and AI context can see the resulting gameplay activity

## Test Plan

- smoke test
- command flow test
- timer-to-event test

## Handoff Notes For Next Codex

Document execution order carefully. This module will become the base contract
for scripting, physics-triggered gameplay, and AI-authored features.
