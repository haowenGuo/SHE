# Game Features

Gameplay code in this repository should be organized by feature, not by a large
miscellaneous source folder.

Each feature should own:

- its layer or systems
- its data schemas
- its authoring notes
- its tests

This structure is intentionally optimized for AI-assisted development. Codex can
be pointed at one feature directory plus the engine service contracts and make
focused changes without scanning the entire project.

