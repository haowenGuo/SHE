# W07 Handoff - SDL3 Platform Input Runtime Shell

## Summary

- replaced the null runtime window path with a real `SDL3`-backed
  `SdlWindowService`
- extended the shared window contract so runtime code can query keyboard,
  pointer, and window snapshots without leaking SDL types outside
  `Engine/Platform`
- switched the game, sandbox, and smoke runtime wiring onto the SDL-backed
  window service
- added a focused platform/input test that pushes SDL events directly and
  verifies frame-edge input semantics

## Suggested Status For W00

- `ready_for_integration`

## Changed Files

- `Engine/Core/Include/SHE/Core/ApplicationConfig.hpp`
- `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`
- `Engine/Core/Source/Application.cpp`
- `Engine/Platform/CMakeLists.txt`
- `Engine/Platform/Include/SHE/Platform/SdlWindowService.hpp`
- `Engine/Platform/Source/SdlWindowService.cpp`
- `Engine/Platform/Include/SHE/Platform/NullWindowService.hpp`
- `Engine/Platform/Source/NullWindowService.cpp`
- `Game/Source/Main.cpp`
- `Tools/Sandbox/Source/Main.cpp`
- `Tests/CMakeLists.txt`
- `Tests/Source/SmokeTests.cpp`
- `Tests/Source/GameplayContractTests.cpp`
- `Tests/Source/PlatformInputTests.cpp`

## Shared Interface Changes

- `Engine/Core/Include/SHE/Core/ApplicationConfig.hpp`
  - added `startWindowHidden` so automated runs can use a real SDL window path
    without popping a visible desktop window
- `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`
  - added `KeyCode`, `PointerButton`, `ButtonState`, `PointerState`, and
    `WindowState`
  - expanded `IWindowService` with:
    - `GetKeyState`
    - `GetPointerButtonState`
    - `GetPointerState`
    - `GetWindowState`
    - `GetPumpCount`
- `Engine/Core/Source/Application.cpp`
  - platform diagnostics now record real window/input snapshot data from the
    shared window contract instead of a hard-coded null-window summary

## Threading And Event Ownership Assumptions

- `SdlWindowService::Initialize` and `SdlWindowService::PumpEvents` require the
  main thread and throw if called elsewhere
- SDL event pumping is owned by `Application::Run`; other modules should read
  input through `IWindowService` snapshots instead of polling SDL directly
- keyboard and pointer edge flags (`pressed`, `released`, `resizedThisFrame`,
  wheel and pointer delta) are per-frame data and clear on the next
  `PumpEvents`
- held state (`isDown`, pointer position, close-request state) persists across
  frames until SDL reports a change

## Tests Run And Results

- `cmake --preset default` via the Visual Studio Build Tools CMake binary:
  passed
- `cmake --build --preset build-default`: passed
- `ctest --preset test-default`: passed (`4/4` tests)

## Unresolved Risks

- the current key contract intentionally covers the first gameplay/debug set
  rather than the full keyboard, so later UI/editor work may want to broaden
  `KeyCode`
- SDL is pinned through `FetchContent` in `Engine/Platform/CMakeLists.txt`,
  which means configure now depends on network access unless W00 later vendors
  or mirrors that dependency
- resize tests currently validate the window snapshot contract through pushed
  events rather than a real visible desktop resize interaction

## Exact Next Steps

1. have `W00` review the shared `RuntimeServices.hpp` additions before
   downstream scene, renderer, UI, and physics work starts consuming input
2. let `W05` and later runtime slices depend on `IWindowService` snapshots
   instead of adding direct SDL calls outside `Engine/Platform`
3. if `W08` needs renderer-backed pixel sizing, add that as a follow-up
   contract on top of the current window snapshot rather than exposing raw SDL
   handles
