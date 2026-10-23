---
schema: code
kind: USTRUCT
role: primary
source:
  - Source/ConnectIt/Public/ConnectIt_Structs.h
reconciled: 2026-10-23
commit: bcc5319
---

# ConnectItStructs

The game's board data model, in `ConnectIt_Structs.h`.

## When you touch this

- Any board state read/write; a new piece of state the board carries (it replicates and is copied by the AI's search).

## Entry points

### `FConnectItBoardState` — the authoritative board

Parallel arrays (`TMap` can't replicate): `TilePositions` + `TileDataArray`. Also `FactionTurn`, `ScoreBoard` (index =
faction slot), `TurnNumber` (the turn the board is at, stamped by whoever resolves a change on it -- the Mediator,
Simulate Operation's caller, the AI's search root, +1 per ply; tile power cooldowns count against it; was
`LastModifiedTurn`, with a CoreRedirect), `RandomSeed` (the board's random stream: powers that roll advance it, so every
copy rolls the same), `NextTileId`, `LevelWideReadyTurns` (per level-wide power, same order as the rules'; a level-wide
power cools down board-wide), `bGameOver`, `WinningFactionSlot`, `LosingFactionSlot` (the side
that met a lose condition, else -1), `TargetScore` (0 = the goal isn't score-based → UI hides the bar), `WinProgress`
(0..1 per faction), `GroupsCompleted` (each faction's progress through its ordered goals), `StartingPieces` (per faction;
piece-count goals measure from it; added tiles' starting pieces count), `CurrentPhase` (a phased level's
`ConnectIt.Phase.*` tag, empty until one begins).
Helpers: `GetTileData` / `GetTileDataMutable` / `SetTileData` (add or update), **`AddTile`** (a tile joins the board:
gives it the next id -- use it, not `SetTileData`, for a new tile), `RemoveTileData` (the other tiles keep their order),
`FindTileIndexById`, `LinkChainPartner(Position, bHasPartner, PartnerPosition)`, `IsTileOccupied`, `IsTileActive`,
`GetScore`, `NumTiles`, `GetPositionAt` / `GetTileDataAt`, `HasSameTilesAndScores(Other)` (tiles by position and
scores, not match-level fields -- checks that a board stepped along by its events arrived where the change did).

### `FConnectItTileData`

`FactionPiece` (-1 = empty), `Multiplier` (bumps when part of a scoring line), `bIsActive` (inactive tiles can't be
selected), `bIsOccupied` (holds a piece), `bCanShift` (an unshiftable tile is skipped during a shift rotation),
`bIsLocked` (in play but not placeable; only operations that say so act on it --
[[lock-a-tile-or-chain-tiles]]), `TileId` (given by the board when the tile joins it, never reused or authored; travels
with the tile, so links between tiles store it, not a position), `TilePowerTag` (the tile's power -- only the tag; the
definition is in the rules), `PowerReadyTurn` (the first turn its power can fire again), `ChainPartnerId` (a Chained
tile's partner's `TileId`, or -1). `SetFactionPiece` keeps `bIsOccupied` in sync. Built from designer values by `FConnectItTileInitialData::ToTileData`
(`ConnectIt_TileDataProvider.h`), for placed tiles and layout cells alike.

### `FConnectItBoardChangeEvent` — what happened in one board change

An ordered list, `Events` (`TArray<FInstancedStruct>`), each entry one board event struct
(`ConnectIt_BoardEvents.h`: Piece Placed, Scored, Tiles Added, Phase Changed, Game Won...) in the order it happened.
`Add(Event)`, `FindFirst<TEvent>()`. Built on the server as the change is carried out (the operation, the tile power
chain, the scoring rule, the win stamp each append theirs -- [[tile-power-chain]]); played one event at a time on every machine, each queued with itself as payload.

### `FConnectItBoardStateSnapshot` — the ONE replicated property

`PreviousState` + `CurrentState` + `ChangeEvent`, arriving together. Lives on
[[UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]]. One snapshot = one board change; the server makes no
new change until the last has played out (one board change at a time).

Requests are no longer structs here: a board operation (`ConnectIt_BoardOperations.h`) **is** the request payload.

## Gotchas

- Look tiles up by position (`GetTileData(Position)`, linear `IndexOfByKey`) -- an index into `TileDataArray` is only
  good for one board: tiles can be removed and added mid-match (phased levels).
- Keep it plain data -- no UObject references: the AI's background search copies it per node.
- `GameWon` (event, edge) vs `bGameOver` (state, latched) — don't confuse them.
- `TargetScore == 0` means "not score-based," not "target is zero."

## Cross-impact

`FConnectItBoardState` / `…Snapshot` shape ripples into replication, `UConnectIt_BoardStateLibrary`, every UI reader,
the MinMax AI (which copies boards per node -- watch Throughput when adding fields), and the debug widgets. A new kind
of change is a new board event struct, not a field here ([[add-a-board-request-type]]).

## Changes

- 2026-10-23 — tile powers and the tile lock: tile data gained `bIsLocked`, `TileId`, `TilePowerTag`, `PowerReadyTurn`,
  `ChainPartnerId`; board state gained `TurnNumber` (replaces `LastModifiedTurn`), `RandomSeed`, `NextTileId`,
  `LevelWideReadyTurns`, and `AddTile`, `FindTileIndexById`, `LinkChainPartner`, `HasSameTilesAndScores`. See
  [tile powers in the after-move chain](../_decisions/2026-10-22-tile-powers-in-the-after-move-chain.md),
  [the tile lock](../_decisions/2026-10-23-tile-lock-per-operation-rule.md),
  [chained tiles](../_decisions/2026-10-23-chained-tiles-as-a-sync-power.md). (`process-code` sweep — commit
  `bcc5319`.)
- 2026-10-22 — the change event is now an **ordered list of event structs** (the per-kind flags and fields are gone);
  request payload structs removed (operations are the requests); board state gained `LosingFactionSlot`,
  `WinProgress`, `GroupsCompleted`, `StartingPieces`, `CurrentPhase` and `RemoveTileData`; `IsTileValidForPlacement`
  moved to the Place Piece operation.
- 2026-09-18 — **Board Shift fields added**: `FConnectItTileData::bCanShift`;
  `FConnectItBoardChangeEvent::bBoardShifted` + `ShiftDirection`/`ShiftAnchorPosition`/
  `ShiftStartPositions`/`ShiftEndPositions`; new `FConnectItRequestBoardShift` payload.
  (`process-code` sweep — commit `9187568`, "Board Shift Backend Setup".)
- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[ConnectIt/CLAUDE|ConnectIt overview]].
