# Handoff - W01

From: W01 Gameplay Core Codex
To: W00 Integrator Codex
Date: 2026-04-13
Branch / Worktree: `codex/w01/gameplay-core` / `F:\SHE-workspace\SHE-w01-gameplay`
Status: `ready_for_integration`

## What Changed

- Reworked `GameplayService` from a thin queue/digest wrapper into a first-pass gameplay substrate with:
  - command registry
  - formal command execution path
  - frame-scoped event bus
  - timer dispatch routed through the same event system
- Added execution-order comments that make the contract explicit:
  - timers advance during fixed-step updates and enqueue gameplay events
  - `FlushCommands()` snapshots the current command buffer, routes commands into events, then drains queued events FIFO
  - events queued by handlers dispatch in the same flush
  - commands queued by handlers wait until the next flush
- Added a small bootstrap demonstration:
  - `SpawnEncounter` is now registered as a gameplay command
  - bootstrap subscribes to `timer/TimerElapsed`
  - the bootstrap timer now triggers follow-up gameplay activity through the shared gameplay path
- Added focused gameplay contract tests alongside the existing smoke test coverage.

## Files Changed

- `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`
- `Engine/Gameplay/CMakeLists.txt`
- `Engine/Gameplay/Include/SHE/Gameplay/CommandBuffer.hpp`
- `Engine/Gameplay/Include/SHE/Gameplay/CommandRegistry.hpp`
- `Engine/Gameplay/Include/SHE/Gameplay/EventBus.hpp`
- `Engine/Gameplay/Include/SHE/Gameplay/GameplayService.hpp`
- `Engine/Gameplay/Include/SHE/Gameplay/TimerService.hpp`
- `Engine/Gameplay/Source/CommandBuffer.cpp`
- `Engine/Gameplay/Source/CommandRegistry.cpp`
- `Engine/Gameplay/Source/EventBus.cpp`
- `Engine/Gameplay/Source/GameplayService.cpp`
- `Engine/Gameplay/Source/TimerService.cpp`
- `Game/Features/Bootstrap/BootstrapFeatureLayer.hpp`
- `Game/Features/Bootstrap/BootstrapFeatureLayer.cpp`
- `Tests/CMakeLists.txt`
- `Tests/Source/GameplayContractTests.cpp`
- `Tests/Source/SmokeTests.cpp`

## Interfaces Changed

Shared interface touched:

- `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`

`IGameplayService` gained these public contract points:

- `RegisterCommand(GameplayCommandDescriptor)`
- `HasCommand(std::string_view)`
- `SubscribeToEvent(std::string, std::string, GameplayEventHandler)`
- `UnsubscribeFromEvent(std::size_t)`

New shared gameplay data contracts added in that header:

- `GameplayCommandDescriptor`
- `GameplayEvent`
- `GameplayEventHandler`

Shared high-risk files not changed:

- `Engine/Core/Source/Application.cpp`
- `Engine/CMakeLists.txt`
- top-level `CMakeLists.txt`

Integration note:

- `W00/main` should record this shared interface change in `coordination/INTEGRATION_REPORT.md`.

## Tests Run

Configured and built with:

- `C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --preset default`
- `C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build --preset build-default`

Executed:

- `C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe --preset test-default`

Results:

- `she_smoke_tests` passed
- `she_gameplay_contract_tests` passed

## Acceptance Status

Acceptance target status:

- command registry: done
- command execution path: done
- event bus: done
- timer dispatch through gameplay events: done
- focused tests: done

Recommended workstream state for `W00/main`:

- `ready_for_integration`

Coordination mismatch observed at start:

- from this worktree, `git show main:coordination/TASK_BOARD.md` still showed `W01` as `ready` / `unassigned`, not `in_progress`
- if `W00` had uncommitted local coordination edits, they were not visible here

## Known Risks

- The new gameplay event bus is intentionally in-process and frame-local; diagnostics/AI can inspect it through `BuildGameplayDigest()`, but there is not yet a dedicated diagnostics hook or exported structured event history API.
- Command descriptors currently route one command into one primary event. If future scripting or physics needs richer multi-step command execution, that should extend the command executor rather than bypass it.
- Event filtering currently uses exact match or wildcard-empty fields. If more complex routing is needed later, evolve the subscription contract carefully because it now lives in `RuntimeServices.hpp`.

## Recommended Next Step

1. `W00` updates `coordination/TASK_BOARD.md` and `coordination/STATUS_LEDGER.md` on `main` to reflect W01 progress and completion state.
2. `W00` records the `RuntimeServices.hpp` gameplay interface expansion in `coordination/INTEGRATION_REPORT.md`.
3. `W03` can now target the richer gameplay digest for command/event visibility.
4. `W04`, `W09`, and `W10` should treat the new `IGameplayService` command/event path as the stable entry point for scripting-triggered, physics-triggered, and audio-triggered gameplay activity.
