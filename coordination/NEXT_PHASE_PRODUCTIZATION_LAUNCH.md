# Next Phase Productization Launch

Issued by: `W00`
Date: `2026-04-14`
Phase: `Post-Slice Productization Wave`

## Phase Gate Result

The vertical-slice wave is complete on `main`.

Integrated baseline now available to every new workstream:

- `W01-W11` engine contracts are already accepted on `main`
- `W12` is integrated on `main` as the default playable game entry
- `main` configures, builds, and passes the full 14-test preset suite on the
  accepted vertical-slice baseline
- the repository now has one coherent playable slice, so the next step is to
  improve that shipped experience rather than prove another prototype

## Immediate Launch Set

Start the next phase with these four parallel workstreams:

1. `W13` Polish + Feel
2. `W14` Balance + Progression
3. `W15` Content Expansion
4. `W16` Release Hardening

These are parallel windows, not serial gates.

## Why This Launch Shape

`W13` should move first in parallel because the existing slice is already
playable, but its moment-to-moment readability and response quality are now the
highest-value improvements for every later playtest.

`W14` can run beside `W13` because the current loop is already data-driven.
That means pacing, pickup values, hazard pressure, and round timing can be
tuned without reopening engine architecture.

`W15` should start now as an additive-content lane rather than waiting for the
entire tuning pass to finish. The accepted slice already has feature-owned data,
scripts, and assets, so new encounter content can land as bounded authored
extensions while `W14` stabilizes the baseline numbers.

`W16` belongs in the same launch set because hardening is now a first-class
product task. Build stability, launch sanity, regression coverage, runbooks,
and delivery hygiene should mature alongside the game rather than becoming a
last-minute scramble.

## Workstream Docs To Use

- `coordination/WORKSTREAMS/W13_polish-feel.md`
- `coordination/WORKSTREAMS/W14_balance-progression.md`
- `coordination/WORKSTREAMS/W15_content-expansion.md`
- `coordination/WORKSTREAMS/W16_release-hardening.md`

## W00 Rules For The New Phase

`W00` should keep `main` as the coordination source of truth and prefer:

- improving the integrated slice instead of branching into a second prototype
- preserving narrow ownership boundaries so parallel work stays mergeable
- treating product feel, content depth, and delivery readiness as equal parts
  of the same phase
- rejecting casual engine-contract churn when the work can stay feature-local
