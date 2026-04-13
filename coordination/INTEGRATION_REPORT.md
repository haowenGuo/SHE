# Integration Report

This file is maintained by the integrator Codex.

## Purpose

Track:

- which workstreams are ready to integrate
- which interfaces changed
- which cross-module risks need validation

## Current Summary

- `W00` integrated successfully
- `W01`, `W02`, and `W03` were the recommended first-wave launch set and are now integrated on `main`
- `W01` is integrated on `main` and now defines the gameplay public-contract baseline for downstream runtime workstreams
- `W02` is integrated on `main` as commit `1202779` and now defines the accepted data-contract baseline for downstream runtime workstreams
- `W03` is integrated on `main` as commit `6b13173` and now defines the accepted diagnostics and AI-context baseline layered on top of the W01 and W02 foundations
- the first-wave foundation is complete on `main`, and the project can now begin the core runtime-wave implementation stage
- `W05`, `W06`, and `W07` define the second-wave runtime spine
- `W08`, `W09`, and `W10` define the third-wave playable runtime layer
- `W04` and `W11` should wait until stronger runtime contracts exist
- `W00/main` remains the source of truth for task status, ledger entries, and integration notes

## Pending Interface Watchlist

- [RuntimeServices.hpp](../Engine/Core/Include/SHE/Core/RuntimeServices.hpp)
- [Application.cpp](../Engine/Core/Source/Application.cpp)
- [Engine/CMakeLists.txt](../Engine/CMakeLists.txt)
- [CMakeLists.txt](../CMakeLists.txt)

## 2026-04-13 W00 Multi-Wave Freeze

The following rules are frozen for the current launch plan:

- `W01` owns `Engine/Gameplay/*` and may touch shared runtime files only when the gameplay contract cannot be expressed inside its module boundary.
- `W02` owns `Engine/Data/*` and should keep validation failures structured and machine-readable.
- `W03` owns `Engine/Diagnostics/*` and `Engine/AI/*` and should treat AI context as read-only with respect to simulation.
- `W05` owns the world model and should define scene lifetime and component query rules before renderer and physics harden their own contracts.
- `W06` owns asset identifiers and loader contracts that renderer and audio will consume.
- `W07` owns the first real platform/input path and should keep shared-loop changes minimal and explicit.
- `W08` depends on `W05`, `W06`, and `W07` and should not invent hidden scene or asset contracts.
- `W09` depends on `W01` and `W05` and should keep fixed-step sequencing explicit.
- `W10` owns the first real audio runtime and should keep playback rules documented and event-friendly.
- `W11` depends on diagnostics plus runtime surfaces and should not become a dumping ground for missing engine responsibilities.
- Any change to `RuntimeServices.hpp`, `Application.cpp`, `Engine/CMakeLists.txt`, or `CMakeLists.txt` must be called out in the workstream handoff and reviewed by `W00` before integration.
- Workstream-local copies of coordination files are not shared state; `W00/main` is the only authoritative coordination surface during active parallel work.
- Structural interface changes that affect more than one workstream should be recorded in `docs/ARCHITECTURE_DECISIONS.md` before integration.

## 2026-04-13 Acceptance Pass

Integrated on `main`:

- `W01` gameplay contracts
  - verified in `F:\SHE-workspace\SHE-w01-gameplay`
  - integrated into `W00/main` as commit `290150d`
  - new shared gameplay surface in `RuntimeServices.hpp`:
    - `GameplayCommandDescriptor`
    - `GameplayEvent`
    - `GameplayEventHandler`
    - `IGameplayService::RegisterCommand(...)`
    - `IGameplayService::HasCommand(...)`
    - `IGameplayService::SubscribeToEvent(...)`
    - `IGameplayService::UnsubscribeFromEvent(...)`
  - downstream baseline:
    - `W03` should inspect gameplay activity through `BuildGameplayDigest()` and the new event-aware gameplay contract rather than private queues
    - `W04`, `W09`, and `W10` should route gameplay-facing triggers through `IGameplayService` instead of inventing parallel dispatch paths

Integrated first-wave checkpoints:

- `W01` integrated as `290150d`
- `W02` integrated as `1202779`
- `W03` integrated as `6b13173`

The W02/W03 closeout directives remain in the repository as historical launch
artifacts:

- `coordination/WORKSTREAMS/W02_closeout-directive.md`
- `coordination/WORKSTREAMS/W03_closeout-directive.md`

Recommended integration order:

1. launch `W07` Platform + Input as the first active core-runtime workstream
2. launch `W05` Scene + ECS after W07 documents loop/input assumptions on `main`
3. launch `W06` Asset Pipeline after W05 establishes the world-model boundary
4. hold `W08`, `W09`, `W10`, and `W11` until the runtime spine contracts stabilize

## Integration Rules

Before merging a workstream:

1. read its workstream note
2. read its handoff note
3. verify acceptance checklist coverage
4. verify build/tests
5. note any changed shared interfaces here
