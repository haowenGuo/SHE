# RogueLite

Top-down room-action sample for the current AI-native 2D runtime.

Controls:
- `WASD`: move
- Arrow keys: fire
- `Shift`: dash
- `R`: restart run
- `Esc`: quit

Run loop:
- Clear every wave in the room.
- Pick up the reward core or heart.
- Walk into the portal at the top edge to enter the next room.
- Finish all three rooms to win the run.

This slice is intentionally feature-owned: gameplay commands, schemas, assets, diagnostics-facing context, and smoke coverage all live under `Game/Features/RogueLite`.
