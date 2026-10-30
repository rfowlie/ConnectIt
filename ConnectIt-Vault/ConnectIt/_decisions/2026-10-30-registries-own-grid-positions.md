---
Date: 2026-10-30
status: Active
superseded by:
tags:
  - architecture
  - visuals
  - board-events
summary: 'The registries own where grid actors are: a piece''s GridPosition is a cache only the registry writes (in the same call that maps it); hooks get a copy of their event plus the handle; the visualizer reconciles every actor against the registries after each event and each played-out change (error + fix, CVar ConnectIt.Visualizer.Reconcile); single-event visuals (one typed class per event kind, listed on the level config) replace a default function'
---
## Decision

**The registries are the source of truth for where a grid actor is. Visuals get event copies, can be swapped per
event, and are checked afterwards.**

- **Position ownership:**
  - `AGridPieceBase::GridPosition` is private and Blueprint-read-only. Only the piece registries write it, through
    `UGridPieceRegistryBase::SetPieceAt`, in the same call that maps the piece (get-or-create, map, move, swap, shift,
    update mappings).
  - A position an `InstantiatePiece` override sets is overwritten.
  - `AConnectIt_PieceBase::OccupiedPosition` and `SetBoardPosition` are gone, and `InitialisePiece` takes only the
    faction. `PieceMap` is read-only to Blueprint.
- **Hooks take a copy of the event plus the handle:**
  - typed for the piece hooks and for the tile hooks with one source;
  - a hook that another kind of event leads to gets an event **made for it**, whose `CauseIndex` is that event. For
    example, a starting piece's `Piece Placed` points at the `Board Seeded`, and a shifted piece's `Piece Moved` at the
    `Board Shifted`;
  - `OnPieceEnter` / `OnPieceExit` take the source event as an `FInstancedStruct` (several kinds lead there) plus the
    faction, and `GetBoardEventTag` reads its tag.
- **Reconcile fallback:**
  - after each event's visuals, the visualizer checks the actors the event named, found by reflection over the event's
    positions;
  - once a change has played out (the board state's new `OnBoardEventsPlayed`), it checks every actor and the board
    against the piece registry;
  - each mismatch logs an error naming the actor, where it is, where it belongs, and the last event's handle if it
    timed out. It's put right if it can be: tile → cell, piece position cache, piece → tile;
  - `ConnectIt.Visualizer.Reconcile` sets the mode: 0 off, 1 log, 2 log and fix (the default);
  - reconcile only runs once a handle has completed, so it reuses the handle's 10 s timeout. No second deadline is
    needed, because every event completes through its handle.
- **Single-event visuals:**
  - `AConnectIt_EventVisual` with one thin generated class per event kind (`AConnectIt_EventVisual_PiecePlaced`...),
    each declaring its tag and a typed `OnPlay(Event, Handle)`;
  - they're listed in the level config's `EventVisuals`. An entry for a tag runs **instead of** the visualizer's
    function, and `PlayDefault()` runs that function too. They're spawned on first use;
  - bookkeeping and reconcile run either way;
  - validation reports a duplicate tag as an error, and an empty, abstract or tagless entry as a warning.

## Why

- **Ownership:** the owner's Place Piece miss came from a Blueprint `InstantiatePiece` override that never set the
  piece's position. With the registry writing the cache, that mistake can't be made.
- **Event copies:** a hook gets everything about what happened (faction, cause, positions) in one typed value, instead
  of a parameter list that grows with every need.
- **Reconcile:** a broken visual shows up as a loud, named error (an automation test fails on it) instead of a board
  that is subtly wrong, and the game recovers.
- **Event visuals:** artists can replace one event's look without subclassing the whole visualizer, and the default
  stays a call away.

## Links

- Plan: [board-visualizer-revision-plan](../design/board-visualizer-revision-plan.md); design:
  [board-visualizer](../design/board-visualizer.md) (Round 3).
- Builds on: [one board visualizer per map](2026-10-29-one-board-visualizer-per-map.md).
- Recipe: [add visuals for a board event](../code/recipes/add-visuals-for-a-board-event.md).
