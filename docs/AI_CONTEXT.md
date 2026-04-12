# AI Context Contract

This file describes what the engine should always be able to export for Codex.

## Required Sections

The authoring context should contain:

1. project intent
2. active scene and entity count
3. asset count
4. registered engine/gameplay types
5. registered feature modules
6. registered schemas
7. current gameplay digest
8. registered script modules
9. latest frame diagnostics report

## Current Exporter

The current exporter lives in:

- [AuthoringAiService.cpp](../Engine/AI/Source/AuthoringAiService.cpp)

and gathers data from:

- [ReflectionService.cpp](../Engine/Reflection/Source/ReflectionService.cpp)
- [DataService.cpp](../Engine/Data/Source/DataService.cpp)
- [GameplayService.cpp](../Engine/Gameplay/Source/GameplayService.cpp)
- [ScriptingService.cpp](../Engine/Scripting/Source/ScriptingService.cpp)
- [DiagnosticsService.cpp](../Engine/Diagnostics/Source/DiagnosticsService.cpp)

## Stability Rules

1. Context output should prefer stable labels over transient memory details.
2. New engine subsystems should extend the context exporter, not bypass it.
3. Game features should register metadata instead of forcing AI tools to scrape
   random files.
4. Data changes should be discoverable through schema registration.
