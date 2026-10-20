---
Date: 2026-09-20
status: Superseded
superseded by: [2026-09-20-grid-x-horizontal-y-vertical-line-function-fixed](2026-09-20-grid-x-horizontal-y-vertical-line-function-fixed.md) — this note's convention (X = row, Up = +X) was a misreading of the owner's map layout; the direction table change described here was reverted the same session
tags:
  - grid
  - board-shift
  - convention
---
## Decision

**Grid convention: X is the ROW index, Y is the COLUMN index.** An `FGridDirectionVector`'s
`Row` field is the step in X and its `Column` field the step in Y. Up/Down therefore move
between rows (±X) and Left/Right between columns (±Y). `EGridDirection::Up` is +X and `Right`
is +Y, so the eight entries run clockwise.

`UGridMechanics_GridLibrary::GridDirectionVectors` (the direction table) was the one place
that disagreed and has been corrected:

| Direction | Before (Row, Col) | After |
|---|---|---|
| Up | (0, 1) | (1, 0) |
| UpRight | (1, 1) | (1, 1) |
| Right | (1, 0) | (0, 1) |
| DownRight | (1, -1) | (-1, 1) |
| Down | (0, -1) | (-1, 0) |
| DownLeft | (-1, -1) | (-1, -1) |
| Left | (-1, 0) | (0, -1) |
| UpLeft | (-1, 1) | (1, -1) |

`GetTilesByDirection`, `GetRowPositions`/`GetColumnPositions`, `GetClosestGridDirection…`, the
shape/scoring line walks and Board Shift's sort already used Row = X / Column = Y, so they
needed no change.

## Why

The mismatch was the root cause of Board Shift's direction inversion. `GetTilesByDirection`
walks Up/Down along X (same Y), but the table gave Up/Down a Y step. Board Shift sorts its line by
the signed distance along the direction vector, which for Up/Down was `dy * ±1` = 0 for every
tile on the line — every key tied, so the list kept the registry's discovery order and Up and
Down came out identical (one always backwards). The "\" diagonals (UpLeft/DownRight) were
inverted for a related reason. Fixing the table makes every key non-degenerate.

The line-walking users (scoring, shape library) enumerate axes rather than orientations, and
the corrected table still covers the same four axes, so they are unaffected.

## What Would Change It

If the game's camera puts screen-up on −X or on Y, the sign/axis of Up in this table is the one
thing to flip — deliberately a single table so it stays a one-place change.
