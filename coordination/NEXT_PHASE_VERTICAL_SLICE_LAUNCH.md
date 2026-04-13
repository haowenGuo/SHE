# Next Phase Vertical Slice Launch

Issued by: `W00`
Date: `2026-04-13`
Phase: `Vertical Slice Wave`

## Phase Gate Result

The scripting-and-debug wave is complete on `main`.

Integrated authoring/debug workstreams:

- `W04` Scripting Host
- `W11` UI + Debug Tools

The accepted mainline now provides:

- a structured script-host boundary for feature-owned script modules
- a stable runtime debug report that aggregates renderer, physics, diagnostics,
  and authoring-context state
- the previously integrated gameplay, data, diagnostics, platform, scene,
  assets, renderer, physics, and audio baselines
- smoke and contract tests that cover the combined W01-W11 engine stack end to
  end

## Immediate Launch Set

Start the next phase with:

1. `W12` Vertical Slice Game

## Why W12 Now

`W12` no longer needs to guess about missing engine surfaces.

It can now build directly on:

- `W01` gameplay commands, events, and timers
- `W02` schema-first data contracts
- `W03` diagnostics and authoring-context exports
- `W04` structured scripting host contracts
- `W05` scene/entity lifetime and query boundaries
- `W06` asset metadata, loaders, and handle contracts
- `W07` SDL3 window/input runtime shell
- `W08` visible 2D renderer
- `W09` Box2D runtime baseline
- `W10` gameplay-routed audio playback
- `W11` debug/runtime inspection surfaces

That means the next slice can focus on proving one small playable game loop
instead of inventing more engine scaffolding.

## Workstream Doc To Use

- `coordination/WORKSTREAMS/W12_vertical-slice.md`

## W00 Rule For The New Phase

`W00` should keep `main` as the coordination source of truth and prefer:

- proving the integrated engine stack through one coherent playable slice
- reusing accepted scripting/debug/runtime surfaces instead of adding temporary
  slice-only shortcuts
- adding only the minimum new contracts needed for the slice to stay readable,
  testable, and extensible
