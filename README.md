# SHE

SHE is a bootstrap repository for a small 2D game engine and the game built on top of it.

This repository is intentionally structured as a teaching-friendly codebase:

- `Engine/` contains reusable runtime modules.
- `Game/` contains concrete gameplay code that depends on the engine.
- `Tools/` contains non-shipping tools such as the sandbox app.
- `Tests/` contains smoke tests for the bootstrap architecture.

At this stage the project is a compileable architectural skeleton rather than a feature-complete engine. The goal of this first milestone is to make the ownership boundaries, module responsibilities, and development workflow obvious before complex rendering and physics code arrives.

## Read This First

1. [Architecture](docs/ARCHITECTURE.md)
2. [Tech Stack](docs/TECH_STACK.md)
3. [Development Workflow](docs/DEVELOPMENT_WORKFLOW.md)

## Current Milestone

Phase 1 is about the runtime spine:

- application loop
- layer stack
- runtime service contracts
- bootstrap implementations for window, renderer, audio, physics, assets, and scene
- game and sandbox entry points
- smoke tests

The current code uses minimal in-house placeholder implementations so the repository can compile without external dependencies. The architecture documents already explain which third-party technologies are planned for later phases.

## Directory Map

```text
SHE/
  cmake/                  Build helpers and compiler policies
  docs/                   Architecture and process documentation
  Engine/                 Reusable engine runtime modules
  Game/                   Game-specific layers, data, and executable
  Tools/Sandbox/          Engine inspection executable
  Tests/                  Smoke tests
```

## Build

```bash
cmake --preset default
cmake --build --preset build-default
ctest --preset test-default
```

If you prefer classic commands:

```bash
cmake -S . -B build/default -G "Visual Studio 16 2019" -A x64
cmake --build build/default --config Debug
ctest --test-dir build/default -C Debug --output-on-failure
```

If `cmake` is not in your `PATH`, Visual Studio Build Tools usually ships one at:

`C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`

## Intended Future Stack

The planned runtime stack for later milestones is:

- `SDL3` for windowing and input
- `OpenGL` for the first 2D renderer backend
- `EnTT` for ECS
- `Box2D` for 2D physics
- `miniaudio` for audio
- `yaml-cpp` for scenes, prefabs, and gameplay data
- `Dear ImGui` for debug tooling

The bootstrap code keeps those dependencies at the architecture level for now, then leaves clean replacement points for the real implementations.
