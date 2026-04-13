# Integration Report

This file is maintained by the integrator Codex.

## Purpose

Track:

- which workstreams are ready to integrate
- which interfaces changed
- which cross-module risks need validation

## Current Summary

- `W00` integrated successfully
- `W01`, `W02`, and `W03` remain the recommended first-wave launch set
- `W01` is integrated on `main` and now defines the gameplay public-contract baseline for downstream runtime workstreams
- `W02` and `W03` both passed standalone configure/build/test, but they are not yet integration-ready as a pair because they overlap in AI context and smoke-test surfaces
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

Open integration blockers:

- `W02` edits `Engine/AI/Source/AuthoringAiService.cpp`, which is inside the `W03` ownership boundary.
- `W02` and `W03` both modify:
  - `Engine/AI/Source/AuthoringAiService.cpp`
  - `docs/AI_CONTEXT.md`
  - `Tests/Source/SmokeTests.cpp`
- `W03` smoke tests still use the pre-W02 `IDataService::RegisterSchema(...)` shape, so the branch must be rebased or manually merged after the W02 data contract lands.
- `W02` and `W03` should rebase onto the integrated W01 gameplay baseline before additional cross-module work continues.

Recommended integration order:

1. integrate `W01`
2. harmonize `W02` and `W03` on one branch or through a W00 merge pass
3. re-run configure/build/test after the merged AI-context and smoke-test surface is settled
4. only then mark `W02` and `W03` as `ready_for_integration`

## Integration Rules

Before merging a workstream:

1. read its workstream note
2. read its handoff note
3. verify acceptance checklist coverage
4. verify build/tests
5. note any changed shared interfaces here
