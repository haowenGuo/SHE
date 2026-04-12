# Bootstrap Feature

This feature is the reference example for how gameplay should be authored in
the AI-native engine.

What it demonstrates:

- feature registration through `IReflectionService`
- schema registration through `IDataService`
- command and timer usage through `IGameplayService`
- script module registration through `IScriptingService`
- authoring intent setup through `IAIService`

When adding a new feature, mirror this structure and keep the feature contract
inside one directory.

