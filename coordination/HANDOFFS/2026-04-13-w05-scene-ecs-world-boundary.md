# W05 Handoff - Scene ECS World Boundary

## Summary

- expanded the shared scene contract from a scene-name plus entity-count shell
  into a first-pass world boundary for entity identity, lifetime, transform
  access, and query
- taught `SceneWorld` to keep scene-owned name and transform component storage
  behind a stable `ISceneService` interface
- added deferred destroy semantics so runtime callers lose visibility
  immediately while storage pruning stays anchored to the scene update boundary
- added focused scene contract tests that validate create/query/update/destroy
  flow and scene-switch reset behavior

## Suggested Status For W00

- `ready_for_integration`

## Changed Files

- `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`
- `Engine/Scene/Include/SHE/Scene/SceneWorld.hpp`
- `Engine/Scene/Source/SceneWorld.cpp`
- `Tests/CMakeLists.txt`
- `Tests/Source/SceneContractTests.cpp`

## Shared Interface Changes

- `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`
  - added `SceneTransform`
  - added `SceneEntityView`
  - expanded `ISceneService` with:
    - `DestroyEntity`
    - `HasEntity`
    - `TryGetEntityView`
    - `TrySetEntityTransform`
    - `ListEntities`

## Lifetime And Ownership Rules

- scene identity remains owned by `SetActiveScene`; switching scenes replaces
  the active world contents but does not rewind entity id allocation
- `CreateEntity` returns a stable runtime id for the active world only
- `DestroyEntity` removes external visibility immediately and schedules storage
  pruning for the next `UpdateSceneGraph`
- `TryGetEntityView` returns a scene-owned snapshot view rather than exposing
  component storage directly
- transform writes flow through `TrySetEntityTransform`; downstream runtime
  systems should not assume direct access to component containers

## Tests Run And Results

- `cmake --preset default`: passed
- `cmake --build --preset build-default`: passed
- `ctest --preset test-default`: passed (`4/4` tests)

## Unresolved Risks

- the current ECS slice only covers scene-owned name and transform data, so
  later renderer and physics work may still need broader component conventions
- entity lookup is linear in the slot list for now, which is acceptable for the
  bootstrap runtime but may need tightening once higher-volume worlds arrive
- `SceneEntityView` is a snapshot contract, not a mutable live reference, so
  downstream modules should avoid caching it as if it were authoritative state

## Exact Next Steps

1. let `W06` and later runtime slices depend on `ISceneService` views instead
   of reaching into `SceneWorld` internals
2. have `W08` and `W09` build their scene consumption rules on `ListEntities`
   and `TryGetEntityView` before expanding shared scene contracts
3. if later work needs richer component families, extend the scene boundary
   deliberately rather than exposing raw storage maps through `RuntimeServices`
