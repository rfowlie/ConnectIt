---
type: plan
status: not-started
approved: true
approved_on: 2026-10-29
completed_on:
started:
stages: 6
current_stage: not started
playtested: false
design: "[actor-pool-spawning](actor-pool-spawning.md)"
question:
decision:
requires: "[board-visualizer-plan](board-visualizer-plan.md)"
---

# Plan: the actor pool -- async requests, a spawn clamp, tile pooling and pre-warming

Design: [actor-pool-spawning](actor-pool-spawning.md) (decided with the owner, 2026-10-29). Written 2026-10-29 in a
design-only session; **approved by the owner 2026-10-29**. Not built -- building happens in its own session.

> **Prerequisite:** the [board-visualizer-plan](board-visualizer-plan.md) -- **only Stage 4 here** (pooling tiles)
> changes the visualizer's tile bookkeeping, so it waits for that plan's visualizer stages. **Stages 1-3 are plugin-only
> and independent.**

## Context

Settled with the owner:

- **Today the pool is fully synchronous** (`SpawnActor` in `UActorPool::Create`, `CreatePool` and the shortfall of
  `GetObjects`) and **never pre-warmed**: pools start at size 1 (the level config's `PiecePoolInitialSize = 32` is read by
  `GetPiecePoolInitialSize` but nothing uses it), and tiles aren't pooled at all.
- **An async path beside the synchronous one.** A call returns a **handle**; the caller binds its **`OnReady`**; when it
  fires, **`GetActors()`** holds the requested number of actors. The subsystem takes as many pooled actors as it needs;
  with enough it fires `OnReady` **one tick later** (never inside the call); otherwise it **spawns the missing ones one
  per tick** until it has them, then fires.
- **The clamp: one spawn per tick, shared by all pending requests, async path only** (adjustable for testing). The
  synchronous calls and `CreatePool` stay unclamped -- level-load warming uses them, so no "loading mode".
- **Pre-warming is the main defence;** the async path is the safety net. It fits the event queue: a held event can wait
  for its actors.
- **Also in the first version (owner):** warming pools at level start from known counts, pooling tiles, and a Blueprint
  async node.

Each stage builds and passes the tests before the next; the editor must be closed for builds; the owner commits between
stages.

## Stage 1: pool foundations (plugin)

- **Pool-miss tracking:** `UActorPool` counts the actors a synchronous `GetObjects` had to **spawn** (beyond its inactive
  list); the subsystem logs a **warning once per class** ("pool for X grew by N -- pre-warm it") and exposes
  `GetPoolMissCount(Class)`. (A console variable `Pool.LogMisses`, default on.)
- **Reserve and spawn-one primitives on `UActorPool`:** `TakeInactive(Amount) → Actors` (reserve up to Amount from the
  inactive list, no spawning) and `SpawnOne() → Actor*` (one inactive actor spawned and counted as taken). `GetObjects`
  is rewritten on top of them (behaviour unchanged).
- **Tunables as console variables** (no new module): `Pool.MaxAsyncSpawnsPerTick` (default **1**, for testing) and
  `Pool.LogMisses`.
- Tests: a synchronous get beyond the pool counts and logs a miss once per class; `TakeInactive` never spawns; the
  rewritten `GetObjects` returns the same as before (including `Amount` below 1).

## Stage 2: the async request (plugin)

- **`UActorPoolRequest`** (`UObject`, Blueprint-friendly): `OnReady` and `OnFailed` (dynamic multicast delegates),
  `IsReady()`, `GetState()` (Pending / Ready / Cancelled / Failed), `GetActors()`, `Cancel()`, `ReleaseAll()` (returns
  every actor of the request to the pool).
- **`UActorPoolSubsystem::RequestActors(Class, Amount) → UActorPoolRequest*`**, and
  **`PrewarmPool(Class, Count) → UActorPoolRequest*`** (a request whose actors are released the moment it's ready).
- The subsystem becomes a **tickable world subsystem that ticks only while requests are pending** (`IsTickable` false
  when idle).
- **At request time** it **reserves** (`TakeInactive`) the pooled actors the request needs, so nothing else can take
  them. **Each tick:** requests already holding enough count down their one-tick wait and fire `OnReady` (so it **never
  fires inside the call**); then up to `Pool.MaxAsyncSpawnsPerTick` actors are spawned for the **oldest incomplete
  request**, first-in-first-out, **shared by all requests**.
- **Actors come back inactive** (the caller activates and releases them, as with the synchronous path).
- **Lifetime:** the subsystem holds pending requests strongly until ready or cancelled. `Cancel()` returns everything the
  request holds (reserved and spawned) to the inactive pool without deactivating; `Deinitialize` cancels all pending
  requests. A failed spawn (null class, world gone) ends the request **Failed** with `OnFailed` (`GetActors()` returns
  what it had). Cancelling a ready request or a repeat is a no-op with a log.
- Tests (a test world): a request with enough pooled actors fires `OnReady` on the next tick, not inside the call;
  a shortfall spawns exactly one per tick and fires when complete; two requests share the clamp, oldest first;
  `Pool.MaxAsyncSpawnsPerTick` raises it; reserved actors can't be taken by a synchronous `GetObjects` meanwhile;
  `Cancel` returns reserved and spawned actors; `Deinitialize` cancels; a null class fails.

## Stage 3: the Blueprint async node (plugin)

- **`UActorPoolAsyncAction`** (`UBlueprintAsyncActionBase`): **Request Pooled Actors (Class, Amount)** with **Ready
  (Actors)** and **Failed** exec pins, and a **Prewarm** twin, so visualizer Blueprints never bind a delegate by hand.
  Keeps the request alive for the node's life; cancels it if the node's owner is destroyed first.
- Tests: none without a world; checked in the editor.

## Stage 4: pool the tiles (ConnectIt)

*Waits for the board visualizer's visualizer stages.*

- **`AConnectIt_TileBase` implements the pool's activate / deactivate** (`IActorPoolInterface`): deactivating hides,
  disables collision and tick, unregisters from hover, and **resets the tile to a blank state** (cursor state → neutral,
  lock / chain / power / multiplier visuals back to default); activating shows it. A Blueprint can add to both.
- **`UConnectIt_TileRegistry` takes tiles from the pool** instead of `SpawnActor`: `SpawnLayoutTiles` (synchronous,
  `GetObjects`) and `SwapBuilderTiles` (level start, `CreatePool` / `GetObjects`); each applies the cell's data as it does
  now, then activates.
- **Async twin:** `SpawnLayoutTilesAsync(Cells) → handle` (a request wrapped to apply the cells on ready), for the
  visualizer's Tiles Added handler, which **holds** the event, requests, and **releases** in `OnReady`.
- **Tiles that leave** (`UnregisterTilesAt` → the visualizer's bookkeeping) are **deactivated and released to the pool**
  instead of `Destroy()` (the one `Tile->Destroy()` site in `AConnectIt_BoardVisualizer::FinishBookkeeping`).
- Pools are **per tile class** (one per catalog entry class).
- Tests: a released tile comes back blank and reusable; a layout change twice in a row reuses the same actors (pool
  counts don't grow); `SwapBuilderTiles` still produces the same board.

## Stage 5: warm the pools at level start (ConnectIt)

- **Pieces:** the piece registry creates its pool with the level config's existing **`PiecePoolInitialSize`**
  (synchronously, in `InitialiseRegistry`, i.e. during level load -- unclamped).
- **Tiles:** the tile registry warms one pool per catalog class from the placed builders' counts, plus a new
  **`TilePoolExtra`** on the level config (a hand-set margin for phased layouts, default 0).
- **`PrewarmLayout(Layout)`** on the tile registry: counts a layout's tiles per class, subtracts what's pooled, and
  calls `PrewarmPool` for the rest -- the Level director calls it a phase early so a coming `Change Layout` finds its
  tiles waiting.
- **Pool misses become the tuning tool:** with misses logged, a PIE run shows which pools need a bigger start.
- Tests: warming creates the expected pool sizes from a small level; `PrewarmLayout` requests only the shortfall.

## Stage 6: vault

Decision note (a handle-based async request beside the synchronous calls; one spawn per tick on the async path; tiles
pooled); `UnrealGameMechanics` code pages (the pool, the request, the async node); a recipe "request pooled actors
without a hitch" and an update to the add-visuals recipe; the tile pooling in the tile registry page; log.

## Not in this plan

- **A millisecond spawn budget** -- decided against (owner): one actor per tick is the clamp.
- **Request priorities** between requests (FIFO for now) -- wishlist, later.
- **Warming cursor pools** from the loadouts -- not worth it (the action cursors acquire a couple synchronously on first
  use).
- **Async-loading classes** (`RequestAsyncLoad` on soft classes) -- wishlist, later; a step if loading, not spawning,
  shows up as the hitch.
- **Pooling the editor tools' previews and builder tiles** -- not needed (owner: a hitch when previewing is fine; revisit
  only if it is noticeably bad).

## Verification

- Each stage: build (editor closed); `Automation RunTests ConnectIt` and the plugin's tests pass.
- Owner, PIE: no pool-miss warnings on a normal level after warming (or, at least, the misses name the pools to enlarge);
  a `Change Layout` that adds many tiles moves smoothly (frame time steady) and the log shows tiles taken from the pool;
  with `Pool.MaxAsyncSpawnsPerTick 5` in the console, async requests fill faster.

## Settled before approval (owner, 2026-10-29: as written; exclusions above)

1. `Pool.MaxAsyncSpawnsPerTick` and `Pool.LogMisses` as console variables (not a settings class).
2. Tile pooling waits for the visualizer's stages (Stage 4); Stages 1-3 and 5's piece warming don't.
3. The tile registry's async twin returns a handle rather than a Blueprint-only node.
