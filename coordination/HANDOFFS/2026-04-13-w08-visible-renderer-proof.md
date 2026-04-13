# W08 Handoff - Visible Renderer Proof

## Summary

- upgraded `Renderer2DService` from capture-only bookkeeping to a real
  SDL-backed 2D present path
- kept the accepted camera/sprite/material/frame-ownership contract, but now
  drive it into actual pixels on an SDL window renderer at `EndFrame`
- added a native-window bridge on `IWindowService` so the renderer can bind to
  the W07 SDL window without leaking SDL types through gameplay code
- strengthened renderer validation so tests now require both a completed frame
  snapshot and proof that visible pixels were produced beyond the clear color

## Suggested Status For W00

- `ready_for_integration`

## Changed Files

- `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`
- `Engine/Platform/Include/SHE/Platform/SdlWindowService.hpp`
- `Engine/Platform/Source/SdlWindowService.cpp`
- `Engine/Platform/Include/SHE/Platform/NullWindowService.hpp`
- `Engine/Platform/Source/NullWindowService.cpp`
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
- `Tests/Source/PlatformInputTests.cpp`
- `Tests/Source/SmokeTests.cpp`
- `Tests/Source/RendererContractTests.cpp`

## Shared Interface Changes

- `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`
  - added `NativeWindowBackend` and `NativeWindowHandle`
  - expanded `IWindowService` with `GetNativeWindowHandle`
  - extended `RenderFrameSnapshot` with:
    - `presentedToSurface`
    - `visiblePixelSampleCount`
- renderer contract additions from the earlier W08 slice remain in place:
  - camera submission
  - sprite submission
  - material lookup
  - completed frame snapshots

## Minimal Shared-File Touch Note

- `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`
  - minimal renderer/platform bridge only:
    - one opaque native-window handle contract for renderer bootstrap
    - two renderer-only presentation evidence fields on `RenderFrameSnapshot`
- `Tests/CMakeLists.txt`
  - no new shared-test churn beyond the already-added
    `she_renderer_contract_tests` target
- `Engine/Core/Source/Application.cpp`
  - unchanged

## Visible Renderer Path

- if the active window service exposes an SDL3 native window handle,
  `Renderer2DService` now creates an SDL renderer bound to that window
- hidden test windows prefer the SDL software renderer for deterministic
  readback; visible runtime windows prefer `opengl` with `software` fallback
- `EndFrame` now:
  1. clears the SDL backbuffer from the submitted camera clear color
  2. resolves submitted sprite materials to asset-backed placeholder textures
  3. renders sorted sprites to the SDL renderer
  4. samples the backbuffer before present to confirm non-clear pixels exist
  5. presents the frame to the bound window surface

## Resource And Ownership Rules

- frame-owned asset leases still open at `BeginFrame`/submission time and are
  released at `EndFrame`
- textures are cached per asset id inside the renderer backend but do not
  replace W06 asset lifetime; W06 still owns the stable asset identity and
  lease rules
- the native SDL window pointer stays owned by `SdlWindowService`; W08 only
  consumes it through the opaque `NativeWindowHandle` contract

## Runtime Wiring Assumptions

- bootstrap gameplay still decides which entities become camera/sprite
  submissions; W08 did not widen the W05 scene query surface
- placeholder texture pixels are generated from asset metadata so the renderer
  can produce visible output before a real image-import path exists
- visible proof now exists through SDL present even though W08 is not yet a
  full OpenGL sprite batcher

## Tests Run And Results

- `cmake --preset default` via the Visual Studio Build Tools CMake binary:
  passed
- `cmake --build --preset build-default`: passed
- `ctest --preset test-default --output-on-failure`: passed (`7/7` tests)

## Unresolved Risks

- sprite textures are still generated placeholders; W06/W08 have not yet added
  real PNG import/upload
- visible proof currently uses the SDL renderer abstraction rather than a
  custom OpenGL path, which is acceptable for this milestone but not the final
  planned backend shape
- the renderer now depends on the SDL-backed platform bridge, so future
  renderer backend refactors should keep the opaque native-window contract
  stable instead of reaching deeper into `SdlWindowService`

## Exact Next Steps

1. have `W00` re-run W08 acceptance and treat this handoff as superseding the
   earlier capture-only W08 checkpoint
2. if accepted, let W11 consume the completed frame snapshot plus presentation
   evidence rather than renderer internals
3. follow up with real texture upload/import once W06 grows beyond placeholder
   asset records
