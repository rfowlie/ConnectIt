---
Date: 2026-10-29
status: Active
superseded by:
tags:
  - architecture
  - visuals
  - board-events
summary: 'Board visuals go through one visualizer actor per map (level config Visualizer Class): a Blueprint-native function per board event with a Hold / Release handle, registry bookkeeping in C++ regardless of overrides; typed registry getters; event cause helpers'
---
## Decision

**Board visuals go through one visualizer actor per map, with a function per event and a completion handle; registries
hand back ConnectIt types.**

- **Typed registry getters:**
  - `UConnectIt_TileRegistry::GetTileAt` / `GetTilesAt` and `UConnectIt_PieceRegistry::GetPieceAt` return the ConnectIt
    actor classes, so nobody casts;
  - the utility library returns the ConnectIt registries;
  - interfaces stay for plugin-level contracts (`RefreshFromBoard`, hover).
- **`AConnectIt_BoardVisualizer`, one per map:**
  - an actor, spawned by the board registry subsystem from the level config's **Visualizer Class** (default: the C++
    base) after the registries;
  - artists subclass it per look;
  - it replaces `UConnectIt_TileVisualizer` and the separate piece visual handler.
- **C++ binds every board event** (found by reflection over `FConnectItBoardEvent` structs) and calls a
  **Blueprint-native function per event, with its struct and a handle**:
  - each default calls the actors' hooks; override to add (call the parent) or replace (don't);
  - an event with no function goes to `OnBoardEvent(Payload, Handle)`.
- **Registry bookkeeping is C++ and always runs:**
  - before the visuals: spawn, re-key, unmap;
  - after them: release, destroy, snap moved actors onto their cells;
  - so overriding a visual never desyncs the registries.
- **Completion:** one `UConnectIt_VisualHandle` per playing event, with explicit `Hold()` / `Release()`:
  - the event completes when every hold is released (at once if none);
  - a safety timeout (`HoldTimeoutSeconds`) logs who never released and completes anyway;
  - it replaces the counted `UConnectIt_TileReaction`s.
- **Actors' hooks take the handle:** tiles (`OnPieceEnter`...) and new piece hooks (`OnPlaced`, `OnMoved`, `OnSwapped`,
  `OnCaptured`, `OnRemoved`, `OnClearedByScore`).
- **Cause helpers:** `GetActiveBoardEventIndex`, `GetBoardEventCause` / `GetBoardEventCauseTag` (library and
  visualizer). They work because queued events are stamped with their change and index, and the board state component
  keeps the change until its events have played.

## Why

- **An entry point for artists:** the old visualizer was C++-only and only called fixed tile hooks, so a look couldn't
  add effects without C++.
- **One per map** keeps a level's visuals in one place (several per level got messy) and sits with the registries.
- **Explicit holds** follow the event as a whole: a hold is named and visible, and a forgotten one is logged instead of
  silently freezing the game.
- **Bookkeeping out of Blueprint:** a visual override that skipped a registry call left pieces on the wrong tiles.

## What Would Change It

- Several independent visual layers per map (e.g. a teaching layer): a second listener on the same handle, or a
  visualizer component list.
- Visuals that must run ahead of the board (camera focus registers its own earlier-phase task instead).

Plan: [board-visualizer-plan](../design/board-visualizer-plan.md). Design: [board-visualizer](../design/board-visualizer.md).
Recipe: [[add-visuals-for-a-board-event]].
