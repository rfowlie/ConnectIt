---
Date: 2026-09-20
status: Active
superseded by:
tags:
  - grid
  - board-shift
  - convention
---
## Decision

**Grid axes follow Unreal's world axes: +X is forward (Up), +Y is right (Right).** The owner had
the two axes backwards in the mental model (and in the direction table); after reading the UE
coordinate docs the table was swapped to match:

- `GridDirectionVectors`: Up = (1,0), Right = (0,1), Down = (-1,0), Left = (0,-1), diagonals are the
  sums. `FGridDirectionVector`'s first field (`Row`) is the X step, the second (`Column`) the Y step.
- A vertical line (Up/Down) holds **Y** fixed and varies X; a horizontal line (Left/Right) holds **X**
  fixed and varies Y. `UGridTileRegistryBase::GetTilesByDirection` was rewritten to match (it had
  been fixed for the old table earlier the same day). Diagonals are unchanged: "/" (UpRight/DownLeft)
  holds X−Y constant, "\" (UpLeft/DownRight) holds X+Y constant.
- The swap left `UpLeft` as (-1,-1), a duplicate of `DownLeft`; corrected to (1,-1).
- World<->grid conversion (`WorldToGridPosition`, `CalculateGridPositionFromSize`) was already
  world X -> grid X, world Y -> grid Y, so it needed no change.

Supersedes [X horizontal, Y vertical; line function fixed](2026-09-20-grid-x-horizontal-y-vertical-line-function-fixed.md),
which encoded the old (backwards) picture.

## Why

The table and the world now agree, which also makes rotation consistent:
`GetGridDirectionFromDegrees` maps yaw 0 to Up, and UE yaw 0 faces +X — under the old table Up was
+Y, so an actor facing "Up" by yaw pointed along the wrong grid axis. Board Shift's line sort (dot
product with the direction vector) only works when the line function and the table agree on which
axis a direction moves along, so both had to flip together.

## Loose end resolved

`UGridTileRegistryBase::GetRowPositions` / `GetRow` select `Pos.X == RowIndex`. Under this
convention a fixed-X line is a horizontal row, so those names now read correctly.

## What Would Change It

On screen, "Up" is world +X, so it only looks up if the level camera's yaw matches. A level whose
camera differs needs the camera (or a Blueprint-side mapping) adapted, not the table.
