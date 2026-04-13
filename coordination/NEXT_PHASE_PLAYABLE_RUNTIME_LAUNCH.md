# Next Phase Playable Runtime Launch

Issued by: `W00`
Date: `2026-04-13`
Phase: `Playable Runtime Wave`

## Phase Gate Result

The runtime spine is complete on `main`.

Integrated runtime-spine workstreams:

- `W07` Platform + Input
- `W05` Scene + ECS
- `W06` Asset Pipeline

The accepted mainline now provides:

- a real SDL3-backed window, event, timing, and input shell
- stable scene/entity lifetime and query contracts
- stable asset identity, metadata, loader, and handle contracts
- smoke and contract tests that cover the combined runtime spine

## Launch Order

Start the playable-runtime wave in this order:

1. `W08` Renderer2D
2. `W09` Physics2D
3. `W10` Audio Runtime

`W04` Scripting Host may now run as a lower-priority authoring track because
the runtime spine is real, but it is not the primary next-wave path.

Do not launch `W11` UI + Debug Tools until `W08` and `W09` have exposed enough
runtime surface to inspect.

## Why This Order

### W08 First

`W08` is now the shortest path from accepted runtime contracts to visible
engine proof. It consumes the integrated scene, asset, and platform baselines
without needing to invent those contracts itself.

### W09 Second

`W09` depends on the accepted gameplay and scene boundaries. Launching it after
`W08` keeps visual runtime proof moving while fixed-step and collision rules
settle against the now-stable world model.

### W10 Third

`W10` depends on the accepted asset and platform baselines and can trail the
first visible runtime proof slightly without blocking the engine from becoming
demonstrably playable.

## Workstream Docs To Use

- `coordination/WORKSTREAMS/W08_renderer2d.md`
- `coordination/WORKSTREAMS/W09_physics2d.md`
- `coordination/WORKSTREAMS/W10_audio-runtime.md`
- `coordination/WORKSTREAMS/W04_scripting-host.md`

## W00 Rule For The New Phase

`W00` should keep `main` as the coordination source of truth and should prefer:

- runtime contract stabilization
- renderer/physics/audio integration sequencing
- explicit shared-interface review
- test growth with each newly-real runtime service

before broadening into higher-level tooling work.
