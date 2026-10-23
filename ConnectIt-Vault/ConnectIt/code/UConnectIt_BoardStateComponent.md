---
schema: code
kind: UCLASS
role: primary
source:
  - Source/ConnectIt/Public/Board/ConnectIt_BoardStateComponent.h
  - Source/ConnectIt/Private/Board/ConnectIt_BoardStateComponent.cpp
  - Source/ConnectIt/Public/ConnectIt_Structs.h
reconciled: 2026-10-23
commit: 4c9625c
---

# UConnectIt_BoardStateComponent

`UBoardStateComponentBase` subclass on
[[AConnectIt_GameState|AConnectIt_GameState]]. **The board's single source of
truth.** Exactly one replicated property — `FConnectItBoardStateSnapshot BoardSnapshot`
(previous + current `FConnectItBoardState` + the `FConnectItBoardChangeEvent` describing
the delta) — so state and "what changed" arrive atomically, and one signal drives every
visual system on both server and client.

## When you touch this

- Any authoritative board mutation (server).
- Reading current / previous board state, or the last change event.

## Entry points

- **Server:**
  - `InitialiseBoardState(TileRegistry, PieceRegistry, NumFactions, InitialMultiplier, InTargetScore, Rules)` — once at
    match start.
    - Each registered tile's data comes from **`MakeTileData`** (static, public): its `IConnectIt_TileDataProvider`
      values (a builder tile's included) or the defaults. Its starting multiplier is clamped to its tile type's limits; a
      Turn Start power's ready turn is its cooldown; a power tag the tile catalog doesn't define is dropped with a
      warning. A starting piece for a faction the board doesn't have is dropped with a warning.
    - Each tile joins with `AddTile` (its id); chain partners are linked once every tile has one (a partner that isn't
      a tile: warned, no partner).
    - Counts `StartingPieces`, then plays a `Board Seeded` event so visuals can create starting pieces.
    - **With the rules, the seeded board is settled once** (`ResolveBoardChange`): its Sync powers set up what they
      keep true (chained tiles lock), and their events follow Board Seeded. Previous = the board as built, Current =
      settled.
    - Then the tile registry swaps the **builder tiles** for the catalog's actors (`SwapBuilderTiles`), before
      anything plays on them.
  - `SetBoardState(NewState, ChangeEvent)` — the only writer: current → previous, new state and event list stored in
    the one snapshot; `OnBoardStateChanged` on the server; `EnqueueBoardEventTags()`; then **`OnBoardChangeCommitted`**
    (native, server only) and the **`ConnectIt.Event.BoardChangeComplete`** marker queued behind the events (it
    completes once they've all played out: the GameMode's one-board-change-at-a-time lock).
- **Reads:** `GetCurrentState()`, `GetPreviousState()`, `GetChangeEvent()` (the latest change's list -- not necessarily
  the one playing), `GetBoardSnapshot()`, `GetInfo()` → `FConnectItBoardStateInfo`; helpers `GetFactionPieceCount`,
  `GetFactionPiecePositions`, `GetPositionMultiplier`, `GetTileMultiplier`.
- `OnBoardStateChanged` (inherited, **zero-param**) — bind, then read `GetCurrentState()` / `GetChangeEvent()`.
- **Display board** (every machine, for visuals): `GetDisplayBoard()` -- the board as the player has seen it so far.
  It steps along one board event at a time, each applied (`FConnectItBoardEvent::ApplyTo`) when its queue entry
  completes, so a handler playing an event sees the board before it, plus the event. `OnDisplayBoardChanged` fires
  each time it moves. When the queue drains it copies Current (match-level fields no event carries), warning if the
  tiles or scores differ. Game logic reads Current, never this.

## Collaborators

- `OnRep_BoardSnapshot` (client) and `SetBoardState` (server) **both** call `BroadcastChange()` then
  `EnqueueBoardEventTags()` — symmetric. A client's first snapshot also starts its display board (from that
  snapshot's Previous) and swaps its own builder tiles (tile actors are local).
- The display board listens to `UGameEventTaskSubsystem::OnAnyTagComplete` (`HandleQueueEntryComplete`); `EndPlay`
  unbinds it.
- `EnqueueBoardEventTags()` queues **one entry per event** in the change list, in order, on
  [[UnrealGameMechanics/code/UGameEventTaskSubsystem|UGameEventTaskSubsystem]]
  (`QueueTagContainerWithPayload`: the event's own tag, the event itself as payload). A listener reads the event that
  is playing through `UConnectIt_BoardEventLibrary` (`GetActive...Event`), even if newer snapshots have arrived.
- Written by [[UConnectIt_BoardRequestMediator|UConnectIt_BoardRequestMediator]] (`DispatchRequest`) via
  `SetBoardState`; the GameMode binds `OnBoardStateChanged` (directors, Level triggers) and `OnBoardChangeCommitted`
  (the lock).

## Gotchas

- **Only `SetBoardState` may mutate** — never write `CurrentState` directly.
- `Game Won` (event) is **edge-triggered**; `FConnectItBoardState::bGameOver` stays true afterward.
- `EnqueueBoardEventTags` runs on both machines — visual sequencing must be client-safe / idempotent.
- Scoring and the win stamp happen *before* `SetBoardState` (the mediator works on a copy, then commits) — the list
  already holds `Scored` / `Game Won`.
- The completion marker is server-only and not a board event; with no visual tasks registered it completes inside
  `SetBoardState` (the GameMode copes -- see [[place-piece-request]]).
- The seed doesn't fire `OnBoardChangeCommitted` (no lock at match start).

## Cross-impact

A new kind of change is a new board event struct (`ConnectIt_BoardEvents.h`) with its tag and a
`UConnectIt_BoardEventLibrary` getter -- nothing here changes ([[add-a-board-request-type]]). Anything reading the
snapshot (`AConnectIt_GameState` wrappers, debug widgets, `UConnectIt_BoardStateLibrary`) reads the event list, not
per-kind fields.

## Changes

- 2026-10-23 — tile powers and the tile lock: `MakeTileData` (catalog-checked tag, clamped multiplier, Turn Start
  ready turn); tiles join with ids and chain partners; the seeded board is settled once under the rules; builder tiles
  swapped after the seed (and on a client's first snapshot); the **display board** (`GetDisplayBoard`,
  `OnDisplayBoardChanged`). See [tile powers in the after-move chain](../_decisions/2026-10-22-tile-powers-in-the-after-move-chain.md),
  [tile catalog and builder tiles](../_decisions/2026-10-22-tile-catalog-and-builder-tiles.md),
  [chained tiles](../_decisions/2026-10-23-chained-tiles-as-a-sync-power.md). (`process-code` sweep — commit
  `4c9625c`.)
- 2026-10-22 — the change event is an ordered list of event structs, each queued as its own entry with itself as
  payload (the per-flag tag branches are gone); `OnBoardChangeCommitted` + the `BoardChangeComplete` marker (one board
  change at a time); board setup uses `FConnectItTileInitialData::ToTileData`.
- 2026-09-21 — designer tile data (`IConnectIt_TileDataProvider`) and starting pieces read at
  `InitialiseBoardState`; `bBoardSeeded` event + `ConnectIt_Event_BoardSeeded` tag added. See
  [designer tile data decision](../_decisions/2026-09-20-designer-tile-data-via-blueprint-interface.md).
- 2026-09-14 — corrected the Collaborators event-tag list: it previously described a
  three-tag "shift/piece-placed → line-scored → player-win" fixed order that no longer
  matches the source — six independent concrete-change tags now exist (including
  `bPiecesSwapped`/`bPieceCaptured` from the SWAP/capture work), each firing from its own
  disjoint `ChangeEvent` flag.
- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[ConnectIt/CLAUDE|ConnectIt overview]].
- [[board-state-single-source-of-truth|systems/board-state-single-source-of-truth]] ·
  [[ConnectItStructs|ConnectItStructs]]
