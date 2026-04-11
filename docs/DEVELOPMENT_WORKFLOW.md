# Development Workflow

This repository is meant to be easy to read and easy to extend.

The workflow below is designed to keep both true.

## 1. Start with the Responsibility

Before writing code, answer three questions:

1. Is this engine runtime code, gameplay code, tool code, or test code?
2. Which module should own it?
3. Does it create a new dependency edge?

If the answer to question 3 is "yes", document the reason in the relevant architecture file.

## 2. Where New Code Goes

### Engine code

Add to `Engine/<Module>/`.

- Public API goes in `Include/SHE/...`
- Internal implementation goes in `Source/`

### Gameplay code

Add to `Game/Source/`.

Gameplay should be expressed as:

- layers
- systems
- state logic
- data-driven configuration

Gameplay should not directly own low-level platform or rendering backends.

### Tooling code

Add to `Tools/`.

Tools may inspect engine internals more aggressively than the shipping game, but they still should not become a dumping ground for engine responsibilities.

### Tests

Add to `Tests/Source/`.

Prefer small, targeted smoke tests that validate contracts and frame flow.

## 3. Commenting Standard

This project is intentionally comment-friendly.

Expect comments in three places:

1. **File-level orientation**
   Explain why the file exists and what boundary it owns.
2. **Class-level responsibility**
   Explain what the type is responsible for and what it is not responsible for.
3. **Non-obvious methods**
   Explain lifecycle order, invariants, or future replacement points.

Avoid comments that only restate obvious syntax.

## 4. Build and Verify

Typical local loop:

```bash
cmake --preset default
cmake --build --preset build-default
ctest --preset test-default
```

If you are only touching architecture docs, still consider a configure step to make sure the project structure remains healthy.

## 5. How to Add a New Engine Feature

Example: adding an animation subsystem.

1. Add the interface contract if the application loop or other modules need to talk to it.
2. Create `Engine/Animation/Include` and `Source`.
3. Add a CMake target and link it through `Engine/CMakeLists.txt`.
4. Update `docs/ARCHITECTURE.md` and `docs/TECH_STACK.md`.
5. Add a smoke test if the new feature changes runtime sequencing or ownership.

## 6. Pull Request Checklist

Before considering a change complete, check:

- module ownership still makes sense
- no forbidden dependency direction was added
- comments explain the new boundary
- docs were updated if architecture changed
- build still configures
- tests still pass

## 7. Short-Term Roadmap for Contributors

If you want to help extend this bootstrap into a real 2D engine, the recommended next sequence is:

1. Replace the null platform service with SDL3
2. Replace the null renderer with a real OpenGL sprite pipeline
3. Replace the bootstrap scene store with EnTT
4. Introduce asset metadata and YAML scene files
5. Add Box2D integration on the fixed-step path
6. Add ImGui-based debug tooling

