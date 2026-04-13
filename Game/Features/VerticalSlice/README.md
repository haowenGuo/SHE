# Vertical Slice Feature

This feature is the W12 playable slice for the integrated engine baseline.

Player loop:

- move with `WASD` or arrow keys
- collect three signal cores
- avoid red patrol drones
- press `R` after a win or loss to restart
- press `Esc` to quit

Engine surfaces exercised here:

- gameplay commands, events, and timers
- feature-owned schema registration and authored records
- script-module registration plus command-routed invocation
- scene entities and renderer-driven sprite submission
- Box2D-backed sensor collisions for pickup and fail states
- gameplay-routed audio playback
- shared debug/UI/AI context exports
