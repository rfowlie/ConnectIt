---
Date: 2026-10-07
status: Active
superseded by:
tags:
  - architecture
  - board
  - rules
---
## Decision

Two calls from the owner's review of the board-operations work.

**Placement is one concrete function, not a swappable rule.**
`FConnectItBoardOperation_PlacePiece::IsTilePlaceable(Board, TileIndex)` (and `IsTilePlaceableAt` for a grid position)
is the single definition of "a piece may go on this tile". The server's check of a place request, the AI's move
generation and the client's hover highlight all call it. `FConnectItTilePlaceableRule` (+ `_Unoccupied`), the rule
set's `TilePlaceableRule` slot and the duplicate `FConnectItBoardState::IsTileValidForPlacement` are removed. This
reverses the placement part of [rules are thread-safe structs](2026-10-06-rules-are-thread-safe-structs.md).

**The change event reports scoring as a list of scoring configurations.**
`FConnectItBoardChangeEvent::ScoringConfigurations` is an array of `FConnectItScoringConfiguration { FactionSlot,
Points, Positions }`, one per thing that scored (for the Lines rule: one completed line). It replaces `bLineScored`,
`ScoringFactionSlot`, `PointsScored` and `ScoringLinePositions`. Scoring rules append configurations through an
optional output; the AI's search passes none. The event tag is `ConnectIt.Event.Scored` (was `...LineScored`) and the
stub game event class is `UConnectIt_ScoreGameEvent`; both have redirects.

## Why

Placement may come to depend on many factors; a swappable rule struct per combination would scatter them across small
types for a flexibility nothing uses. One function keeps every condition in one place, and there were already two
definitions that could drift apart.

Scoring need not be lines, so "line" in the event's names was wrong. A list also removes a known limitation: the old
event could name only one scoring faction and merged every scoring line into one set of tiles.

## What Would Change It

A level that needs a different placement condition from another level (then the function gains data on the operation,
or the swappable rule returns). A scoring rule whose result doesn't fit "who, points, tiles".
