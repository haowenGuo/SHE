# Task Board

Status values:

- `backlog`
- `ready`
- `in_progress`
- `blocked`
- `ready_for_integration`
- `integrated`

| ID | Workstream | Module Focus | Status | Owner | Branch / Worktree | Dependencies | Deliverable |
| --- | --- | --- | --- | --- | --- | --- | --- |
| W00 | Coordination Foundation | docs + coordination | integrated | current-codex | local | none | workflow docs and templates |
| W01 | Gameplay Core | `Engine/Gameplay` | ready | unassigned | `codex/w01/gameplay-core` | W00 | command registry, executor, event bus |
| W02 | Data Core | `Engine/Data` | ready | unassigned | `codex/w02/data-core` | W00 | YAML load + schema validation contract |
| W03 | Diagnostics + AI Context | `Engine/Diagnostics`, `Engine/AI` | ready | unassigned | `codex/w03/diagnostics-ai` | W00 | richer traces and authoring export |
| W04 | Scripting Host | `Engine/Scripting` | backlog | unassigned | `codex/w04/scripting-host` | W01, W02 | Lua host boundary |
| W05 | Scene + ECS | `Engine/Scene` | backlog | unassigned | `codex/w05/scene-ecs` | W01, W02 | stronger world model |
| W06 | Asset Pipeline | `Engine/Assets` | backlog | unassigned | `codex/w06/asset-pipeline` | W02, W05 | metadata + load contract |
| W07 | Platform + Input | `Engine/Platform` | backlog | unassigned | `codex/w07/platform-input` | W00 | SDL3 platform path |
| W08 | Renderer2D | `Engine/Renderer` | backlog | unassigned | `codex/w08/renderer2d` | W05, W06, W07 | sprite renderer |
| W09 | Physics2D | `Engine/Physics` | backlog | unassigned | `codex/w09/physics2d` | W01, W05 | Box2D integration |
| W10 | UI + Debug Tools | `Engine/UI` | backlog | unassigned | `codex/w10/ui-debug` | W03, W08 | debug panels |
| W12 | Vertical Slice Game | `Game/Features/*` | backlog | unassigned | `codex/w12/vertical-slice` | W01-W10 | playable small game |

