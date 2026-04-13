# Next Phase Runtime Launch

Issued by: `W00`
Date: `2026-04-13`
Phase: `Core Runtime Wave`

## Phase Gate Result

The first-wave foundation is complete on `main`.

Integrated foundation workstreams:

- `W01` Gameplay Core
- `W02` Data Core
- `W03` Diagnostics + AI Context

The accepted mainline now provides:

- gameplay command/event/timer contracts
- schema-first data contracts with validation/reporting
- structured diagnostics and authoring-context export
- smoke and contract tests that cover the combined baseline

## Launch Order

Start the next runtime wave in this order:

1. `W07` Platform + Input
2. `W05` Scene + ECS
3. `W06` Asset Pipeline

Do not launch `W08`, `W09`, `W10`, or `W11` until the runtime spine above has
stabilized its first public contracts.

## Why This Order

### W07 First

`W07` should establish the first real application loop, window, and input path.
It is the thinnest route to replacing placeholder platform behavior with a real
runtime shell.

### W05 Second

`W05` should define the world model, scene lifetime, and component-query
contracts that renderer and physics will later consume.

### W06 Third

`W06` should sit on top of the W02 data baseline and the W05 world boundary to
define asset identifiers, metadata, and loader contracts.

## Workstream Docs To Use

- `coordination/WORKSTREAMS/W07_platform-input.md`
- `coordination/WORKSTREAMS/W05_scene-ecs.md`
- `coordination/WORKSTREAMS/W06_asset-pipeline.md`

## W00 Rule For The New Phase

`W00` should keep `main` as the coordination source of truth and should prefer:

- interface review
- integration sequencing
- architecture decisions
- runtime contract stabilization

before starting any additional broad module implementation directly on `main`.
