# Session Bootstrap

This file is the short bridge between the earlier planning conversation and the
new multi-worktree development setup.

Use this file when opening a fresh Codex window so the new session can recover
the important context quickly without relying on old chat history.

## 1. Current Repository Home

The active workspace layout is now:

- `F:\SHE-workspace\SHE` for `W00` on branch `main`
- `F:\SHE-workspace\SHE-w01-gameplay` for `W01`
- `F:\SHE-workspace\SHE-w02-data` for `W02`
- `F:\SHE-workspace\SHE-w03-diagnostics` for `W03`

The older `F:\SHE` path should be treated as a legacy workspace from the
earlier conversation, not the primary place to continue development.

## 2. What Has Already Been Established

The repository is no longer a blank skeleton. It already contains:

- a CMake-based engine/game/tools/tests layout
- AI-native engine service boundaries
- initial modules for reflection, data, gameplay, scripting, diagnostics, and AI
- bootstrap feature code in `Game/Features/Bootstrap/`
- smoke tests that build and pass on the current baseline
- a multi-Codex coordination system under `docs/` and `coordination/`

## 3. Important Documents To Read First

Every fresh Codex session should start with:

1. `docs/ARCHITECTURE.md`
2. `docs/AI_NATIVE_REFACTOR.md`
3. `docs/MULTI_CODEX_WORKFLOW.md`
4. `docs/MULTI_CODEX_LAUNCH_PLAN.md`
5. `docs/MODULE_PRIORITY.md`
6. `coordination/TASK_BOARD.md`
7. `coordination/STATUS_LEDGER.md`

## 4. Current Collaboration Rule

The most important workflow rule is:

`W00` on branch `main` is the source of truth for shared coordination files.

That means:

- `coordination/TASK_BOARD.md` is authoritative on `W00/main`
- `coordination/STATUS_LEDGER.md` is authoritative on `W00/main`
- `coordination/INTEGRATION_REPORT.md` is authoritative on `W00/main`

Workstream windows should mainly implement code in their owned module boundary
and report progress back to `W00`.

## 5. Current Workstream Boundaries

### W00

- workspace: `F:\SHE-workspace\SHE`
- branch: `main`
- role: architect, integrator, acceptance owner

### W01

- workspace: `F:\SHE-workspace\SHE-w01-gameplay`
- branch: `codex/w01/gameplay-core`
- owns: `Engine/Gameplay/*`

### W02

- workspace: `F:\SHE-workspace\SHE-w02-data`
- branch: `codex/w02/data-core`
- owns: `Engine/Data/*`

### W03

- workspace: `F:\SHE-workspace\SHE-w03-diagnostics`
- branch: `codex/w03/diagnostics-ai`
- owns: `Engine/Diagnostics/*` and `Engine/AI/*`

## 6. Shared High-Risk Files

These files are cross-cutting and should be changed sparingly:

- `Engine/CMakeLists.txt`
- `CMakeLists.txt`
- `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`
- `Engine/Core/Source/Application.cpp`

If a workstream must touch one of them, the change must be reported to `W00`.

## 7. Verified Baseline

The project baseline has already been verified on Windows using the Visual
Studio Build Tools CMake binaries.

The commands used successfully were equivalent to:

```powershell
$cmake = 'C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ctest = 'C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe'
& $cmake --preset default
& $cmake --build --preset build-default
& $ctest --preset test-default
```

## 8. Human Context

The user is still getting familiar with Codex, Git worktrees, and multi-window
development. New Codex sessions should therefore:

- explain actions clearly
- avoid assuming advanced Git habits
- keep the workflow concrete and visible
- prefer repository-documented handoff over chat-dependent memory

## 9. What The Next Codex Should Do

If this is a new `W00` session:

1. verify the active workspace is `F:\SHE-workspace\SHE`
2. confirm `git branch --show-current` returns `main`
3. read the documents listed above
4. maintain shared coordination state on `main`
5. orchestrate `W01`, `W02`, and `W03`

If this is a new workstream session:

1. verify the workspace path and branch
2. read the launch plan and the matching workstream note
3. stay inside the owned module boundary
4. write a handoff before stopping
5. report integration-impacting changes back to `W00`

## 10. Why This File Exists

Fresh Codex windows do not inherit the full original chat history.

This file exists so the project can carry forward stable operational memory in
the repository itself, which is more reliable than depending on any one chat
thread.
