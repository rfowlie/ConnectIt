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

**Grid convention: X runs left-right, Y runs up-down** (the owner's map-building picture).
Right = +X, Up = +Y. `FGridDirectionVector`'s first field (`Row`) is the X step and its second
(`Column`) is the Y step. A vertical line holds X fixed and varies Y; a horizontal line holds Y
fixed and varies X.

The fix is in `UGridTileRegistryBase::GetTilesByDirection`, which had this backwards:

- Up/Down now collect tiles with `TilePosition.X == StartPosition.X` (was `Y == Y`).
- Left/Right now collect tiles with `TilePosition.Y == StartPosition.Y` (was `X == X`).
- Diagonals unchanged ("/" holds X−Y constant, "\" holds X+Y constant).

`GridDirectionVectors` (Up = (0,1), Right = (1,0), …) was already correct and is back to its
committed values — only a comment was added.

Supersedes [2026-09-20 — Grid X is row, Y is column](2026-09-20-grid-x-is-row-y-is-column-direction-table.md),
which read the owner's "rows along X" the other way round and edited the table instead.

## Why

Board Shift sorts its line by signed distance along the direction vector. With Up = +Y but a
"vertical" line that held Y fixed, that distance was 0 for every tile on an Up/Down line (and
Left/Right likewise), so the sort was a no-op: Up and Down came out in the same registry order and
one was always backwards. The "\" diagonals were inverted by a related effect. With the line
function matching the table, every sort key is non-degenerate.

## Known loose end

`UGridTileRegistryBase::GetRowPositions` / `GetRow` select `Pos.X == RowIndex` — a fixed-X line,
which in this convention is a *column* (vertical). The names read the opposite way from the map
picture. Nothing in the shift path uses them (the older `BoardShiftComponent` does); not renamed
here. Worth a decision before any new code relies on them.

## What Would Change It

If a level's camera puts world +Y on screen-left/right instead of up/down, the same
one-table/one-function pair is where to adapt — but the map-building convention above is the
stated intent.
