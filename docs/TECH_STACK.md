# Tech Stack

This file explains both the current bootstrap implementation and the intended production technology for each module.

## 1. Guiding Principle

The repository is starting with **interface-first architecture**.

That means Phase 1 favors:

- compileable boundaries
- readable code
- explicit ownership
- replacement points for real middleware later

The current code is intentionally not pretending that placeholder services are final production code.

## 2. Module by Module

| Module | Phase 1 Implementation | Planned Production Technology | Why |
| --- | --- | --- | --- |
| Core | Standard C++20 only | Standard C++20 + optional profiling hooks | Keep the engine spine portable and stable |
| Platform | Null window service | `SDL3` | Strong Windows support now, easy path to more platforms later |
| Assets | In-memory registry | `yaml-cpp` + import metadata + cooked cache | Clean asset identity before import complexity |
| Scene | Minimal scene world | `EnTT` | Strong ECS ergonomics without overbuilding custom storage |
| Renderer | Null renderer service | `OpenGL` first, optional later RHI | Fastest route to a practical 2D renderer |
| Physics | Null physics service | `Box2D` | Reliable 2D collisions and rigid body basics |
| Audio | Null audio service | `miniaudio` | Lightweight and very practical for indie-scale runtime audio |
| UI | Null UI service | `Dear ImGui` + simple runtime HUD layer | Fast debug tooling without blocking gameplay work |

## 3. Why C++20

The engine is planned in `C++20` because it gives:

- value semantics and RAII for clear ownership
- templates where they help, without requiring runtime reflection
- low-level control for rendering and memory layout
- compatibility with middleware we likely want later

It is also realistic for an engine project that may eventually need performance-sensitive render and simulation code.

## 4. Why CMake

`CMake` is used because it gives us:

- a build system that scales from bootstrap to multi-target runtime
- easy separation of engine libraries and executables
- future dependency integration through `FetchContent`, `vcpkg`, or system packages
- IDE support across Visual Studio, CLion, VS Code, and Ninja builds

The current repository structure maps cleanly to CMake targets, which keeps module boundaries visible in build configuration as well as source code.

## 5. Planned Future Middleware

### SDL3

Used for:

- window creation
- keyboard/mouse/gamepad input
- event pumping

Reason:

- mature, practical, and fast to integrate
- ideal for an engine whose first shipping target is desktop 2D

### OpenGL

Used for:

- sprite rendering
- texture uploads
- framebuffers
- post-processing

Reason:

- simpler first backend than modern explicit APIs
- enough for a strong 2D renderer
- easier to teach and debug during early engine development

### EnTT

Used for:

- entity identifiers
- component storage
- queries and system iteration
- event dispatch where useful

Reason:

- very strong fit for a small-to-medium engine
- avoids over-investing in a custom ECS too early

### Box2D

Used for:

- colliders
- rigid bodies
- contact callbacks
- raycasts

Reason:

- proven 2D physics behavior
- strong documentation and community familiarity

### miniaudio

Used for:

- sound effect playback
- music
- buses and volume control

Reason:

- compact integration footprint
- good enough for a full small game

### yaml-cpp

Used for:

- scene files
- prefab files
- data tables for gameplay tuning

Reason:

- readable by humans
- suitable for iteration-heavy game development

### Dear ImGui

Used for:

- debug HUD
- inspector panels
- profiling views
- scene and asset inspection

Reason:

- very fast path to useful tooling
- keeps editor/tooling work from blocking core runtime progress

## 6. What This Means for Contributors

If you add code during Phase 1:

- prefer interfaces and contracts over complex functionality
- keep module boundaries explicit
- leave clear comments about future replacement points

If you add code in later phases:

- replace placeholder implementations behind the existing interfaces
- do not let middleware leak everywhere
- preserve the rule that gameplay code depends on engine contracts, not middleware APIs

