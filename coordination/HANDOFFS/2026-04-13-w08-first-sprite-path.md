# W08 Handoff - First Sprite Path

## Summary

- replaced the old null renderer shell with a first real `Renderer2DService`
  that bootstraps a capture-oriented 2D backend
- expanded the shared renderer contract to carry camera submission, sprite
  submission, material lookup, backend info, and completed frame snapshots
- wired the bootstrap game layer and smoke runtime onto the new renderer path
  so scene entities now submit a camera and sprite through the renderer instead
  of only reporting scene name/entity count
- added a focused renderer contract test that locks down backend bootstrap,
  material resolution, and frame-owned texture lease rules

## Suggested Status For W00

- `ready_for_integration`

## Changed Files

- `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`
- `Engine/Renderer/CMakeLists.txt`
- `Engine/Renderer/Include/SHE/Renderer/Renderer2DService.hpp`
- `Engine/Renderer/Source/Renderer2DService.cpp`
- `Engine/Renderer/Include/SHE/Renderer/NullRendererService.hpp`
- `Engine/Renderer/Source/NullRendererService.cpp`
- `Game/Features/Bootstrap/BootstrapFeatureLayer.hpp`
- `Game/Features/Bootstrap/BootstrapFeatureLayer.cpp`
- `Game/Source/Main.cpp`
- `Tools/Sandbox/Source/Main.cpp`
- `Tests/CMakeLists.txt`
- `Tests/Source/SmokeTests.cpp`
- `Tests/Source/RendererContractTests.cpp`

## Shared Interface Changes

- `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`
  - added:
    - `Camera2DSubmission`
    - `Sprite2DSubmission`
    - `Material2DDescriptor`
    - `ResolvedMaterial2D`
    - `RenderedSprite2D`
    - `RendererBackendInfo`
    - `RenderFrameSnapshot`
  - expanded `IRendererService` with:
    - `RegisterMaterial`
    - `HasMaterial`
    - `ResolveMaterial`
    - `SubmitCamera`
    - `SubmitSprite`
    - `HasFrameInFlight`
    - `GetLastCompletedFrame`
    - `GetBackendInfo`

## Minimal Shared-File Touch Note

- `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`
  - minimal renderer-only contract expansion; no scene/data/platform contracts
    were rewritten
- `Tests/CMakeLists.txt`
  - appended one new target: `she_renderer_contract_tests`
- `Engine/Core/Source/Application.cpp`
  - unchanged

## Frame And Resource Ownership Rules

- `BeginFrame` opens renderer-owned submission scope for the current frame
- `SubmitCamera` and `SubmitSprite` only succeed while a frame is in flight
- sprite submission resolves the authored material to a texture asset through
  the integrated W06 asset registry before accepting the draw
- the renderer acquires at most one texture lease per asset for the active
  frame, reuses it across sprite submissions, and releases it at `EndFrame`
- `EndFrame` seals an immutable `RenderFrameSnapshot` that later UI/diagnostics
  work can read through `GetLastCompletedFrame`

## Runtime Wiring Assumptions

- the renderer currently consumes window sizing through the camera submission,
  which lets W08 depend on W07 window snapshots without adding SDL-specific
  hooks to the renderer contract
- bootstrap gameplay registers a placeholder texture loader and a simple
  material name (`materials/bootstrap.player`) before submitting sprites
- features/layers remain responsible for deciding which scene entities become
  camera/sprite submissions; W08 did not widen the W05 scene interface

## Tests Run And Results

- `cmake --preset default` via the Visual Studio Build Tools CMake binary:
  passed
- `cmake --build --preset build-default`: passed
- `ctest --preset test-default --output-on-failure`: passed (`7/7` tests)

## Unresolved Risks

- the current backend is a capture-oriented bootstrap path; it proves camera,
  sprite, and resource ownership flow, but it does not yet present real pixels
  through SDL/OpenGL
- scene-driven sprite submission still lives in feature layers because W05 does
  not yet expose broader render-component queries
- material registration is explicit per feature/test right now, so later
  content work may want a higher-level asset-to-material convention

## Exact Next Steps

1. have `W00` review and integrate the renderer contract additions in
   `RuntimeServices.hpp`
2. let `W11` and diagnostics follow-on work consume `GetLastCompletedFrame`
   instead of inspecting renderer internals
3. if the next renderer slice wants on-screen pixels, layer an actual
   SDL/OpenGL present path behind `Renderer2DService` without narrowing the
   accepted frame/material contract
