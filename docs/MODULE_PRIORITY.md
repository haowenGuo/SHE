# Module Priority

This file ranks the engine workstreams by:

- **importance**: how strongly the whole project depends on the workstream
- **difficulty**: how hard it is to design and implement correctly

Scales:

- Importance: `S`, `A`, `B`, `C`
- Difficulty: `S`, `A`, `B`, `C`

`S` means highest.

## 1. Full W01-W11 Table

| Workstream | Module | Plane | Importance | Difficulty | Why It Matters | Recommended Timing |
| --- | --- | --- | --- | --- | --- | --- |
| W01 | Gameplay Core | control plane | S | A | Every future game rule, trigger, command, and event flow depends on it | Start first |
| W02 | Data Core | control plane | S | A | Schema and data contracts are the difference between structured authoring and guesswork | Start first wave |
| W03 | Diagnostics + AI Context | control plane | S | B | This is what makes the engine inspectable by humans and Codex | Start first wave |
| W04 | Scripting Host | authoring plane | A | A | Fast gameplay iteration needs a script boundary, but it should sit on top of stronger contracts | Start after W01 and W02 stabilize |
| W05 | Scene + ECS | runtime plane | S | S | The world model is foundational and expensive to replace later | Start second wave |
| W06 | Asset Pipeline | runtime/data plane | A | A | Runtime modules need stable asset IDs, metadata, and loader boundaries | Start second wave |
| W07 | Platform + Input | runtime plane | A | B | The engine cannot become a real runtime until windowing, timing, and input are real | Start second wave |
| W08 | Renderer2D | runtime plane | S | S | Rendering is the visible proof that the engine can execute a world model | Start third wave |
| W09 | Physics2D | runtime plane | A | A | Deterministic motion, collision, and fixed-step callbacks are central to many 2D games | Start third wave |
| W10 | Audio Runtime | runtime plane | B | B | Audio matters for playability and feedback, but it can trail the visual runtime slightly | Start third wave |
| W11 | UI + Debug Tools | tooling plane | B | B | Debug visibility becomes high leverage after runtime systems exist to inspect | Start fourth wave |

## 2. Why W00-W03 Was Not Enough

`W00-W03` covers the project's control plane:

- coordination
- gameplay contracts
- data contracts
- diagnostics and AI context

That is necessary for an AI-native engine, but it is **not** the full running
engine.

The actual runtime layer also needs:

- `W05 Scene + ECS`
- `W06 Asset Pipeline`
- `W07 Platform + Input`
- `W08 Renderer2D`
- `W09 Physics2D`
- `W10 Audio Runtime`

Without those workstreams, the repository has a strong authoring architecture
but not yet a complete playable engine runtime.

## 3. Recommended Development Waves

### Wave A - Foundation contracts

Start first:

- `W01 Gameplay Core`
- `W02 Data Core`
- `W03 Diagnostics + AI Context`

These create the project's stable control plane.

### Wave B - World and runtime spine

Start when Wave A has early shape:

- `W05 Scene + ECS`
- `W06 Asset Pipeline`
- `W07 Platform + Input`

These define the running engine's backbone.

### Wave C - Actual playable runtime

Start when Wave B has usable interfaces:

- `W08 Renderer2D`
- `W09 Physics2D`
- `W10 Audio Runtime`

These make the engine visibly and audibly functional.

### Wave D - Higher-level authoring and inspection

Start after the runtime spine is real:

- `W04 Scripting Host`
- `W11 UI + Debug Tools`

These amplify iteration speed but should not be used to guess missing runtime
contracts.

## 4. If The Goal Is "Playable Fast"

If the team's goal is the fastest path to a real 2D runtime rather than the
fastest path to scripting, the recommended order is:

1. `W01 Gameplay Core`
2. `W02 Data Core`
3. `W03 Diagnostics + AI Context`
4. `W05 Scene + ECS`
5. `W07 Platform + Input`
6. `W06 Asset Pipeline`
7. `W08 Renderer2D`
8. `W09 Physics2D`
9. `W10 Audio Runtime`
10. `W04 Scripting Host`
11. `W11 UI + Debug Tools`

This order keeps the engine AI-friendly while still converging toward a playable
runtime quickly.

## 5. Workstream Notes

### W01 - Gameplay Core

What should land:

- command registry
- command execution path
- event bus
- timer dispatch
- stable gameplay context conventions

### W02 - Data Core

What should land:

- YAML loading contract
- schema registration
- structured validation result model
- gameplay-readable data registry

### W03 - Diagnostics + AI Context

What should land:

- richer frame and phase tracing
- command/event capture
- structured authoring context export
- integration-focused diagnostics summaries

### W04 - Scripting Host

What should land:

- script module loading boundary
- host lifecycle hooks
- binding registration conventions
- one bootstrap integration example

### W05 - Scene + ECS

What should land:

- stable entity identity model
- component storage/query conventions
- transform ownership rules
- scene update and lifetime hooks

### W06 - Asset Pipeline

What should land:

- asset identifiers and metadata model
- loader registration
- asset handle lifetime rules
- renderer/audio-friendly resource contracts

### W07 - Platform + Input

What should land:

- SDL3-backed window loop
- keyboard and pointer input collection
- frame timing and event pumping integration
- platform service replacement for the null bootstrap

### W08 - Renderer2D

What should land:

- first real 2D render backend
- camera and sprite submission path
- texture/material handle integration
- frame begin/end ownership clarified

### W09 - Physics2D

What should land:

- Box2D runtime boundary
- body and collider lifetime management
- fixed-step simulation integration
- collision callbacks into gameplay events

### W10 - Audio Runtime

What should land:

- miniaudio-backed playback path
- sound and music asset playback contract
- channel/group ownership rules
- gameplay-triggered audio events

### W11 - UI + Debug Tools

What should land:

- debug overlay or panel system
- runtime counters and traces
- scene/physics/render inspection hooks
- sandbox and debug tooling entry points

## 6. Follow-On Tracks

After `W01-W11` are meaningfully in place, the next logical tracks are:

- save/load
- editor tooling
- first vertical slice game

Those are real milestones, but they should not distract from finishing the
core runtime and authoring planes first.
