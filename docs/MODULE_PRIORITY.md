# Module Priority

This file ranks engine modules by:

- **importance**: how strongly the whole project depends on the module
- **difficulty**: how hard it is to design and implement correctly

Scales:

- Importance: `S`, `A`, `B`, `C`
- Difficulty: `S`, `A`, `B`, `C`

`S` means highest.

## 1. Priority Table

| Workstream | Module | Importance | Difficulty | Why It Matters | Recommended Timing |
| --- | --- | --- | --- | --- | --- |
| W01 | Gameplay Core | S | A | Every future game rule, script, trigger, and AI-authored behavior depends on it | Start first |
| W02 | Data Core | S | A | Without schema and loading contracts, gameplay becomes ad hoc and hard for Codex to extend | Start first wave |
| W03 | Diagnostics + AI Context | S | B | This is what makes the engine explainable to Codex and to humans | Start first wave |
| W04 | Scripting Host | A | A | Critical for rapid gameplay iteration and Codex-authored behavior | Start second wave |
| W05 | Scene + ECS | S | S | The world model is foundational and expensive to change later | Start second wave after Gameplay/Data direction is stable |
| W06 | Asset Pipeline | A | A | Required for real content flow and data/resource stability | Start second wave |
| W07 | Platform + Input | A | B | Necessary for real runtime interaction, but easier to replace than world/gameplay contracts | Start third wave |
| W08 | Renderer2D | A | S | Large surface area and many downstream dependencies | Start third wave |
| W09 | Physics2D | B | A | Important for many game types, but should sit on top of stronger gameplay/scene contracts | Start fourth wave |
| W10 | UI + Debug Tools | B | B | Valuable for iteration, but depends on other systems being meaningful first | Start fourth wave |
| W11 | Save/Load | B | A | Important once world/data contracts stabilize | Later |
| W12 | First Vertical Slice Game | S | A | The true validation of the engine architecture | Continuous milestone capstone |

## 2. Recommended Initial Order

The best starting order is:

1. `Gameplay Core`
2. `Data Core`
3. `Diagnostics + AI Context`
4. `Scripting Host`
5. `Scene + ECS`
6. `Asset Pipeline`
7. `Platform + Input`
8. `Renderer2D`
9. `Physics2D`
10. `UI + Debug Tools`

## 3. Why This Order Is Better Than Starting With Renderer

For this repository, the engine is supposed to be **AI-native**, not just
graphically functional.

That means the first irreversible decisions should be about:

- gameplay structure
- data contracts
- context export
- feature boundaries

If rendering comes first, you can get pixels fast, but you risk building a
runtime that is hard for Codex to extend later.

## 4. First Parallel Batch

The recommended first parallel batch for multiple Codex sessions is:

- `W01 Gameplay Core`
- `W02 Data Core`
- `W03 Diagnostics + AI Context`

These three can progress in parallel with moderate coordination and form the
minimum platform for AI-assisted gameplay authoring.

## 5. Module Notes

### Gameplay Core

What should land:

- command registry
- command executor
- event bus
- timer dispatcher
- stable gameplay context conventions

### Data Core

What should land:

- YAML loading contract
- schema validation contract
- structured error reporting
- data registry

### Diagnostics + AI Context

What should land:

- richer phase tracing
- command/event trace capture
- authoring context export improvements
- integration with acceptance flow

