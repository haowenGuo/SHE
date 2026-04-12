# Multi-Codex Workflow

This document defines how multiple Codex sessions should collaborate on the
same engine repository without stepping on one another.

The goal is not just parallel speed. The goal is reliable handoff, stable
integration, and enough shared structure that every Codex can understand what
the others are doing.

## 1. Core Principle

Each Codex should own a **workstream**, not a random pile of files.

A good workstream has:

- a clear module boundary
- a defined acceptance target
- a known dependency list
- a documented handoff format

Bad parallelization looks like:

- two Codex sessions editing the same subsystem without a contract
- shared files changing without ownership
- hidden architectural decisions

Good parallelization looks like:

- one Codex owns `Gameplay Core`
- one Codex owns `Data Core`
- one Codex owns `Diagnostics + AI context`
- the integrator Codex merges them through documented acceptance criteria

## 1.1 Coordination Source Of Truth

When multiple worktrees are active, the authoritative shared coordination state
should live in the `W00` integration workspace on branch `main`.

That means:

- `coordination/TASK_BOARD.md` on `main` is the source of truth
- `coordination/STATUS_LEDGER.md` on `main` is the source of truth
- `coordination/INTEGRATION_REPORT.md` on `main` is the source of truth

Workstream branches may contain copies of those files because they share the
repository history, but those branch-local copies are **not** the shared live
state.

In practice:

- workstream Codex sessions focus on code inside their owned module boundary
- they report progress and handoff details back to `W00`
- `W00` updates the coordination files on `main`

This avoids the false impression that editing a coordination file in one branch
will automatically update every other active worktree.

## 2. Roles

Multiple roles may be played by the same human-guided thread, but the role
boundaries should remain explicit.

### Architect Codex

Owns:

- architecture direction
- workstream slicing
- milestone planning
- conflict resolution between modules

### Workstream Codex

Owns:

- one bounded module or feature workstream
- implementation details inside that boundary
- tests and docs for that boundary
- handoff notes and status updates

### Integrator Codex

Owns:

- final merge review
- interface compatibility checks
- cross-workstream validation
- integration report updates

### QA Codex

Owns:

- acceptance verification
- scenario validation
- smoke/regression review

## 3. Recommended Working Model

Use **separate branches and separate worktrees** whenever possible.

Preferred branch pattern:

```text
codex/<workstream-id>/<short-name>
```

Examples:

```text
codex/w01/gameplay-core
codex/w02/data-core
codex/w03/diagnostics-ai
```

Preferred worktree pattern:

```text
../SHE-w01-gameplay
../SHE-w02-data
../SHE-w03-diagnostics
```

If multiple Codex sessions must share one workspace, they should only edit:

1. files inside their owned workstream boundary
2. coordination files that are explicitly listed in this document

## 4. Document Stack

Parallel development should use the following document stack:

- [MODULE_PRIORITY.md](./MODULE_PRIORITY.md)
- [MILESTONES.md](./MILESTONES.md)
- [ACCEPTANCE_CHECKLIST.md](./ACCEPTANCE_CHECKLIST.md)
- [ARCHITECTURE_DECISIONS.md](./ARCHITECTURE_DECISIONS.md)
- [MULTI_CODEX_LAUNCH_PLAN.md](./MULTI_CODEX_LAUNCH_PLAN.md)
- [coordination/TASK_BOARD.md](../coordination/TASK_BOARD.md)
- [coordination/STATUS_LEDGER.md](../coordination/STATUS_LEDGER.md)
- [coordination/INTEGRATION_REPORT.md](../coordination/INTEGRATION_REPORT.md)

This is the minimum shared memory system for multiple Codex sessions.

## 5. Required Flow For Every Workstream

Every workstream should follow this order:

1. `W00` claims the workstream in the task board on `main`.
2. `W00` records the start in `STATUS_LEDGER.md` on `main`.
3. The workstream Codex implements only inside the agreed boundary.
4. The workstream Codex runs build/tests relevant to the workstream.
5. The workstream Codex fills a handoff note using `HANDOFF_TEMPLATE.md`.
6. The workstream Codex reports results back to `W00`.
7. `W00` updates `INTEGRATION_REPORT.md` if shared interfaces changed.
8. `W00` marks the task board entry as `ready_for_integration` or `integrated`.

## 6. Update Cadence

Every active Codex should communicate status at these points, while `W00`
updates the authoritative shared coordination files on `main`:

### At start

- workstream ownership confirmation to `W00`
- status start signal to `W00`

### When interface changes

- workstream note
- interface change summary to `W00`
- architecture decision request if the change is structural

### At handoff

- handoff file
- result summary to `W00`
- next-step recommendation to `W00`

## 7. Conflict Avoidance Rules

### Rule 1: One owner per mutable module boundary

At a given moment, only one Codex should own:

- `Engine/Gameplay/*`
- `Engine/Data/*`
- `Engine/Scene/*`
- `Engine/Renderer/*`
- `Engine/Physics/*`

Shared docs may be edited by the owning workstream and the integrator.

### Rule 2: Cross-cutting files are high risk

These files should be edited only when necessary and must be mentioned in the
handoff:

- [Engine/CMakeLists.txt](../Engine/CMakeLists.txt)
- [CMakeLists.txt](../CMakeLists.txt)
- [RuntimeServices.hpp](../Engine/Core/Include/SHE/Core/RuntimeServices.hpp)
- [Application.cpp](../Engine/Core/Source/Application.cpp)

### Rule 3: Schema and metadata updates are part of the implementation

If a workstream adds a gameplay feature or service contract, it should also add:

- schema registration
- feature metadata registration
- acceptance notes

## 8. What A Handoff Must Contain

Every handoff should answer:

1. What changed?
2. Which files changed?
3. Which interfaces changed?
4. Which tests ran?
5. What remains risky?
6. What should the next Codex do first?

If any of those are missing, the handoff is incomplete.

## 9. Integration Standard

The integrator Codex should not merge based on "looks good".

It should confirm:

- acceptance checklist items are satisfied
- tests passed
- no forbidden dependency direction was added
- coordination files were updated
- handoff note is complete
- no overlapping ownership conflict remains open

## 10. Multi-Codex Success Criteria

The workflow is working if:

- any Codex can find the current milestone quickly
- any Codex can see who owns each workstream
- handoff artifacts explain enough to continue work without rereading the whole repo
- architectural decisions are documented instead of living only in chat history
