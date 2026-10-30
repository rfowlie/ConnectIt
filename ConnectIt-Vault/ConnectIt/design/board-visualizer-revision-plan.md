---
type: plan
status: complete
approved: true
approved_on: 2026-10-30
started: 2026-10-30
completed_on: 2026-10-30
stages: 5
current_stage: "complete (code); owner content migration open"
owner_done: false
playtested: false
design: "[board-visualizer](board-visualizer.md)"
question:
decision: "[2026-10-30-registries-own-grid-positions](../_decisions/2026-10-30-registries-own-grid-positions.md)"
requires: "[board-visualizer-plan](board-visualizer-plan.md)"
---

# Plan: board visualizer revision (position ownership, reconcile fallback, event copies, event visuals)

Design: [board-visualizer](board-visualizer.md), "Round 3" (owner's decisions, 2026-10-30). Written 2026-10-30 in a
design-only session; **approved by the owner 2026-10-30**. Builds on the finished
[board-visualizer-plan](board-visualizer-plan.md).

## Context

Settled with the owner on 2026-10-30:

- **Hooks take a copy of the event plus the handle** (not the event split into parameters).
- **Single-event visualizers:** a typed class per event kind that declares its tag, an override list, lazy actors; today's
  per-event functions stay as the default; registry bookkeeping always runs in C++.
- **The registries are the source of truth for where a grid actor is.** A piece's position field is a cache written only
  by C++. Keep **`GridPosition`** (plugin `AGridPieceBase`, has `GetGridPosition`); remove `OccupiedPosition`.
- **The reconcile fallback:** the base visualizer verifies every grid actor against the registries, logs an error and moves
  the ones that are wrong. **Waits for held handles, with a timeout.**
- The cause of the owner's Place Piece miss: a Blueprint override of `InstantiatePiece` on the piece registry that did not
  set the piece's position. Stage 1 makes that impossible.

Each stage builds and passes the tests before the next; the editor must be closed for builds; the owner commits between
stages.

## Stage 1: the registry writes the position cache

- `AGridPieceBase` gets a registry-only setter for `GridPosition` (friend / protected) -- Blueprint cannot write it.
- `UGridPieceRegistryBase` (and `GridPieceRegistryComponent`) set the piece's `GridPosition` **in the same call that maps
  it**: after `InstantiatePiece` returns, and in map / unmap / move / swap / shift. A position set by an
  `InstantiatePiece` override is overwritten, not trusted.
- Remove `OccupiedPosition` from `AConnectIt_PieceBase`; `InitialisePiece` and `SetBoardPosition` stop writing it;
  `RefreshFromBoard` keeps the registry's position. C++ readers move to `GetGridPosition()`.
- If the registry maps a piece at a position the board says is empty (or the reverse), log it.
- Tests: instantiating through an override that sets nothing still yields the right `GridPosition`; map, move, swap and
  shift keep the cache equal to the registry's.

- **As built (2026-10-30):**
  - `AGridPieceBase::GridPosition` is private (`VisibleInstanceOnly`, Blueprint read-only). The piece registries are
    friends and write it through `UGridPieceRegistryBase::SetPieceAt` on get-or-create, `UpdateMappings`, and
    ConnectIt's map / move / swap / shift.
  - `PieceMap` is Blueprint read-only. `GetPiece` maps only a valid piece (a null one used to be cached).
  - New: `GetAllPiecePositions`.
  - `AConnectIt_PieceBase`:
    - `OccupiedPosition` and `SetBoardPosition` removed; `InitialisePiece(Faction)` lost its position pin;
    - new `GetFactionID`;
    - `using AGridPieceBase::GetGridPosition`, plus `IGridTileHandler::GetGridPosition` returning the same position (the
      interface made the name ambiguous).
  - **Not done:** the board-vs-registry log at map time. The display board lags while events play, so that check is in
    `ReconcileAll` (Stage 3) instead.
  - The "override that sets nothing" test needs a Blueprint (`InstantiatePiece` is Blueprint-implementable), so it's an
    owner check. `ConnectIt.Board.PiecePositionCache` covers map / move / swap / shift / unmap.

## Stage 2: hooks take a copy of the event + the handle

- Piece hooks: `OnPlaced(PiecePlaced, Handle)`, `OnMoved(PieceMoved, Handle)`, `OnSwapped(PiecesSwapped, Handle)`,
  `OnCaptured(PieceCaptured, Handle)`, `OnRemoved(PieceRemoved, Handle)`, `OnClearedByScore(<its event>, Handle)` -- a
  typed copy, so Blueprint stays typed. The event carries position(s), faction, `CauseIndex` and its tag.
- Tile hooks: `OnTilePowerTriggered`, `OnMultiplierChanged`, `OnLockChanged`, `OnActiveChanged`, `OnChainPartnerChanged`
  take their event copy likewise.
- **To decide at build:** `OnPieceEnter` / `OnPieceExit` are fed by several events (placed, moved, swapped, captured,
  shifted). Either pass the polymorphic event payload the visualizer already carries, or give each source its own typed
  hook. Lean: the base event copy (tag + cause) with typed getters, since the hook mostly wants the faction and cause.
- The visualizer defaults pass `Event` through unchanged. `Cause` helpers (`GetBoardEventCause`) stay.
- Tests: `ConnectIt.Visuals.HookDefaults` updated -- each default hands its hook the event it was fired by.

- **As built (2026-10-30):**
  - The signatures are as listed, plus `OnMultiplierChanged(MultiplierChanged)`, `OnLockChanged(TileLockChanged)` and
    `OnActiveChanged(TileActiveToggled)`.
  - **Decided at build:** `OnPieceEnter` / `OnPieceExit(Event, Faction, Handle)` take the source event as an
    `FInstancedStruct`. The new `UConnectIt_BoardEventLibrary::GetBoardEventTag` reads its tag.
  - **Events made for a hook:** a hook fed by another event kind gets an event built for it (`CauseIndex` / play index
    = the source event's):
    - a starting or added piece → Piece Placed;
    - a shifted piece → Piece Moved;
    - a piece going with its tile → Piece Removed;
    - Scored / Tile Multiplier Destroyed → Multiplier Changed.
  - Test: `ConnectIt.Visuals.HookDefaults` plays real events through the queue on a test board (registries, recording
    tile and piece actors).

## Stage 3: the reconcile fallback

- On the base visualizer: `ReconcileActors(Positions)` and `ReconcileAll()`.
  - After an event's visuals finish (in `FinishBookkeeping`, once the handle is complete): reconcile the actors that
    event touched. When the event list of a board change drains: `ReconcileAll()`.
  - For each registered tile then piece (tiles first): world location equals `GridPositionToWorld(Position)` (+ piece
    offset) within a tolerance; `GridPosition` equals the registry's; a registered piece is active and has a tile.
  - **A mismatch logs an error** -- actor name, current location, expected grid position and world location, and which
    handle (if any) was held -- then **moves the actor and fixes the cache**.
- **Waiting and the timeout:** reconcile runs only when no handle is held. The existing handle safety timeout (10 s, on the
  visualizer) already completes a stuck hold and logs who never released it; reconcile **reuses it** rather than adding a
  second clock. A second, longer deadline on the whole board change is added only if the event list can stall without a
  held handle (decided at build; report it if so).
- Console variable `ConnectIt.Visualizer.Reconcile` : 0 off, 1 log only, 2 log and fix (default 2).
- Tests: a piece moved off its cell (by a test visual that forgets) is reported and corrected; a tile mid-hold is not
  checked until the handle completes; log only does not move.
- An automation / PIE check that fails a test on any reconcile error, so a bad visual fails loudly in CI.

- **As built (2026-10-30):**
  - `ReconcileActors(Positions)` / `ReconcileAll()` return the mismatch count; `ReconcileTolerance` is 1 unit.
  - **Which positions are checked after each event:** the event's own positions (found by reflection over its fields,
    arrays and nested structs), plus the snapped and arriving positions.
  - **Checks:** a tile on its cell (X / Y only; height is the tile's own); a piece's position cache; a piece with a
    tile under it (logged only); a piece resting on its tile.
  - **The whole board:** `UConnectIt_BoardStateComponent::OnBoardEventsPlayed` (new) triggers `ReconcileAll`, which
    also checks the display board against the piece registry (logged only).
  - The handle records a timeout (`HasTimedOut`, `GetTimedOutHolders`) for the error message.
  - **No second deadline:** every event completes through its handle, whose timeout already covers a stuck hold.
  - **CI:** a reconcile error fails any automation test by default, so no separate check was needed.
  - Test: `ConnectIt.Visuals.Reconcile`. The registry subsystem gained a test-only `SetUpForTest`.

## Stage 4: single-event visualizers

- **`UConnectIt_EventVisual`**: abstract Blueprintable actor, one subclass per event kind (thin, generated for each event
  struct) with `OnPlay(Event, Handle)`; each class **declares its event tag** in its class defaults. `PlayDefault()` runs
  the visualizer's own per-event function.
- **`TArray<TSubclassOf<UConnectIt_EventVisual>> EventVisuals`** on the level config (or the visualizer class); the
  visualizer builds `EventTag -> class` from the declared tags. Two classes for one tag fail validation; an unknown
  or abstract entry warns.
- **Resolution:** an override for the tag runs instead of the default hooks (and may call `PlayDefault()`); no override
  runs today's per-event function. Actors are spawned lazily on first use and destroyed with the map.
- Registry bookkeeping and the reconcile step run regardless of overrides.
- World Settings / level-config validation lists a duplicate or empty entry.
- Tests: tag lookup builds from declared tags; a duplicate tag is reported; an override runs instead of the default;
  `PlayDefault` runs the default; the handle completes the task.
- Optional later: a theme data asset bundling a set of event visuals (not in this plan).

- **As built (2026-10-30):**
  - `AConnectIt_EventVisual` is an actor, as planned; the plan's `UConnectIt_` prefix was a typo.
  - There are 22 thin classes, `AConnectIt_EventVisual_<Kind>`, all Abstract, so a Blueprint is made from them. Each
    declares its tag through a virtual `GetDeclaredEventTag` (read at runtime, not stored), has a typed
    `OnPlay(Event, Handle)` (default: `PlayDefault()`), and is in `Board/ConnectIt_EventVisual.h`.
  - The list is the level config's `EventVisuals`; the visualizer reads it in `Initialise` (`SetEventVisuals`).
  - `BuildEventVisualMap`: a duplicate tag is an error (the first kept); an empty, abstract or tagless entry is a
    warning. The level config's `IsDataValid` reports them.
  - `PlayDefault` runs the visualizer's function for the stored event: call it from `OnPlay` before the handle seals.
  - Test: `ConnectIt.Visuals.EventVisuals`.
  - **Verified 2026-10-30:** 89/89 tests, Game target clean.

## Owner checks: content migration

- Open every Blueprint that implements a piece or tile hook and re-wire it to the event copy + handle (the signatures
  change).
- Replace reads of `OccupiedPosition` in Blueprint with `Get Grid Position`.
- Your `InstantiatePiece` override can stop setting the position (the registry does it now); confirm Place Piece in the
  default visualizer with no override at all.
- Make one `EventVisual` (Place Piece) and put it on the level config; check an event with no override still plays the default.
- PIE through place, swap, shift, scoring, a power chain; break one visual on purpose (leave a piece off its tile, leave a
  hold unreleased) and confirm the error log and the correction.

## Stage 5: vault

Decision note (the registries own grid positions; the event copy + handle hook rule; the reconcile fallback; event
visuals); update the `code/` recipe "add visuals for a board event", [board-visualizer](board-visualizer.md) and the board
state flow page; log.

- **As built (2026-10-30):**
  - decision [registries own grid positions](../_decisions/2026-10-30-registries-own-grid-positions.md);
  - recipe [add visuals for a board event](../code/recipes/add-visuals-for-a-board-event.md) updated;
  - [board-visualizer](board-visualizer.md) and the board state flow page updated;
  - log `_logs/2026-10-30-1915.md`.

## Not in this plan

- Visual themes (a data asset bundling event visuals).
- Pooling tiles (the actor pool plan).
- The teaching layer's guidance visuals.

## Verification

- Each stage: build (editor closed); `Automation RunTests ConnectIt` passes.
- Owner, PIE: Place Piece works in the default visualizer with no Blueprint at all; a misplaced actor produces one error
  line naming it and is moved; an unreleased hold is logged and the game continues.
