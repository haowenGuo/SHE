# W07 - Platform + Input

Owner: unassigned
Status: ready
Primary Module: `Engine/Platform`
Related Modules: `Engine/Core`, `Engine/UI`, `Tools/Sandbox`
Branch: `codex/w07/platform-input`
Worktree: `../SHE-w07-platform`
Dependencies: W00

## Goal

Replace the null platform path with the first real SDL3-backed runtime for
windowing, event pumping, timing, and input capture.

## In Scope

- SDL3 window creation
- event pumping integration
- keyboard and pointer input state
- platform service replacement for the bootstrap null implementation

## Out of Scope

- controller support breadth
- platform-specific packaging

## Files Expected To Change

- `Engine/Platform/*`
- selected core runtime startup files
- platform/input smoke tests

## Shared Interfaces To Watch

- [RuntimeServices.hpp](../../Engine/Core/Include/SHE/Core/RuntimeServices.hpp)
- [Application.cpp](../../Engine/Core/Source/Application.cpp)

## Acceptance Target

- the application loop runs on a real SDL3-backed window path
- input state is queryable through a documented platform contract

## Test Plan

- platform service smoke test
- input state/event smoke test

## Handoff Notes For Next Codex

- report every shared-file hook added to support SDL3
- document assumptions about threading and event ownership
