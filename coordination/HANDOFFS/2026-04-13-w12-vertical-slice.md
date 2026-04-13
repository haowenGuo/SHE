# W12 Vertical Slice Handoff

Summary:
- added a new `Game/Features/VerticalSlice/` feature and switched `she_game` to launch it instead of the bootstrap sample
- implemented a small top-down arena loop: move, collect three signal cores, avoid patrol drones, restart after win/loss
- kept the slice on accepted W01-W11 surfaces instead of adding new shared runtime contracts

Engine Surfaces Exercised:
- `IGameplayService`: start/restart/spawn commands, slice state events, timers
- `IScriptingService`: feature-local script module routes round, pickup, and wave commands
- `IDataService`: round/pickup/wave schemas plus authored records
- `ISceneService` + `IRendererService`: player, pickup, hazard, HUD, and arena sprites
- `IPhysicsService`: sensor collisions drive pickup collection and fail states
- `IAudioService`: gameplay-routed music and SFX playback
- `IUiService` + `IDiagnosticsService` + `IAIService`: runtime debug report and authoring context stay live while the slice runs

Files Added:
- `Game/Features/VerticalSlice/*`
- `Tests/Source/VerticalSliceSmokeTests.cpp`
- `Tests/Source/VerticalSliceFlowTests.cpp`

Files Updated:
- `Game/CMakeLists.txt`
- `Game/Source/Main.cpp`
- `Tests/CMakeLists.txt`

Tests Intended:
- `she_vertical_slice_smoke_tests`
- `she_vertical_slice_flow_tests`

Notes For W00:
- no new shared engine contracts were added
- the slice reuses the existing bootstrap audio clips by local copies under `Game/Features/VerticalSlice/Data/`
- `BootstrapFeatureLayer` and its tests remain in the tree for reference coverage even though `she_game` now launches the vertical slice
