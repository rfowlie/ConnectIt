---
schema: code
kind: USTRUCT
role: primary
source:
  - Source/ConnectIt/Public/ConnectIt_Structs.h
reconciled: 2026-10-22
commit: 1e0d5cf
---

# ConnectItStructs

The game's board data model, in `ConnectIt_Structs.h`.

## When you touch this

- Any board state read/write; a new piece of state the board carries (it replicates and is copied by the AI's search).

## Entry points

### `FConnectItBoardState` — the authoritative board

Parallel arrays (`TMap` can't replicate): `TilePositions` + `TileDataArray`. Also `FactionTurn`, `ScoreBoard` (index =
faction slot), `LastModifiedTurn` (unused, always 0), `bGameOver`, `WinningFactionSlot`, `LosingFactionSlot` (the side
that met a lose condition, else -1), `TargetScore` (0 = the goal isn't score-based → UI hides the bar), `WinProgress`
(0..1 per faction), `GroupsCompleted` (each faction's progress through its ordered goals), `StartingPieces` (per faction;
piece-count goals measure from it; added tiles' starting pieces count), `CurrentPhase` (a phased level's
`ConnectIt.Phase.*` tag, empty until one begins).
Helpers: `GetTileData` / `GetTileDataMutable` / `SetTileData` (add or update), `RemoveTileData` (the other tiles keep
their order), `IsTileOccupied`, `IsTileActive`, `GetScore`, `NumTiles`, `GetPositionAt` / `GetTileDataAt`.

### `FConnectItTileData`

`FactionPiece` (-1 = empty), `Multiplier` (bumps when part of a scoring line), `bIsActive` (inactive tiles can't be
selected), `bIsOccupied`, `bCanShift` (an unshiftable tile is skipped during a shift rotation). `SetFactionPiece` keeps
`bIsOccupied` in sync. Built from designer values by `FConnectItTileInitialData::ToTileData`
(`ConnectIt_TileDataProvider.h`), for placed tiles and layout cells alike.

### `FConnectItBoardChangeEvent` — what happened in one board change

An ordered list, `Events` (`TArray<FInstancedStruct>`), each entry one board event struct
(`ConnectIt_BoardEvents.h`: Piece Placed, Scored, Tiles Added, Phase Changed, Game Won...) in the order it happened.
`Add(Event)`, `FindFirst<TEvent>()`. Built on the server as the change is carried out (the operation, the scoring rule,
the win stamp each append theirs); played one event at a time on every machine, each queued with itself as payload.

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
