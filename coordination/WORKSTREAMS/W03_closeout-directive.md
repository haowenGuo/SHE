# W03 Closeout Directive

Issued by: `W00`
Date: `2026-04-13`
Workstream: `W03`
Owner: `w03-codex`
Branch / Worktree: `codex/w03/diagnostics-ai` / `F:\SHE-workspace\SHE-w03-diagnostics`
Target Status: `ready_for_integration`

## Read First

1. `coordination/WORKSTREAMS/W03_diagnostics-ai.md`
2. `coordination/HANDOFFS/2026-04-13-w03-diagnostics-context-structure.md`
3. `coordination/HANDOFFS/2026-04-13-w03-ai-context-data-registry-alignment.md`
4. `coordination/HANDOFFS/2026-04-13-w03-rebase-final-merge.md`
5. `coordination/HANDOFFS/2026-04-13-w02-data-contract-layer.md`
6. `coordination/INTEGRATION_REPORT.md`
7. `docs/ARCHITECTURE_DECISIONS.md`

## Mission

Produce the final first-wave closeout merge for diagnostics and AI context on
top of:

- the integrated W01 gameplay baseline on `main`
- the accepted W02 data-contract commit

W03 is the final merge owner for diagnostics, AI-context export, and shared
smoke-test reconciliation.

This is a closeout assignment, not a new feature sprint.

## What W03 Must Preserve

W03 must preserve the integrated W01 gameplay baseline:

- `GameplayCommandDescriptor`
- `GameplayEvent`
- `GameplayEventHandler`
- `IGameplayService::RegisterCommand(...)`
- `IGameplayService::HasCommand(...)`
- `IGameplayService::SubscribeToEvent(...)`
- `IGameplayService::UnsubscribeFromEvent(...)`
- diagnostics consuming `BuildGameplayDigest()`

W03 must also preserve the accepted W02 data baseline.

Do **not** narrow `IDataService` back to the older schema-only API.

The final W03 merge must keep the accepted W02 data-side contract visible and
build its exporter/diagnostics view on top of it.

## W03 Ownership For This Closeout

W03 owns final merge resolution for:

- `Engine/Diagnostics/*`
- `Engine/AI/*`
- `Engine/Core/Source/Application.cpp`
- `Tests/Source/SmokeTests.cpp`
- `docs/AI_CONTEXT.md`

W03 may touch data files only when required to preserve the accepted W02
contract during rebase/merge, not to redesign the data system.

## Required Actions

1. Rebase `codex/w03/diagnostics-ai` onto the latest `main`.
2. Rebase or merge on top of the accepted W02 commit once W02 checkpoints it.
3. Keep ownership of the final merge for these overlap files:
   - `Engine/AI/Source/AuthoringAiService.cpp`
   - `docs/AI_CONTEXT.md`
   - `Tests/Source/SmokeTests.cpp`
4. Preserve W02's richer data contract while finalizing:
   - diagnostics frame report structure
   - AI context structure
   - data registry visibility inside AI context
   - smoke coverage for W01 + W02 + W03 together
5. Keep `Application.cpp` changes minimal and observability-focused.
6. Create one final reviewable commit on `codex/w03/diagnostics-ai`.
7. Re-run:
   - `cmake --preset default`
   - `cmake --build --preset build-default`
   - `ctest --preset test-default`
8. Write a final handoff note that explicitly states the branch now sits on the
   W01 gameplay baseline and the accepted W02 data baseline.

## Explicit Non-Goals

Do not spend this closeout on:

- inventing a second data contract
- replacing W02 semantics with AI-only summaries
- adding new gameplay systems
- starting W05/W06/W07 work
- introducing editor/UI/runtime features outside diagnostics and AI context

## Done Means

W03 is done for this phase only when all of these are true:

1. the branch preserves W01 gameplay baseline behavior
2. the branch preserves the accepted W02 data contract rather than narrowing it
3. diagnostics reports remain structured and gameplay-visible
4. AI context exports module counts, schema catalog, data registry, gameplay
   state, and latest frame report in one stable shape
5. smoke/build/test pass on the merged W01 + W02 + W03 surface
6. one final reviewable commit exists on `codex/w03/diagnostics-ai`
7. the handoff is complete and no longer asks W00 to guess how W02 and W03 were
   reconciled

## Required Report Back To W00

When done, report only this:

1. commit hash
2. tests run
3. exact shared files changed
4. confirmation that the final branch preserves the accepted W02 data contract
5. recommendation: `ready_for_integration` or `blocked`
