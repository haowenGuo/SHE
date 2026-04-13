# W02 Closeout Directive

Issued by: `W00`
Date: `2026-04-13`
Workstream: `W02`
Owner: `w02-codex`
Branch / Worktree: `codex/w02/data-core` / `F:\SHE-workspace\SHE-w02-data`
Target Status: `ready_for_integration`

## Read First

1. `coordination/WORKSTREAMS/W02_data-core.md`
2. `coordination/HANDOFFS/2026-04-13-w02-data-contract-layer.md`
3. `coordination/INTEGRATION_REPORT.md`
4. `docs/ARCHITECTURE_DECISIONS.md`

## Mission

Lock the accepted W02 data-contract slice as the stable data baseline for the
first-wave closeout.

This is a closeout assignment, not an expansion assignment.

Do not broaden scope into new data features once the accepted slice is
checkpointed and verified.

## What W02 Must Preserve

The accepted W02 branch is the richer data-contract baseline.

The closeout commit must preserve these `IDataService` contracts in
`Engine/Core/Include/SHE/Core/RuntimeServices.hpp`:

- `RegisterSchema(DataSchemaContract)`
- `LoadRecord(const DataLoadRequest&)`
- `HasTrustedRecord(std::string_view)`
- `GetRecordCount()`
- `GetTrustedRecordCount()`
- `ListSchemas()`
- `ListRecords()`
- `DescribeRegistry()`

The closeout commit must also preserve:

- machine-readable validation issues
- trusted vs untrusted record visibility
- bootstrap schema examples
- focused data contract tests

## W02 Ownership For This Closeout

W02 owns final review and checkpointing for:

- `Engine/Data/*`
- data-side schema examples
- `docs/SCHEMAS/*`
- data-side tests
- the data-contract portion of `RuntimeServices.hpp`

W02 does **not** own the final exporter shape in:

- `Engine/AI/*`
- `Engine/Diagnostics/*`
- `Engine/Core/Source/Application.cpp`

If branch-local edits still touch AI-context/export formatting, remove or avoid
extending them. W03 owns the final exporter merge.

## Required Actions

1. Review the current staged W02 branch state and make sure it reflects only the
   accepted data-contract slice.
2. Keep the accepted `IDataService` contract intact.
3. Do not add new feature work such as:
   - nested schema typing
   - runtime file-loaded schema definitions
   - editor/live reload
   - AI-specific formatting beyond what the data contract must expose
4. Ensure the handoff remains accurate for the final accepted slice.
5. Create one reviewable commit on `codex/w02/data-core`.
6. Re-run:
   - `cmake --preset default`
   - `cmake --build --preset build-default`
   - `ctest --preset test-default`
7. Report the exact commit hash back to `W00`.

## Explicit Non-Goals

Do not spend this closeout on:

- inventing a second exporter path
- redesigning diagnostics output
- starting W05/W06/W07 work
- improving validation breadth beyond the already accepted slice

## Done Means

W02 is done for this phase only when all of these are true:

1. the accepted data-contract slice is committed on `codex/w02/data-core`
2. the branch still exposes the richer W02 data API
3. build and tests pass in the W02 worktree
4. the handoff file is accurate
5. W03 can rebase onto the commit without guessing which data contract is
   authoritative

## Required Report Back To W00

When done, report only this:

1. commit hash
2. tests run
3. whether any shared interface changed after the accepted handoff
4. confirmation that `Engine/AI/*` was not used as the final exporter owner
