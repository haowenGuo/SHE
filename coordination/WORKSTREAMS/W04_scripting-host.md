# W04 - Scripting Host

Owner: unassigned
Status: backlog
Primary Module: `Engine/Scripting`
Related Modules: `Engine/Gameplay`, `Engine/Data`, `Game/Features/*`
Branch: `codex/w04/scripting-host`
Worktree: `../SHE-w04-scripting`
Dependencies: W01, W02

## Goal

Introduce a stable Lua-facing host layer that future gameplay modules can use.

## In Scope

- script host contract refinement
- module loading conventions
- binding registration conventions
- bootstrap script-driven example

## Out of Scope

- large gameplay feature rewrites
- editor scripting tooling

## Files Expected To Change

- `Engine/Scripting/*`
- selected feature bootstrap files
- docs for script conventions

## Shared Interfaces To Watch

- [RuntimeServices.hpp](../../Engine/Core/Include/SHE/Core/RuntimeServices.hpp)
- [GameplayService.hpp](../../Engine/Gameplay/Include/SHE/Gameplay/GameplayService.hpp)

## Acceptance Target

- script modules are loadable through a stable host boundary
- gameplay features can register and call script modules in a documented way

## Test Plan

- script host smoke test
- one bootstrap script module registration test
