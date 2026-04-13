# Next Phase Authoring + Debug Launch

Issued by: `W00`
Date: `2026-04-13`
Phase: `Scripting + Debug Wave`

## Phase Gate Result

The playable-runtime wave is complete on `main`.

Integrated playable-runtime workstreams:

- `W08` Renderer2D
- `W09` Physics2D
- `W10` Audio Runtime

The accepted mainline now provides:

- a visible SDL3-presented 2D renderer with camera/material/frame snapshot coverage
- a Box2D-backed physics runtime keyed by accepted scene entity contracts
- a miniaudio-backed playback runtime routed through accepted gameplay events
- smoke and contract tests that cover the combined runtime stack end to end

## Immediate Parallel Launch Set

Start the next phase with these two workstreams in parallel:

1. `W11` UI + Debug Tools
2. `W04` Scripting Host

Keep `W12` Vertical Slice Game queued until those authoring and inspection
surfaces have settled on `main`.

## Why This Launch Set

### W11 Now

`W11` finally has real runtime surfaces to inspect:

- diagnostics and AI-context from `W03`
- visible renderer state from `W08`
- physics runtime state from `W09`
- live gameplay/data/asset/runtime services already integrated before this wave

That makes the next debug UI pass about surfacing accepted engine truth rather
than inventing placeholder tooling.

### W04 In Parallel

`W04` no longer has to target a mostly stubbed runtime. It can bind the script
host boundary against accepted gameplay, data, scene, asset, renderer, physics,
and audio contracts while staying narrowly focused on the scripting surface
itself.

### W12 After Them

`W12` will move faster once:

- `W11` gives the slice usable inspection panels
- `W04` gives the slice a stable script-facing extension boundary

Launching the vertical slice after those surfaces exist reduces thrash in game
feature code.

## Workstream Docs To Use

- `coordination/WORKSTREAMS/W11_ui-debug.md`
- `coordination/WORKSTREAMS/W04_scripting-host.md`

## W00 Rule For The New Phase

`W00` should keep `main` as the coordination source of truth and prefer:

- accepted debug surfaces over ad hoc inspector backdoors
- stable scripting boundaries over feature-specific shortcuts
- shared runtime inspection contracts that reflect the integrated renderer,
  physics, and audio baselines
- keeping `W12` off the critical path until the scripting and debug-tooling
  layer is real
