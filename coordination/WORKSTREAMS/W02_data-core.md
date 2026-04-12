# W02 - Data Core

Owner: unassigned
Status: ready
Primary Module: `Engine/Data`
Related Modules: `Game/Features/*`, `Engine/AI`, `Tests`
Branch: `codex/w02/data-core`
Worktree: `../SHE-w02-data`
Dependencies: W00

## Goal

Evolve the schema registry into a real data contract layer with:

- YAML loading interfaces
- schema validation rules
- structured load errors
- data registry conventions

## In Scope

- `Engine/Data/*`
- feature schema examples
- docs updates for schema rules
- tests for schema loading/validation contracts

## Out of Scope

- asset import/cooking
- scene ECS replacement
- live editor support

## Files Expected To Change

- `Engine/Data/*`
- `Game/Features/*/Data/*`
- `docs/SCHEMAS/*`

## Shared Interfaces To Watch

- [RuntimeServices.hpp](../../Engine/Core/Include/SHE/Core/RuntimeServices.hpp)
- [AI_CONTEXT.md](../../docs/AI_CONTEXT.md)

## Acceptance Target

- schemas are not just registered, but describable through a loading contract
- validation failures are machine-readable
- AI context can summarize known schemas and their status

## Test Plan

- schema registry test
- validation failure test
- schema description export test

## Handoff Notes For Next Codex

Keep the design independent from one specific content type. The first version
must support enemies, encounters, quests, and future script-backed data.
