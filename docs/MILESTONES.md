# Milestones

This file defines milestone goals and "done" criteria for the engine.

## M0 - Coordination Foundation

Goal:

- make multi-Codex development reliable before parallel implementation grows

Done when:

- workflow docs exist
- coordination files exist
- first workstreams are defined
- acceptance rules are documented

## M1 - Gameplay Authoring Core

Primary workstreams:

- `W01 Gameplay Core`
- `W02 Data Core`
- `W03 Diagnostics + AI Context`

Done when:

- gameplay commands can be registered and executed through a stable contract
- gameplay events are observable and traceable
- timers can trigger gameplay flow in a deterministic way
- data schemas can be loaded/registered and validated through a real contract
- AI context export includes gameplay, data, diagnostics, and feature metadata
- smoke tests cover the new contracts

## M2 - Scriptable Gameplay

Primary workstreams:

- `W04 Scripting Host`
- `W01 Gameplay Core` follow-up
- `W02 Data Core` follow-up

Done when:

- a stable Lua host boundary exists
- one gameplay feature can be partly authored through script modules
- data-driven gameplay definitions can call into script hooks

## M3 - World Model Stabilization

Primary workstreams:

- `W05 Scene + ECS`
- `W06 Asset Pipeline`

Done when:

- the bootstrap scene model is replaced or evolved into a stronger ECS contract
- prefabs/scenes/assets have stable authoring paths
- features can target world objects through supported engine contracts

## M4 - Playable Runtime

Primary workstreams:

- `W07 Platform + Input`
- `W08 Renderer2D`
- `W09 Physics2D`

Done when:

- the engine can open a real window
- input drives gameplay
- sprites render
- physics events participate in gameplay flow

## M5 - Authoring Quality

Primary workstreams:

- `W10 UI + Debug Tools`
- diagnostics follow-up
- AI context follow-up

Done when:

- the engine exposes enough debug UI and diagnostics to support fast iteration
- Codex can inspect game state, schemas, and recent frame activity through documented paths

## M6 - Vertical Slice Game

Primary workstreams:

- `W12 First Vertical Slice Game`

Done when:

- one small but complete game loop is implemented
- it uses the engine's official gameplay, data, diagnostics, and AI-native workflows
- the engine proves that Codex can extend gameplay rapidly without architecture rewrites

