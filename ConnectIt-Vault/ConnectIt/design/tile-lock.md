# Tile lock: the "not placeable" state

Design note (2026-10-22). **Nothing decided yet, nothing built.** Needed by [chained tiles](chained-tiles.md) and by the
tutorial ([adventure-modules-and-progression](adventure-modules-and-progression.md): guided placement in the 4x4 steps,
"some tiles aren't placeable" in the Level 1 draft).

## What a locked tile is

A **locked** tile is on the board, visible and in play, but a player **can't place a piece on it**. It is not a hole:

| | Locked (new) | Inactive (exists: `bIsActive = false`) |
| --- | --- | --- |
| On the board, drawn as a tile | Yes (drawn locked: chains, a padlock...) | Out of play (a hole, or Crumble's lava) |
| Player can place on it | No | No |
| Can hold a piece | Yes -- a starting piece, a forced one, or one placed before it locked | No (in practice) |
| Its piece counts in lines | Yes | -- |
| An empty one breaks a line | Yes, like any empty tile | Yes |

## Today's code (what the lock plugs into)

- **One placement rule**: `FConnectItBoardOperation_PlacePiece::IsTilePlaceable` (active and not occupied). Everything
  that asks "can a piece go here?" already calls it: Place Piece's `CanApply`, its legality (`No Legal Move`), the AI's
  move generation, the client's hover highlight (`ConnectIt_GameState`, `ConnectIt_GameUtilityLibrary`,
  `ConnectIt_BoardStateLibrary`). **Adding the lock there covers all of them at once.**
- **`bIsOccupied` is not the lock.** Its comment says "active but not placeable (non player piece occupying)", but
  `SetFactionPiece` overwrites it from `FactionPiece`, so in practice it means "has a piece". Using it as a lock would
  break the first time a piece moves. (Its comment should be corrected when the lock is built.)
- **Force Place** deliberately skips the placement rule (level scripts, later powers) -- a forced piece can land on a
  locked tile.
- **Board Shift** moves whole tile data, so a lock travels with its tile, like `bCanShift` and a tile power.

## Proposed shape (Claude)

- **State:** `bIsLocked` on `FConnectItTileData` (replicated with the board; Blueprint-readable for the tile visual).
  `IsTilePlaceable` = active **and** not occupied **and** not locked.
- **Starting value:** `bIsLocked` on `FConnectItTileInitialData`, so a placed tile and a layout cell can start locked.
- **Changing it:** a board operation **Set Tile Locked {Position, bLocked}** with event **Tile Lock Changed {Position,
  bLocked}** -- usable by the Level director (guided tutorial steps: lock all but the target, unlock when done), by tile
  powers later, and by the chained-tile rule. A lock change is a **Tile Change** for tile-power triggers.
- **Locking doesn't remove a piece.** A locked tile may hold one. Chained tiles add "and destroy its piece" as their own
  rule (lock + Remove Piece), not as part of the lock.
- **One flag, one owner per tile.** A chained tile's lock belongs to its chain rule; a script shouldn't also lock it.
  (Several reasons at once -- a lock-reason tag container -- only if a level ever needs it.)
- **Why-not for the UI:** a helper returning *why* a tile isn't placeable (inactive / occupied / locked) so a refused
  click can say "This tile is locked" vs "This tile already has a piece" -- the tutorial's phase 2 and 3 messages.
- **The AI:** move generation respects the lock through `IsTilePlaceable` with no change. Its line evaluation
  (`ConnectIt_MinMaxRules`, "open window" check) should treat an **empty locked** tile as blocking a window, since nothing
  can be placed there; it can't foresee a chain unlocking.

## Open (owner)

- **Which other moves does a lock stop?** Place Piece: yes. Force Place: no. Shift: no (the lock moves with the tile).
  Still to decide:
  - **Swap** with a piece on a locked tile (would move a different piece onto it);
  - **Capture / Remove** of a piece on a locked tile;
  - a power's **Move Piece** landing on a locked tile.
- Does the AI's evaluation treat an empty locked tile as blocked (lean: yes)?
