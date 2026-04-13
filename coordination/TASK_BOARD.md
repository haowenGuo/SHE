# Task Board

Status values:

- `backlog`
- `ready`
- `in_progress`
- `blocked`
- `ready_for_integration`
- `integrated`

`W00/main` is the authoritative location for this file.

| ID | Workstream | Module Focus | Status | Owner | Branch / Worktree | Dependencies | Deliverable |
| --- | --- | --- | --- | --- | --- | --- | --- |
| W00 | Coordination Foundation | docs + coordination | integrated | current-codex | `main` / `F:\SHE-workspace\SHE` | none | workflow docs, launch plan, integration control |
| W01 | Gameplay Core | `Engine/Gameplay` | integrated | w01-codex | `codex/w01/gameplay-core` / `SHE-w01-gameplay` | W00 | command registry, executor, event bus, timers |
| W02 | Data Core | `Engine/Data` | in_progress | w02-codex | `codex/w02/data-core` / `SHE-w02-data` | W00 | YAML load + schema validation contract |
| W03 | Diagnostics + AI Context | `Engine/Diagnostics`, `Engine/AI` | in_progress | w03-codex | `codex/w03/diagnostics-ai` / `SHE-w03-diagnostics` | W00, W02 | richer traces and authoring export |
| W04 | Scripting Host | `Engine/Scripting` | backlog | unassigned | `codex/w04/scripting-host` / `SHE-w04-scripting` | W01, W02 | stable script host boundary |
| W05 | Scene + ECS | `Engine/Scene` | backlog | unassigned | `codex/w05/scene-ecs` / `SHE-w05-scene` | W01, W02 | entity registry, component queries, world model |
| W06 | Asset Pipeline | `Engine/Assets` | backlog | unassigned | `codex/w06/asset-pipeline` / `SHE-w06-assets` | W02, W05 | asset IDs, metadata, loader contracts |
| W07 | Platform + Input | `Engine/Platform` | ready | unassigned | `codex/w07/platform-input` / `SHE-w07-platform` | W00 | SDL3 window loop and input path |
| W08 | Renderer2D | `Engine/Renderer` | backlog | unassigned | `codex/w08/renderer2d` / `SHE-w08-renderer` | W05, W06, W07 | sprite renderer and camera path |
| W09 | Physics2D | `Engine/Physics` | backlog | unassigned | `codex/w09/physics2d` / `SHE-w09-physics` | W01, W05 | Box2D fixed-step integration |
| W10 | Audio Runtime | `Engine/Audio` | backlog | unassigned | `codex/w10/audio-runtime` / `SHE-w10-audio` | W01, W06, W07 | miniaudio playback runtime |
| W11 | UI + Debug Tools | `Engine/UI` | backlog | unassigned | `codex/w11/ui-debug` / `SHE-w11-ui-debug` | W03, W08, W09 | ImGui debug surfaces and runtime panels |
| W12 | Vertical Slice Game | `Game/Features/*` | backlog | unassigned | `codex/w12/vertical-slice` / `SHE-w12-vertical-slice` | W01-W11 | playable small game |
