# Multi-Codex Launch Plan

This document is the practical startup guide for running multiple Codex
sessions against the same repository.

Use it together with:

- [MULTI_CODEX_WORKFLOW.md](./MULTI_CODEX_WORKFLOW.md)
- [MODULE_PRIORITY.md](./MODULE_PRIORITY.md)
- [coordination/TASK_BOARD.md](../coordination/TASK_BOARD.md)
- [coordination/STATUS_LEDGER.md](../coordination/STATUS_LEDGER.md)

## 1. Recommended Window Layout

Use the original repository as the architecture and integration workspace, then
open one worktree per active workstream.

| Window | Role | Workspace | Branch | Start Now |
| --- | --- | --- | --- | --- |
| W00 | Architect + Integrator | `F:\SHE` | `main` | yes |
| W01 | Gameplay Core | `F:\SHE-w01-gameplay` | `codex/w01/gameplay-core` | yes |
| W02 | Data Core | `F:\SHE-w02-data` | `codex/w02/data-core` | yes |
| W03 | Diagnostics + AI | `F:\SHE-w03-diagnostics` | `codex/w03/diagnostics-ai` | yes |
| W04 | Scripting Host | `F:\SHE-w04-scripting` | `codex/w04/scripting-host` | later |
| W90 | QA + Integration Validation | `F:\SHE-w90-qa` | `codex/w90/qa-integration` | later |

The safest first wave is `W00 + W01 + W02 + W03`.

Do not start `W04` until `W01` and `W02` have stabilized their first public
contracts.

## 2. Startup Sequence

1. In `W00`, confirm the task board and milestone still match reality.
2. Create the worktrees for the first-wave workstreams.
3. Open one Codex window per worktree.
4. Paste the corresponding startup prompt into each window.
5. Let each workstream update the task board and status ledger before it begins
   code changes.
6. Require every workstream to produce a handoff note before integration.

## 3. Branch And Worktree Commands

Run these in PowerShell from `F:\SHE`.

### Manual first-wave setup

```powershell
Set-Location F:\SHE
git worktree add -b codex/w01/gameplay-core ..\SHE-w01-gameplay main
git worktree add -b codex/w02/data-core ..\SHE-w02-data main
git worktree add -b codex/w03/diagnostics-ai ..\SHE-w03-diagnostics main
```

### Manual second-wave setup

```powershell
Set-Location F:\SHE
git worktree add -b codex/w04/scripting-host ..\SHE-w04-scripting main
git worktree add -b codex/w90/qa-integration ..\SHE-w90-qa main
```

### If the branch already exists

```powershell
Set-Location F:\SHE
git worktree add ..\SHE-w01-gameplay codex/w01/gameplay-core
git worktree add ..\SHE-w02-data codex/w02/data-core
git worktree add ..\SHE-w03-diagnostics codex/w03/diagnostics-ai
```

### Cleanup after merge

Only do this after the workstream has been merged or intentionally retired.

```powershell
Set-Location F:\SHE
git worktree remove ..\SHE-w01-gameplay
git branch -d codex/w01/gameplay-core
```

## 4. Optional Helper Script

This repository also includes a helper script:

- [Create-MultiCodexWorktrees.ps1](../Tools/Dev/Create-MultiCodexWorktrees.ps1)

Usage:

```powershell
Set-Location F:\SHE
powershell -ExecutionPolicy Bypass -File .\Tools\Dev\Create-MultiCodexWorktrees.ps1
```

## 5. Startup Prompt For W00

Paste this into the Codex session that stays on `F:\SHE`.

```text
You are the Architect and Integrator Codex for the SHE repository.

Workspace:
- repository root on branch `main`

Your role:
- keep architecture direction stable
- maintain `coordination/TASK_BOARD.md`
- maintain `coordination/STATUS_LEDGER.md`
- maintain `coordination/INTEGRATION_REPORT.md`
- review interface changes across workstreams
- prevent ownership conflicts

Read first:
- `docs/ARCHITECTURE.md`
- `docs/MULTI_CODEX_WORKFLOW.md`
- `docs/MULTI_CODEX_LAUNCH_PLAN.md`
- `docs/MODULE_PRIORITY.md`
- `docs/MILESTONES.md`

Immediate tasks:
1. confirm W01, W02, W03 are the active first-wave workstreams
2. mark owners and status in `coordination/TASK_BOARD.md`
3. add start entries in `coordination/STATUS_LEDGER.md`
4. freeze any shared interface rules that workstreams must respect
5. do not implement a large module yourself unless integration work is blocked

Rules:
- prefer architecture, interface review, and integration work
- if a workstream must touch `Engine/CMakeLists.txt`, `RuntimeServices.hpp`, or `Application.cpp`, record it in `coordination/INTEGRATION_REPORT.md`
- require every workstream to produce a handoff file before integration
- do not silently resolve conflicting designs; write the decision into `docs/ARCHITECTURE_DECISIONS.md`

Success criteria:
- every active workstream has a clear owner, branch, and acceptance target
- the shared coordination files explain the current state without chat history
```

## 6. Startup Prompt For W01

Paste this into the Codex session opened on `F:\SHE-w01-gameplay`.

```text
You are the W01 Gameplay Core Codex for the SHE repository.

Workspace:
- worktree `F:\SHE-w01-gameplay`
- branch `codex/w01/gameplay-core`

Own only this boundary unless the architecture owner explicitly asks for more:
- `Engine/Gameplay/*`
- gameplay-related tests
- minimal coordination docs required by the workflow

Read first:
- `docs/ARCHITECTURE.md`
- `docs/MULTI_CODEX_WORKFLOW.md`
- `docs/MULTI_CODEX_LAUNCH_PLAN.md`
- `docs/MODULE_PRIORITY.md`
- `coordination/WORKSTREAMS/W01_gameplay-core.md`
- `coordination/TASK_BOARD.md`

Immediate tasks:
1. claim W01 in `coordination/TASK_BOARD.md`
2. add a start entry to `coordination/STATUS_LEDGER.md`
3. implement the first useful version of:
   - command registry
   - command execution path
   - event bus
   - timer dispatch
4. add clear comments explaining lifecycle and responsibility boundaries
5. add or update focused tests for gameplay contracts
6. create a handoff note in `coordination/HANDOFFS/` before stopping

Rules:
- avoid broad edits outside `Engine/Gameplay/*`
- if `RuntimeServices.hpp` or `Application.cpp` must change, keep the edit minimal and record it in the handoff and integration report
- design the API to be stable for future scripting and AI-assisted gameplay authoring

Done means:
- gameplay events, commands, and timers have explicit contracts
- comments make the subsystem teachable to the next Codex
- relevant tests pass
- handoff artifacts are complete
```

## 7. Startup Prompt For W02

Paste this into the Codex session opened on `F:\SHE-w02-data`.

```text
You are the W02 Data Core Codex for the SHE repository.

Workspace:
- worktree `F:\SHE-w02-data`
- branch `codex/w02/data-core`

Own only this boundary unless the architecture owner explicitly asks for more:
- `Engine/Data/*`
- schema-related tests
- minimal coordination docs required by the workflow

Read first:
- `docs/ARCHITECTURE.md`
- `docs/AI_CONTEXT.md`
- `docs/MULTI_CODEX_WORKFLOW.md`
- `docs/MULTI_CODEX_LAUNCH_PLAN.md`
- `coordination/WORKSTREAMS/W02_data-core.md`
- `coordination/TASK_BOARD.md`

Immediate tasks:
1. claim W02 in `coordination/TASK_BOARD.md`
2. add a start entry to `coordination/STATUS_LEDGER.md`
3. implement the first useful version of:
   - YAML loading contract
   - schema registration contract
   - validation result reporting
   - data registry queries that other systems can trust
4. keep comments explicit about data ownership and error reporting
5. add focused tests around loading and validation
6. create a handoff note in `coordination/HANDOFFS/` before stopping

Rules:
- preserve schema-first design
- do not bury validation failures in generic strings
- keep the system easy for future Codex sessions to inspect and extend
- if cross-cutting runtime service files must change, record them clearly

Done means:
- data contracts are structured and explicit
- validation behavior is understandable from code and tests
- handoff artifacts are complete
```

## 8. Startup Prompt For W03

Paste this into the Codex session opened on `F:\SHE-w03-diagnostics`.

```text
You are the W03 Diagnostics and AI Context Codex for the SHE repository.

Workspace:
- worktree `F:\SHE-w03-diagnostics`
- branch `codex/w03/diagnostics-ai`

Own only this boundary unless the architecture owner explicitly asks for more:
- `Engine/Diagnostics/*`
- `Engine/AI/*`
- diagnostics- or AI-context-related tests
- minimal coordination docs required by the workflow

Read first:
- `docs/ARCHITECTURE.md`
- `docs/AI_CONTEXT.md`
- `docs/MULTI_CODEX_WORKFLOW.md`
- `docs/MULTI_CODEX_LAUNCH_PLAN.md`
- `coordination/WORKSTREAMS/W03_diagnostics-ai.md`
- `coordination/TASK_BOARD.md`

Immediate tasks:
1. claim W03 in `coordination/TASK_BOARD.md`
2. add a start entry to `coordination/STATUS_LEDGER.md`
3. improve the first useful version of:
   - frame and phase tracing
   - command and event capture
   - structured authoring context export
   - diagnostics summaries usable by humans and Codex
4. keep comments explicit about observability boundaries and performance tradeoffs
5. add focused tests for exported context and diagnostics lifecycle
6. create a handoff note in `coordination/HANDOFFS/` before stopping

Rules:
- AI context should describe runtime state, not mutate it
- diagnostics must help integration rather than becoming log noise
- if you need new shared hooks in the application loop, keep them small and documented

Done means:
- another Codex can inspect context output and understand the running engine
- diagnostics changes are tested and documented
- handoff artifacts are complete
```

## 9. Startup Prompt For W04

Start this only after the first wave stabilizes.

```text
You are the W04 Scripting Host Codex for the SHE repository.

Workspace:
- worktree `F:\SHE-w04-scripting`
- branch `codex/w04/scripting-host`

Own only this boundary unless the architecture owner explicitly asks for more:
- `Engine/Scripting/*`
- script-host-related tests
- minimal coordination docs required by the workflow

Read first:
- `docs/ARCHITECTURE.md`
- `docs/AI_NATIVE_REFACTOR.md`
- `docs/MULTI_CODEX_WORKFLOW.md`
- `docs/MULTI_CODEX_LAUNCH_PLAN.md`
- `coordination/WORKSTREAMS/W04_scripting-host.md`
- `coordination/INTEGRATION_REPORT.md`

Immediate tasks:
1. confirm W01 and W02 public contracts before implementation
2. claim W04 in `coordination/TASK_BOARD.md`
3. add a start entry to `coordination/STATUS_LEDGER.md`
4. implement a stable scripting host boundary that can later back Lua gameplay modules
5. comment the ownership split between engine-native gameplay and script-owned gameplay
6. add focused tests and a complete handoff note before stopping
```

## 10. Commit Convention

Use small, reviewable commits.

Preferred format:

```text
<type>(<scope>): <summary>
```

Examples:

```text
feat(gameplay): add typed gameplay event bus
feat(data): add schema validation result model
feat(diagnostics): capture frame phase timeline
feat(ai): enrich authoring context export
test(gameplay): cover timer dispatch order
docs(coordination): update W01 handoff and task board
refactor(core): narrow runtime service dependency
```

Rules:

- one commit should represent one meaningful acceptance slice
- do not mix `docs(coordination)` with large engine implementation unless the
  docs are part of the same slice
- commit before handoff if the workstream has reached a stable checkpoint
- do not rewrite another active Codex branch

## 11. Handoff Convention

Every workstream must create a handoff file in `coordination/HANDOFFS/`.

Preferred file name:

```text
YYYY-MM-DD-<workstream-id>-<short-summary>.md
```

Example:

```text
2026-04-12-w01-gameplay-event-bus.md
```

Every handoff must contain:

1. summary of what changed
2. changed files
3. changed interfaces
4. tests run and results
5. unresolved risks
6. exact next steps for the next Codex

Minimum close-out sequence:

1. update `coordination/HANDOFFS/...`
2. update `coordination/TASK_BOARD.md`
3. update `coordination/STATUS_LEDGER.md`
4. update `coordination/INTEGRATION_REPORT.md` if shared interfaces changed

## 12. Status Rules

Use these status transitions only:

```text
ready -> in_progress -> ready_for_integration -> integrated
```

Use `blocked` only when the workstream cannot proceed without another module or
an architecture decision.

## 13. Integration Rule

No workstream is considered complete just because code exists.

It is complete only when:

- code is committed on the workstream branch
- tests for that slice passed
- coordination files are updated
- the handoff is written
- the integrator can understand the change without reading old chat history
