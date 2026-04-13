# W09 Handoff - Box2D Runtime Slice

## Summary

- replaced the placeholder fixed-step physics path with a first-pass
  `Box2DPhysicsService` that owns a real Box2D world behind the existing core
  runtime boundary
- expanded `IPhysicsService` just enough to register bodies and box colliders
  against W05 scene entity ids without reaching into `SceneWorld` storage
- kept physics/scene integration on the W05 public surface only:
  - initial body transforms come from `TryGetEntityView`
  - fixed-step writeback flows through `TrySetEntityTransform`
  - destroyed scene entities are pruned from the physics registry on the next
    fixed step
- bridged collision begin/end callbacks into the W01 gameplay event contract by
  queueing `physics/CollisionStarted` and `physics/CollisionEnded` events
- switched the real game runtime, sandbox runtime, and smoke test runtime from
  `NullPhysicsService` to `Box2DPhysicsService`
- added focused physics contract tests for:
  - body/collider lifecycle
  - scene transform synchronization
  - collision callback delivery through gameplay events

## Suggested Status For W00

- `ready_for_integration`

## Changed Files

- `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`
- `Engine/Physics/CMakeLists.txt`
- `Engine/Physics/Include/SHE/Physics/Box2DPhysicsService.hpp`
- `Engine/Physics/Include/SHE/Physics/NullPhysicsService.hpp`
- `Engine/Physics/Source/Box2DPhysicsService.cpp`
- `Engine/Physics/Source/NullPhysicsService.cpp`
- `Game/Source/Main.cpp`
- `Tools/Sandbox/Source/Main.cpp`
- `Tests/CMakeLists.txt`
- `Tests/Source/SmokeTests.cpp`
- `Tests/Source/PhysicsContractTests.cpp`

## Shared Interface Changes

Shared file touched:

- `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`

New shared physics data contracts:

- `PhysicsColliderId`
- `PhysicsBodyType`
- `PhysicsBodyDefinition`
- `PhysicsBoxColliderDefinition`

`IPhysicsService` gained these public contract points:

- `CreateBody(EntityId, const PhysicsBodyDefinition&)`
- `DestroyBody(EntityId)`
- `HasBody(EntityId) const`
- `CreateBoxCollider(EntityId, const PhysicsBoxColliderDefinition&)`
- `DestroyCollider(PhysicsColliderId)`

Shared high-risk build file touched:

- `Engine/Physics/CMakeLists.txt`
  - adds `FetchContent` for Box2D
  - pins Box2D to `v2.4.1`

Shared high-risk files not changed:

- `Engine/Core/Source/Application.cpp`
- `Engine/CMakeLists.txt`
- top-level `CMakeLists.txt`

## Box2D Version Note

- this slice pins Box2D to `v2.4.1`, not Box2D `3.x`
- reason: the repository is currently configured with the Visual Studio 2019
  CMake `3.20` binary, while Box2D `3.x` requires newer CMake
- the runtime boundary stays Box2D-specific but middleware-leakage remains
  contained inside `Engine/Physics/*`, so a later upgrade to Box2D `3.x` can
  happen behind the same `IPhysicsService` surface if the toolchain is raised

## Fixed-Step Sequencing Assumption

- no application-loop reorder was needed
- the accepted sequencing remains:
  1. layer `OnFixedUpdate`
  2. gameplay timer advance through `AdvanceFixedStep`
  3. physics `Step`
- collision callbacks therefore enqueue gameplay events during the fixed-step
  phase and those events dispatch later through the existing
  `gameplay->FlushCommands()` path in the same frame

## Tests Run And Results

- `C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --preset default`: passed
- `C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build --preset build-default`: passed
- `C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe --preset test-default`: passed (`7/7` tests)

Physics-specific coverage now includes:

- `she_physics_contract_tests`: passed
- `she_smoke_tests`: passed with the runtime now using `Box2DPhysicsService`

## Unresolved Risks

- scene-authored transform overrides for dynamic or kinematic bodies currently
  use teleport-style `SetTransform` sync rather than velocity-targeted motion,
  which is acceptable for this first slice but may need refinement once gameplay
  starts driving moving platforms or scripted kinematic actors
- collision events currently preserve the W01 gameplay contract exactly by using
  `QueueEvent(...)`, which means the gameplay event `source` field remains
  `gameplay_api`; if downstream diagnostics or gameplay logic needs a more
  explicit `physics:*` source string, that should be added as a deliberate W01/W09
  shared-contract follow-up rather than bypassing the event bus
- only box colliders are exposed in the shared contract for now; circles,
  joints, filters, and richer material rules are still future work

## Exact Next Steps

1. `W00` should record the new physics interface expansion in
   `coordination/INTEGRATION_REPORT.md`, especially the `RuntimeServices.hpp`
   additions and the `Engine/Physics/CMakeLists.txt` Box2D dependency hook.
2. downstream gameplay/features can start registering physics bodies and
   colliders through `IPhysicsService` without depending on `SceneWorld`
   internals.
3. if `W11` later needs inspectable runtime data, expose a small physics debug
   snapshot on top of this service rather than reading Box2D objects directly.
4. if renderer/gameplay slices need deterministic contact provenance in the
   gameplay event `source` field, add that as a small shared-contract extension
   instead of inventing a second collision dispatch path.
