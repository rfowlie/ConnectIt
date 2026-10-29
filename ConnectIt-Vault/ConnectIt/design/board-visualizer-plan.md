---
type: plan
status: not-started
approved: true
approved-on: 2026-10-29
completed-on:
playtested: false
design: "[board-visualizer](board-visualizer.md)"
question:
decision:
---

# Plan: the board visualizer (one per map, a function per event, completion handles)

Design: [board-visualizer](board-visualizer.md) (decided 2026-10-29). Written 2026-10-29 in a design-only session;
**approved by the owner 2026-10-29**. Not built -- building happens in its own session.

## Context

Settled with the owner on 2026-10-29:

- **Registries:** typed getters on the existing ConnectIt subclasses (`UConnectIt_TileRegistry`,
  `UConnectIt_PieceRegistry`) so callers never cast; interfaces only for plugin-level contracts (`RefreshFromBoard`,
  hover).
- **One visualizer per map:** `AConnectIt_BoardVisualizer`, an **actor**, its class set on the **level config** next to
  the registries; artists subclass it per look. It replaces `UConnectIt_TileVisualizer` (a C++-only UObject) and the
  separate piece visual handler (`CI_PieceVisualHandler`).
- **C++ binds every board event** (found by reflection over `FConnectItBoardEvent` structs, not a hand-kept list) and
  calls a **Blueprint-native function per event, with its struct**. Each default calls the actors' own hooks; a
  designer overrides to **add** (call the parent) or **replace** (don't) visuals for any event.
- **Completion:** one **handle per playing event** with explicit **`Hold()` / `Release()`**, replacing the counted
  `UConnectIt_TileReaction`s. The event queue's gate stays; the event completes when every hold is released (or at once
  if none), with a safety timeout.

Each stage builds and passes the tests before the next; the editor must be closed for builds; the owner commits between
stages.

## Stage 1: typed registry getters

- `UConnectIt_TileRegistry::GetTileAt(Position) → AConnectIt_TileBase*`, `GetTilesAt(Positions)`; 
  `UConnectIt_PieceRegistry::GetPieceAt(Position) → AConnectIt_PieceBase*` -- Blueprint-pure; the cast inside, null
  (with a log) for an actor of the wrong class.
- Replace casts in ConnectIt code that only exist to reach these (visualizer, actions, layout tool) with the typed
  getters.
- Tests: none without a world; existing tests pass.

## Stage 2: the completion handle

- **`UConnectIt_VisualHandle`** (`UObject`): `Hold()` / `Release()` (Blueprint-callable), `IsComplete()`; an
  `OnComplete` delegate; a safety timeout (configurable on the visualizer, e.g. 10 s) that logs which hold never
  released and completes.
- One handle per playing board event, created by the visualizer when the event's gated task starts; the task completes
  when the handle does. A hold / release after completion is ignored with a warning.
- Tests: no holds → complete at once; two holds → complete after both releases; a double release doesn't go negative;
  the timeout completes and logs (unit-level with a test world where needed).

## Stage 3: the visualizer actor and its event functions

- **`AConnectIt_BoardVisualizer`** (`Public/Board/`): spawned by `UConnectIt_BoardRegistrySubsystem` from the level
  config's **`VisualizerClass`** (default: the C++ base), after the registries; one per map.
- **Binding:** at start it enumerates every concrete `FConnectItBoardEvent` struct (reflection, cached like
  `FConnectItBoardOperation::GetAllRequestTypes`), registers a gated task for each event's tag, and on execute reads the
  queue's active payload, makes a handle, and calls the typed function. An event struct with no function falls to
  **`OnBoardEvent(Payload, Handle)`** (generic, Blueprint-native) -- so nothing new is ever unhandled.
- **One Blueprint-native function per event with its struct and the handle**, e.g. `OnPiecePlaced`, `OnPiecesSwapped`,
  `OnBoardShifted`, `OnPieceCaptured`, `OnPieceRemoved`, `OnPieceMoved`, `OnScored`, `OnMultiplierChanged`,
  `OnTileLockChanged`, `OnTileActiveToggled`, `OnTilesAdded`, `OnTilesRemoved`, `OnLayoutChanged`, `OnTilePowerTriggered`,
  `OnChainPartnerChanged`, `OnPhaseChanged`, `OnGoalsChanged`, `OnWinCheckpointReached`, `OnLoseConditionMet`,
  `OnGameWon`, `OnBoardSeeded` (the list follows the event structs that exist when built).
- **Defaults** call the actors' hooks with the same handle (Stage 4), plus the registry bookkeeping each event needs
  (piece re-keying on move / swap / shift / remove, tile moves on shift, spawning / removing tiles and pieces).
  **Registry bookkeeping stays in C++ and runs even when a Blueprint override doesn't call the parent** -- overriding a
  visual must never desync the registries.
- **Effects:** being an actor, a visualizer Blueprint spawns and destroys effects (Niagara, meshes, sounds) and can use
  timelines / delays; it cleans up with the map.
- Tests: every event struct is bound (a test fails when an event struct has no tag); a handle completes the task.

## Stage 4: actor hooks take the handle

- **Tiles:** `AConnectIt_TileBase`'s hooks (`OnTilePowerTriggered`, `OnPieceEnter`, `OnPieceExit`,
  `OnMultiplierChanged`, `OnLockChanged`, `OnActiveChanged`, `OnChainPartnerChanged`) take a `UConnectIt_VisualHandle*`
  instead of a `UConnectIt_TileReaction*`; a hook that animates holds and releases.
- **Pieces:** `AConnectIt_PieceBase` gains the same kind of hooks -- `OnPlaced`, `OnMoved(From, To)`, `OnSwapped`,
  `OnCaptured(NewFaction)`, `OnRemoved`, `OnClearedByScore` -- each with the handle (today it only has
  `OnFactionVisualUpdate`).
- `UConnectIt_TileReaction` and `UConnectIt_TileVisualizer` are removed.

## Stage 5: event cause helpers

From [editor-notes-2026-10-28](editor-notes-2026-10-28.md): `GetActiveBoardEventIndex()` and `GetBoardEventCause()`
(the active event's cause, typed like the other getters) in `UConnectIt_BoardEventLibrary`, and on the visualizer as
convenience -- so e.g. `OnPieceMoved` can ask whether a tile power caused it.
- Tests: a power's Piece Moved reports its Tile Power Triggered as the cause; the action's own event has none.

## Stage 6: content migration (owner, editor)

- A `BP_BoardVisualizer` subclass; set it on the level configs.
- Move `CI_PieceVisualHandler`'s piece visuals into piece hooks (`OnMoved`...) or the visualizer's overrides; delete
  the handler.
- Tile Blueprints: their hooks now take a handle (Hold / Release around animations).
- PIE through place, swap, shift, scoring, a power chain and a layout change.

## Stage 7: vault

Decision note (one visualizer actor per map, per-event functions, completion handles, typed registry getters); a
`code/` recipe "add visuals for a board event"; update [board-events](board-events.md) and the board-state flow page;
log.

## Not in this plan

- The teaching layer's guidance visuals (a later plan).
- Camera focus on events (the camera plan registers its own earlier-phase task).

## Verification

- Each stage: build (editor closed); `Automation RunTests ConnectIt` passes.
- Owner, PIE: every board event plays through the visualizer; overriding `OnPieceMoved` in a Blueprint without calling
  the parent replaces the move visual and the pieces still end on the right tiles; a hook that forgets `Release()` is
  logged by the timeout and the game continues; a 2-player client plays the same visuals.

## Settled before approval (owner, 2026-10-29: as written)

1. The visualizer's class is set on the **level config** (with the registries), defaulting to the C++ base.
2. Registry bookkeeping runs in C++ regardless of Blueprint overrides.
3. An unhandled event type falls to a generic `OnBoardEvent(Payload, Handle)`.
