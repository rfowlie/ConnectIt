---
Date: 2026-10-23
status: Active
superseded by:
tags:
  - architecture
  - board
  - tile-lock
summary: 'A locked tile is in play but not placeable; each operation declares whether it may act on locked tiles, checked once by FConnectItRuleSet::CanCarryOut for players, the level, powers and simulation; Set Tile Locked; the AI treats an empty locked tile as blocking'
---
## Decision

A tile can be **locked**: on the board and in play (its piece counts in lines), but no piece can be placed on it, and only
operations that say so act on it.

- `bIsLocked` is board data (`FConnectItTileData`), set by the tile's settings (a builder tile, a layout cell) and by
  **Set Tile Locked**. It travels with its tile through shifts.
- **Each operation declares whether it may act on locked tiles** (`CanOperateOnLockedTiles`, default no). Today yes:
  Shift, Force Place, Remove Tiles, Change Layout, Set Tile Locked, Set Chain Partner.
- **One check for every caller:** `FConnectItRuleSet::CanCarryOut` = the operation's own `CanApply`, plus "no locked
  target" (`GetTargetPositions`) unless it may. The Mediator (players and the Level director), Simulate Operation and
  tile power effects all ask it.
- **Placement** (`IsTilePlaceable`) also excludes locked tiles, so the hover highlight, the AI's moves and No Legal Move
  follow. The AI's evaluation treats an empty locked tile as blocking a line.

## Why

- **Locked, not inactive:** a tutorial or a Chained tile needs a tile that is visibly part of the board, holds and
  scores pieces, but isn't a free placement. Inactive means out of play.
- **Per operation, in one check:** whether a move may touch a locked tile differs by kind (a shift carries the lock
  along; a capture shouldn't reach in). Declaring it on the operation and checking it once keeps every caller -- player,
  level, power, simulation -- the same.

## What Would Change It

- Several reasons to lock one tile at once (a lock-reason container); today a tile has one lock.
- A new operation that needs a different answer per target (one tile locked is fine, another isn't).

Plan: [tile-lock-and-chained-tiles-plan](../design/tile-lock-and-chained-tiles-plan.md) Stage 1. Design:
[tile-lock](../design/tile-lock.md).
