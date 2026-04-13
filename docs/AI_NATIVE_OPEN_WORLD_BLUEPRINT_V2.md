# AI-Native 2D Open World Blueprint V2

This document defines the next architecture target for SHE as an AI-native 2D
open-world engine.

The key design constraint is not raw runtime power. The key constraint is
authoring scale under limited AI context.

The engine must let a top-level creator AI build a whole game through:

- world assembly
- rule assembly
- content assembly
- local validation
- micro-edits to shared bases only when necessary

The engine should never assume that one AI pass can read the whole project.

## 1. Hard Constraints

### 1.1 Single-Agent Budget

Default design target:

- one creator AI turn should succeed with local context only
- one task should usually touch `3-8` files
- one task should usually stay under `10,000` changed lines
- one task should ideally modify at most `1` base contract and `1-2` content packs

Multi-agent workflows are optional accelerators, not baseline assumptions.

### 1.2 Authoring Reality

The creator AI should spend most of its time in:

- structured data
- pack manifests
- script glue
- content validation

The creator AI should spend little time in:

- deep engine rewrites
- cross-cutting runtime surgery
- full-project reverse engineering

### 1.3 Runtime Reality

The engine must support:

- long-form play sessions
- region streaming
- persistent world state
- scalable NPC behavior
- scalable content growth

without forcing most new game work into `Engine/*`.

## 2. Core Thesis

An AI-native open-world engine is not primarily a renderer, ECS, or scripting
host.

It is a **composable world authoring system** built on top of a stable runtime.

The engine succeeds when:

1. the runtime kernel stays small and stable
2. reusable "bases" expose strong but finite command sets
3. most gameplay creation happens by assembling packs
4. AI context is query-based, not full-project text dumping
5. every pack and base can be locally validated

## 3. North Star Architecture

The system is organized into five layers.

### 3.1 Kernel Layer

Purpose:

- own frame sequencing
- own world state lifetime
- own persistence and streaming boundaries
- own diagnostics and query infrastructure

Stable modules:

- `Core`
- `Gameplay`
- `World`
- `Assets`
- `Renderer`
- `Physics`
- `Audio`
- `UI`
- `Scripting`
- `Diagnostics`
- `SaveGame`
- `Streaming`
- `Validation`
- `ContextQuery`

Rule:

- game-specific logic should not live here

### 3.2 Base Layer

Purpose:

- expose high-leverage reusable authoring primitives
- give AI a finite command language for each creative domain

Examples:

- `RegionBase`
- `TilemapBase`
- `AvatarBase`
- `AnimationBase`
- `NPCBase`
- `InteractionBase`
- `QuestBase`
- `DialogueBase`
- `EncounterBase`
- `ItemEconomyBase`

Rule:

- each base should feel like a tiny engine inside the engine

### 3.3 Pack Layer

Purpose:

- store concrete world, rule, and content instances

Examples:

- a town
- a forest region
- one named NPC
- one avatar set
- one dialogue tree
- one quest chain
- one encounter table

Rule:

- most game creation should happen here

### 3.4 Composition Layer

Purpose:

- assemble packs into a coherent world
- resolve links, dependencies, and routing

Examples:

- world manifests
- region graph
- progression graph
- encounter routing
- faction routing
- pack dependency resolution

### 3.5 Validation Layer

Purpose:

- make every change locally testable
- stop broken packs before runtime integration

Examples:

- schema validation
- reference validation
- preview validation
- simulation smoke validation
- save/load roundtrip validation
- budget validation

## 4. Runtime Service Expansion

The current `RuntimeServices` backbone should be preserved, but the runtime
contract needs new first-class services.

### 4.1 Keep

Keep and continue evolving:

- `IWindowService`
- `IAssetService`
- `IGameplayService`
- `IRendererService`
- `IPhysicsService`
- `IAudioService`
- `IUiService`
- `IScriptingService`
- `IDiagnosticsService`
- `IAIService`

### 4.2 Replace or Evolve

#### `ISceneService` -> `IWorldService`

`SceneWorld` is too thin for open-world authoring.

`IWorldService` should own:

- entity identity
- component storage
- prefab instancing
- region residency
- chunk activation
- world object tagging
- transform hierarchy
- streaming hooks

#### `IDataService` -> `IContentService`

Current data contracts are useful but shallow.

`IContentService` should own:

- schema registration
- typed content loading
- record queries
- pack loading
- dependency resolution
- assembled object descriptors
- content diff summaries

#### `IAIService` split with `IContextQueryService`

The current AI export path is too monolithic for open-world scale.

`IContextQueryService` should own:

- scoped context queries
- pack inspection
- base contract summaries
- dependency lookup
- preview lookup
- validation result lookup

`IAIService` can remain as the high-level authoring integration surface, but it
should consume query services instead of constructing one giant report.

### 4.3 Add

Add these services:

- `ISaveGameService`
- `IStreamingService`
- `IValidationService`
- `IPrefabService`
- `IContextQueryService`

Optional later:

- `INavigationService`
- `IAnimationService`
- `IDialogueService`
- `IQuestService`

## 5. Base Contract Standard

Every base must implement the same authoring shape.

A module is not considered an AI-native base unless it provides all of the
following.

### 5.1 Required Parts

1. `Schema`
2. `Finite Command Set`
3. `Assembler`
4. `Runtime Adapter`
5. `Query API`
6. `Validator`
7. `Preview Path`

### 5.2 Why This Matters

This keeps every base:

- readable
- composable
- testable
- locally editable

without requiring the creator AI to inspect unrelated systems.

### 5.3 Finite Command Set Rule

Each base should expose a small, stable verb vocabulary.

Example:

- do not ask AI to write arbitrary NPC logic from scratch
- do ask AI to choose a policy, schedule, role, memory shape, and dialogue binding

The base should provide expressive composition, not unlimited scripting.

## 6. First-Wave Bases

The first implementation wave should focus on the minimum set that unlocks a
small open world.

### 6.1 `RegionBase`

Purpose:

- define one traversable world region

Finite commands:

- `set_biome`
- `set_bounds`
- `add_exit`
- `add_poi`
- `set_streaming_group`
- `bind_ambience`
- `bind_encounter_table`

Outputs:

- region descriptor
- chunk layout
- streaming metadata

### 6.2 `TilemapBase`

Purpose:

- define terrain, collision, and visual layers for a region

Finite commands:

- `set_tileset`
- `add_tile_layer`
- `add_collision_layer`
- `add_decal_layer`
- `add_interaction_marker`

Outputs:

- render layers
- collision map
- interaction anchors

### 6.3 `AvatarBase`

Purpose:

- assemble a complete 2D character appearance from reusable parts

Finite commands:

- `set_body_template`
- `set_palette`
- `equip_part`
- `set_render_profile`
- `bind_anim_set`

Outputs:

- avatar descriptor
- render-ready appearance slots

### 6.4 `AnimationBase`

Purpose:

- bind clips and state transitions for 2D characters

Finite commands:

- `set_skeleton_profile`
- `bind_clip`
- `bind_direction_set`
- `set_transition`
- `bind_event_marker`

Outputs:

- animation graph
- state machine
- clip metadata

### 6.5 `NPCBase`

Purpose:

- create a living NPC from finite structured parts

Finite commands:

- `set_persona_profile`
- `set_role`
- `set_home_region`
- `set_daily_schedule`
- `set_behavior_policy`
- `set_memory_slots`
- `bind_avatar`
- `bind_dialogue`
- `bind_quest_hooks`

Outputs:

- NPC runtime descriptor
- schedule graph
- social and quest bindings

### 6.6 `InteractionBase`

Purpose:

- define how the player interacts with world objects

Finite commands:

- `set_prompt`
- `set_activation_shape`
- `set_requirements`
- `set_effects`
- `bind_dialogue`
- `bind_shop`
- `bind_scene_change`

Outputs:

- interaction descriptor
- trigger rules

### 6.7 `QuestBase`

Purpose:

- define quest state progression

Finite commands:

- `set_start_condition`
- `add_objective`
- `set_transition`
- `set_reward`
- `bind_dialogue_gate`
- `bind_world_flag`

Outputs:

- quest graph
- progression states

### 6.8 `DialogueBase`

Purpose:

- define branching dialogue with conditions and effects

Finite commands:

- `add_node`
- `add_choice`
- `set_condition`
- `set_emotion`
- `set_effect`
- `bind_voice_style`

Outputs:

- dialogue graph
- conditional branches

### 6.9 `EncounterBase`

Purpose:

- define region threat and event behavior

Finite commands:

- `set_spawn_pool`
- `set_spawn_budget`
- `set_time_window`
- `set_region_rules`
- `set_boss_gate`
- `bind_rewards`

Outputs:

- encounter table
- spawn routing rules

### 6.10 `ItemEconomyBase`

Purpose:

- define items, loot, shops, and local economic loops

Finite commands:

- `set_item_class`
- `set_stats`
- `set_drop_sources`
- `set_shop_rules`
- `set_recipe`
- `set_value_profile`

Outputs:

- item descriptors
- shop bindings
- drop/economy metadata

## 7. NPC and Animation as First-Class AI Bases

The user-facing design rule is:

- one NPC should not require a new custom gameplay system
- one character animation set should not require renderer surgery

### 7.1 NPC Design Model

An NPC should be assembled from six bounded slots:

- persona
- role
- schedule
- behavior policy
- memory slots
- system bindings

This prevents free-form NPC scripting from becoming the default authoring path.

### 7.2 Avatar and Animation Design Model

The engine should treat 2D character presentation as a composable rig system.

The first production target is not 3D character modeling.
The first production target is a 2D avatar rig pipeline with:

- body templates
- part slots
- palette slots
- directional variants
- clip bindings
- render profiles

This gives AI a bounded way to create many characters without changing the
renderer for every character.

## 8. Pack Architecture

Packs are the main unit of game creation.

### 8.1 Pack Rules

Each pack must be:

- self-describing
- dependency-declared
- previewable
- locally validatable
- safe to edit in isolation

### 8.2 Suggested Directory Layout

```text
Engine/
  Bases/
    RegionBase/
    TilemapBase/
    AvatarBase/
    AnimationBase/
    NPCBase/
    InteractionBase/
    QuestBase/
    DialogueBase/
    EncounterBase/
    ItemEconomyBase/

Game/
  Worlds/
    <WorldId>/
      world.manifest.yml
      region_graph.yml
      enabled_packs.yml

  Packs/
    Regions/
      forest_01/
        pack.yml
        region.yml
        tilemap.yml
        exits.yml
        ambience.yml
        encounters.yml
        validate.spec.yml

    NPCs/
      mayor_lin/
        pack.yml
        npc.yml
        avatar.yml
        schedule.yml
        memory.yml
        bindings.yml
        validate.spec.yml

    Dialogues/
      mayor_intro/
        pack.yml
        dialogue.yml
        validate.spec.yml

    Quests/
      fix_the_gate/
        pack.yml
        quest.yml
        validate.spec.yml

    Encounters/
      forest_night/
        pack.yml
        encounter.yml
        validate.spec.yml

    Items/
      wooden_bow/
        pack.yml
        item.yml
        validate.spec.yml
```

### 8.3 Pack Manifest Standard

Every pack should declare:

- `pack_id`
- `pack_kind`
- `base_ids_used`
- `exports`
- `dependencies`
- `validation_profiles`
- `preview_profiles`

## 9. Query-First AI Context

The system must stop treating AI context as one giant text blob.

The default model should be scoped queries.

### 9.1 Required Query Operations

- `describe_base(base_id)`
- `list_base_commands(base_id)`
- `inspect_pack(pack_id)`
- `list_pack_dependencies(pack_id)`
- `list_dependents(node_id)`
- `preview_pack(pack_id)`
- `validate_pack(pack_id)`
- `simulate_pack(pack_id, profile_id)`
- `search_ids(prefix_or_tag)`
- `diff_pack(pack_id, against_revision)`

### 9.2 Query Design Rules

Each response should be:

- bounded
- local
- typed
- linkable to files

The goal is to keep one AI turn focused on one base, one pack, or one local
composition problem.

## 10. Validation Architecture

Validation must be built into authoring, not treated as an afterthought.

### 10.1 Validation Levels

#### Level 1: Schema Validation

- required fields
- value kinds
- enum legality

#### Level 2: Link Validation

- missing dependencies
- dangling references
- circular references where forbidden

#### Level 3: Assembly Validation

- base command legality
- assembled descriptor completeness
- incompatible option combinations

#### Level 4: Preview Validation

- avatar renders
- tilemap renders
- dialogue graph previews
- encounter map overlays

#### Level 5: Runtime Smoke Validation

- pack boot simulation
- local region enter/exit
- NPC schedule tick
- quest state transition smoke
- encounter spawn smoke

#### Level 6: Persistence Validation

- save snapshot
- reload snapshot
- verify world flags, inventories, NPC states, and quest states roundtrip

#### Level 7: Budget Validation

- sprite budget
- memory budget
- pack dependency budget
- region residency budget

### 10.2 Validation Ownership

Every base must ship with its own validator.
Every pack must declare which validators to run.

## 11. Creator AI Workflow

The top-level creator AI should follow this pipeline.

### 11.1 Intake

Input:

- player fantasy
- genre
- camera
- play length target
- session loop
- tone
- content constraints

Output:

- world brief
- systems brief
- initial pack list

### 11.2 Base Selection

The creator AI queries available bases and selects:

- which bases are sufficient
- which bases need new content only
- which bases lack one reusable capability

### 11.3 World Assembly

The creator AI creates or edits:

- world manifest
- region graph
- pack manifests

### 11.4 Content Assembly

The creator AI creates or edits:

- regions
- NPCs
- avatars
- quests
- dialogue
- encounters
- items

### 11.5 Local Validation

The creator AI runs:

- pack validation
- preview generation
- local smoke tests

### 11.6 Escalation Rule

The creator AI is allowed to edit engine code only when:

- the requested result cannot be expressed by current bases
- the missing capability is reusable
- the change can remain local to one base or one runtime service

This prevents the creator AI from solving every game request with engine forks.

## 12. Refactor Guidance From Current SHE

The current SHE project should not be thrown away.

### 12.1 Keep as Foundations

Keep:

- the service-oriented core
- gameplay commands/events/timers
- diagnostics
- feature-style ownership discipline
- tests and smoke workflows

### 12.2 Upgrade Areas

Upgrade:

- `SceneWorld` into `WorldService`
- `DataService` into `ContentService`
- AI context export into query-first context services
- renderer into tilemap/avatar/animation/text-capable presentation
- scripting from contract host to bounded pack-side authoring glue

### 12.3 New Center of Gravity

The center of gravity must move from:

- `Game/Features/*`

toward:

- `Engine/Bases/*`
- `Game/Packs/*`
- `Game/Worlds/*`

## 13. Implementation Waves

### Wave A: Kernel Upgrade

Ship:

- `IWorldService`
- `IContentService`
- `IContextQueryService`
- `ISaveGameService`
- `IStreamingService`
- `IValidationService`

Done when:

- packs can be loaded and queried locally
- context can be fetched without full-project dumps

### Wave B: First Four Bases

Ship:

- `RegionBase`
- `TilemapBase`
- `AvatarBase`
- `AnimationBase`

Done when:

- one traversable region can be assembled from packs
- one character can be rendered from avatar and animation descriptors

### Wave C: Social World Bases

Ship:

- `NPCBase`
- `InteractionBase`
- `DialogueBase`
- `QuestBase`

Done when:

- one town can host named NPCs, dialogue, and a quest loop

### Wave D: Open-World Runtime

Ship:

- `EncounterBase`
- `ItemEconomyBase`
- save/load
- region streaming
- persistence smoke tests

Done when:

- the game supports a small open area with revisiting, persistence, and
  progression

### Wave E: Creator AI Loop

Ship:

- creator query workflow
- pack diff workflow
- pack validation workflow
- world assembly workflow

Done when:

- a top-level creator AI can generate a small open-world slice mostly by
  adding or editing packs

## 14. Success Criteria

The architecture is succeeding when all of the following become true.

1. New regions usually require no engine changes.
2. New NPCs usually require no engine changes.
3. New quests usually require no engine changes.
4. Most AI-created changes live in packs, not runtime core.
5. AI can inspect only one local pack and still make correct edits.
6. Validation catches most authoring failures before full game launch.
7. One creator AI can assemble a small open-world game without reading the
   whole repository.

## 15. Final Rule

The engine should be optimized for **assembly under bounded understanding**.

That is the central AI-native requirement.

Not maximum feature count.
Not maximum subsystem cleverness.
Not maximum abstraction density.

The engine wins when limited context still produces rich worlds.
