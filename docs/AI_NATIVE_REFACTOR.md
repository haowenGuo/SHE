# AI-Native Refactor

This document describes the concrete refactor target for turning the current
2D engine skeleton into an AI-native runtime. The design goal is simple:

> Codex should be able to add, inspect, and validate gameplay features by
> reading stable engine contracts instead of reverse-engineering the project.

## 1. Directory Plan

```text
SHE/
  Engine/
    Core/
      Include/SHE/Core/
        Application.hpp
        AppClock.hpp
        Layer.hpp
        RuntimeServices.hpp
      Source/
        Application.cpp
        AppClock.cpp
    Platform/
      Include/SHE/Platform/
        NullWindowService.hpp
      Source/
        NullWindowService.cpp
    Assets/
      Include/SHE/Assets/
        AssetManager.hpp
      Source/
        AssetManager.cpp
    Scene/
      Include/SHE/Scene/
        SceneWorld.hpp
        Components.hpp
        Entity.hpp
      Source/
        SceneWorld.cpp
    Reflection/
      Include/SHE/Reflection/
        ReflectionService.hpp
      Source/
        ReflectionService.cpp
    Data/
      Include/SHE/Data/
        DataService.hpp
      Source/
        DataService.cpp
    Gameplay/
      Include/SHE/Gameplay/
        CommandBuffer.hpp
        TimerService.hpp
        GameplayService.hpp
      Source/
        CommandBuffer.cpp
        TimerService.cpp
        GameplayService.cpp
    Renderer/
      Include/SHE/Renderer/
        NullRendererService.hpp
      Source/
        NullRendererService.cpp
    Physics/
      Include/SHE/Physics/
        NullPhysicsService.hpp
      Source/
        NullPhysicsService.cpp
    Audio/
      Include/SHE/Audio/
        NullAudioService.hpp
      Source/
        NullAudioService.cpp
    UI/
      Include/SHE/UI/
        NullUiService.hpp
      Source/
        NullUiService.cpp
    Scripting/
      Include/SHE/Scripting/
        ScriptingService.hpp
      Source/
        ScriptingService.cpp
    Diagnostics/
      Include/SHE/Diagnostics/
        DiagnosticsService.hpp
      Source/
        DiagnosticsService.cpp
    AI/
      Include/SHE/AI/
        AuthoringAiService.hpp
      Source/
        AuthoringAiService.cpp
  Game/
    Features/
      README.md
      Bootstrap/
        BootstrapFeatureLayer.hpp
        BootstrapFeatureLayer.cpp
        Data/
          bootstrap_enemy.schema.yml
          bootstrap_encounter.schema.yml
    Source/
      Main.cpp
  Tools/
    Sandbox/
      Source/
        Main.cpp
        SandboxLayer.cpp
  Tests/
    Source/
      SmokeTests.cpp
  docs/
    ARCHITECTURE.md
    AI_NATIVE_REFACTOR.md
    AI_CONTEXT.md
    TECH_STACK.md
    DEVELOPMENT_WORKFLOW.md
```

## 2. Runtime Service Interfaces

The engine now treats these services as first-class runtime contracts:

| Interface | Concrete bootstrap class | Responsibility |
| --- | --- | --- |
| `IWindowService` | `NullWindowService` | Platform events and close requests |
| `IAssetService` | `AssetManager` | Asset identity and logical registration |
| `ISceneService` | `SceneWorld` | Active scene and entity ownership |
| `IReflectionService` | `ReflectionService` | Types and features for tooling and AI |
| `IDataService` | `DataService` | Schema-first data contracts |
| `IGameplayService` | `GameplayService` | Events, commands, timers |
| `IRendererService` | `NullRendererService` | Frame submission contract |
| `IPhysicsService` | `NullPhysicsService` | Fixed-step simulation owner |
| `IAudioService` | `NullAudioService` | Audio frame owner |
| `IUiService` | `NullUiService` | Debug/runtime UI frame owner |
| `IScriptingService` | `ScriptingService` | Script host boundary |
| `IDiagnosticsService` | `DiagnosticsService` | Frame traces and reports |
| `IAIService` | `AuthoringAiService` | Codex-facing context export |

## 3. Concrete Class Plan

### Core

- [Application.hpp](../Engine/Core/Include/SHE/Core/Application.hpp)
- [RuntimeServices.hpp](../Engine/Core/Include/SHE/Core/RuntimeServices.hpp)

`Application` owns sequencing only.

### Reflection

- [ReflectionService.hpp](../Engine/Reflection/Include/SHE/Reflection/ReflectionService.hpp)

Key types:

- `TypeDescriptor`
- `FeatureDescriptor`
- `ReflectionService`

### Data

- [DataService.hpp](../Engine/Data/Include/SHE/Data/DataService.hpp)

Key types:

- `DataSchema`
- `DataService`

### Gameplay

- [CommandBuffer.hpp](../Engine/Gameplay/Include/SHE/Gameplay/CommandBuffer.hpp)
- [TimerService.hpp](../Engine/Gameplay/Include/SHE/Gameplay/TimerService.hpp)
- [GameplayService.hpp](../Engine/Gameplay/Include/SHE/Gameplay/GameplayService.hpp)

Key types:

- `GameplayCommand`
- `GameplayTimer`
- `TimerSignal`
- `GameplayEvent`
- `CommandBuffer`
- `TimerService`
- `GameplayService`

### Scripting

- [ScriptingService.hpp](../Engine/Scripting/Include/SHE/Scripting/ScriptingService.hpp)

Key types:

- `ScriptModuleDescriptor`
- `ScriptingService`

### Diagnostics

- [DiagnosticsService.hpp](../Engine/Diagnostics/Include/SHE/Diagnostics/DiagnosticsService.hpp)

Key types:

- `FramePhaseTrace`
- `FrameTrace`
- `DiagnosticsService`

### AI

- [AuthoringAiService.hpp](../Engine/AI/Include/SHE/AI/AuthoringAiService.hpp)

Key types:

- `AuthoringAiService`

## 4. Feature Module Shape

Every gameplay feature should follow this template:

```text
Game/Features/<FeatureName>/
  <FeatureName>Layer.hpp
  <FeatureName>Layer.cpp
  Data/
    <feature>.schema.yml
    <feature>.data.yml
  Tests/
    <FeatureName>Tests.cpp
  README.md
```

Rules:

1. Register feature metadata through `IReflectionService`.
2. Register every new data shape through `IDataService`.
3. Use `IGameplayService` for events, timers, and queued commands.
4. Register script modules through `IScriptingService`.
5. Keep the feature boundary narrow enough that Codex can edit it in one pass.

## 5. Frame Flow

```text
Diagnostics.BeginFrame
  -> Window.PumpEvents
  -> Gameplay.BeginFrame
  -> Fixed updates
      -> Layer.OnFixedUpdate
      -> Gameplay.AdvanceFixedStep
      -> Physics.Step
  -> Layer.OnUpdate
  -> Gameplay.AdvanceFrame
  -> Gameplay.FlushCommands
  -> Scripting.Update
  -> Scene.UpdateSceneGraph
  -> Renderer.BeginFrame / OnRender / SubmitSceneSnapshot / EndFrame
  -> UI.BeginFrame / OnUi / EndFrame
  -> Audio.Update
  -> AI.RefreshContext
  -> Diagnostics.EndFrame
```

This order is the main AI-native choice. It makes the engine explainable:

- diagnostics records each phase
- gameplay commands are centralized
- AI context export happens after the frame has a coherent story

## 6. What Codex Should Target

When asking Codex to add a feature, point it at:

1. the relevant `Game/Features/<Feature>` directory
2. the corresponding schema files
3. [AI_CONTEXT.md](./AI_CONTEXT.md)
4. the engine service contracts in [RuntimeServices.hpp](../Engine/Core/Include/SHE/Core/RuntimeServices.hpp)

That gives the model enough local structure to make useful changes without
guessing how the engine is organized.
