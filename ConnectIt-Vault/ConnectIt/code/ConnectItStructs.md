---
schema: code
kind: USTRUCT
role: primary
source:
  - Source/ConnectIt/Public/ConnectIt_Structs.h
reconciled: 2026-09-07
commit: 668872e
---

# ConnectItStructs

The game's board data model, in `ConnectIt_Structs.h`.

## When you touch this

- Any board state read/write; adding a board-change kind or a request type.

## Entry points

### `FConnectItBoardState` — the authoritative board

Parallel arrays (`TMap` can't replicate): `TilePositions` + `TileDataArray`. Also
`FactionTurn`, `ScoreBoard` (index = faction slot), `LastModifiedTurn`, `bGameOver`,
`WinningFactionSlot`, `TargetScore` (0 = win condition isn't score-based → UI hides the
bar). Helpers: `GetTileData` / `GetTileDataMutable` / `SetTileData`, `IsTileOccupied`,
`IsTileActive`, `IsTileValidForPlacement` (active && !occupied), `GetScore`, `NumTiles`,
`GetPositionAt` / `GetTileDataAt`.

### `FConnectItTileData`

`FactionPiece` (-1 = empty), `Multiplier` (bumps when part of a scoring line), `bIsActive`
(inactive tiles can't be selected — used by mutations like "The Rift"), `bIsOccupied`
(active but not player-placeable). `SetFactionPiece` keeps `bIsOccupied` in sync.

### `FConnectItBoardChangeEvent` — "what changed" on the last commit

Rides inside the snapshot so it replicates atomically. Per-kind flag + fields:
`bPiecePlaced` (+ `PlacedPosition`, `PlacingFactionSlot`), `bLineScored` (+
`ScoringFactionSlot`, `PointsScored`, `ScoringLinePositions` — union across simultaneous
lines), `bGameWon` (**edge-triggered**; + `WinningFactionSlot`), plus
`bTileMultiplierDestroyed`, `bPieceRemoved`, `bPiecesSwapped`, `bTileActiveToggled`,
`bPieceCaptured` and their position/faction fields. Each corresponds to a
`UConnectIt_*Action` (named in the header comments).

### `FConnectItBoardStateSnapshot` — the ONE replicated property

`PreviousState` + `CurrentState` + `ChangeEvent`, arriving together. Lives on
[[UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]].

### `FConnectItRequest*` payloads (wrapped in `FTurnActionRequest::Payload`)

`FConnectItRequestPlacePiece` (`Positions`), `…ForcePlacePiece` (`Position`; same shape
as place, kept separate to diverge later), `…DestroyTileMultiplier`, `…RemovePiece`
(`Position`, `DelayTurns` — only 0 honoured), `…SwapPieces` (`PositionA`/`B`),
`…ToggleTileActive`, `…CapturePiece`. **`FactionID` is on the `FTurnActionRequest`
envelope**, never duplicated into a payload.

## Gotchas

- Parallel-array access only — never index `TileDataArray` by grid position; use
  `GetTileData(Position)` (linear `IndexOfByKey`).
- `bGameWon` (event, edge) vs `bGameOver` (state, latched) — don't confuse them.
- `TargetScore == 0` means "not score-based," not "target is zero."

## Cross-impact

`FConnectItBoardState` / `…Snapshot` shape ripples into replication,
`UConnectIt_BoardStateLibrary`, every UI reader, the MinMax AI (which builds hypothetical
`FConnectItBoardState`s), and the debug widgets.

## See also

- In-repo: `old/Source/ConnectIt/Docs/UIValueCatalogue.md`; `old/Source/ConnectIt/Docs/Workflows/SingleSourceOfTruth-Replication.md`.
