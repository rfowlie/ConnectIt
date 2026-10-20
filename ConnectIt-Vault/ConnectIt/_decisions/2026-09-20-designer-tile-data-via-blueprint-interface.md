---
Date: 2026-09-20
status: Active
superseded by:
tags:
  - board-state
  - level-design
  - tiles
---
## Decision

Designer-set starting values on a placed tile reach the board state through an **interface the tile
Blueprint implements**: `IConnectIt_TileDataProvider::GetInitialTileData()` returns an
`FConnectItTileInitialData` (`Multiplier`, `bIsActive`, `bCanShift`, `StartingFactionPiece`, where -1 =
empty). `UConnectIt_BoardStateComponent::InitialiseBoardState` asks every tile that implements it; a tile
that doesn't starts with the defaults (multiplier from the GameMode's `InitialMultiplier`, active,
shiftable, empty). An out-of-range `StartingFactionPiece` logs a warning and starts empty.

Starting pieces get their visuals through a new one-shot event: the initial snapshot's change event
carries `bBoardSeeded`, `EnqueueBoardEventTags` queues `ConnectIt_Event_BoardSeeded` on the server and (via
`OnRep_BoardSnapshot`) on clients, and a Blueprint reaction spawns/activates a piece per starting piece
using the same path as the PlacePiece visual. Starting pieces add no score.

## Why

The board state was rebuilt from defaults at match start, so anything a designer set on a tile (a starting
multiplier of 3) was silently reset to 1 — `AGridTileBase` has no data properties and nothing read the tile
actors. The owner keeps the values as variables on the tile Blueprint per placed instance, so an interface
avoids reparenting the tile Blueprints; the same mechanism covers starting pieces so designers can build
starting positions that are hard to win from. A separate struct is used because `FConnectItTileData`'s fields
are `BlueprintReadOnly` and a Blueprint can't Make it.

## What Would Change It

The 2026-09-20 "`ChangeEvent` is an ordered step list" design would replace the `bBoardSeeded` flag with a
seed step. A late-joining client receives the latest snapshot without the seed flag and would not create
starting-piece visuals from the event — needs a state-based sync if late joins matter.
