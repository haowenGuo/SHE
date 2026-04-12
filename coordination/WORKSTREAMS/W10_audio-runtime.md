# W10 - Audio Runtime

Owner: unassigned
Status: backlog
Primary Module: `Engine/Audio`
Related Modules: `Engine/Assets`, `Engine/Gameplay`, `Engine/Scene`, `Game/Features/*`
Branch: `codex/w10/audio-runtime`
Worktree: `../SHE-w10-audio`
Dependencies: W01, W06, W07

## Goal

Replace the null audio service with the first real playback runtime so gameplay
events can produce sound and music.

## In Scope

- miniaudio-backed device/service path
- sound and music playback contracts
- channel or group ownership rules
- gameplay-triggered playback conventions

## Out of Scope

- full audio editor tooling
- advanced mixing authoring UI

## Files Expected To Change

- `Engine/Audio/*`
- selected asset/gameplay-facing integration files
- audio smoke tests

## Shared Interfaces To Watch

- [RuntimeServices.hpp](../../Engine/Core/Include/SHE/Core/RuntimeServices.hpp)
- [DataService.hpp](../../Engine/Data/Include/SHE/Data/DataService.hpp)
- [GameplayService.hpp](../../Engine/Gameplay/Include/SHE/Gameplay/GameplayService.hpp)

## Acceptance Target

- gameplay code can trigger sound or music through a documented runtime service
- audio resource usage and lifetime rules are explicit enough for Codex to
  extend safely

## Test Plan

- audio service smoke test
- sound/music playback contract smoke test

## Handoff Notes For Next Codex

- document audio resource assumptions and playback limits
- highlight every shared service contract change
