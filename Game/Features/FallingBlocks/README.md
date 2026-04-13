# Falling Blocks Feature

This feature is the W17 falling-block sample built on top of the integrated
SHE runtime.

Controls:

- move with `A/D` or left/right
- rotate clockwise with `W`, up, or `X`
- soft drop with `S` or down
- hard drop with `Space`
- restart with `R`
- quit with `Esc`

Runtime notes:

- uses a `10x20` board
- uses a deterministic `7-bag` piece queue
- routes player actions through the shared gameplay command/event service
- renders the board, HUD, next-piece preview, and game-over state through the
  accepted 2D renderer baseline
