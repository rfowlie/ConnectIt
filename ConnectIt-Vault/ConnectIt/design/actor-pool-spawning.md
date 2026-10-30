# Actor pool spawning: async, pre-warming and a per-tick budget

Design note (2026-10-29), from the owner's question: does `UActorPoolSubsystem` spawn asynchronously, could that be an
option, and if not, can we clamp spawns per tick (frame rate later on)? **Direction settled with the owner (below); nothing built.** Plugin: `UnrealGameMechanics` (`Pooling/`).

## What it does today (checked in code)

- **Everything is synchronous.** `UActorPool::CreatePooledActor` calls `UWorld::SpawnActor` on the game thread, at three
  moments: **`UActorPool::Create`** (spawns `InitialPoolSize` actors in a loop, all in one frame), **`GetObjects`**
  (pops from the inactive list, and **spawns the shortfall** in the same call), and **`CreatePool`** (the same as
  `Create`).
- **No pre-warming happens in practice.** `GetObjects` creates a pool on first use with the default
  `InitialPoolSize = 1`, and nothing in the project calls `CreatePool`. (*Correction, 2026-10-29:* the level config
  already has **`PiecePoolInitialSize` = 32** and a Blueprint-callable `GetPiecePoolInitialSize`, but nothing in C++ or
  the Blueprints I could search uses it -- an intended warm-up that was never wired.) So a pool grows **one synchronous spawn at a
  time, at the moment an actor is first needed** -- the first N piece placements each pay a full spawn.
- **The only user so far is the grid piece registry** (`RetrievePiece` asks for one piece). Tiles are spawned directly
  (not pooled), so a layout change spawns each added tile in one frame. The action cursors plan will use the pool too.
- There is no stat or log for "the pool had to spawn" (a pool miss).

## Can it be asynchronous? (Claude)

**Not truly.** In Unreal, `SpawnActor` must run on the **game thread** (it creates a UObject, runs the construction
script, registers components, adds to the level, and runs `BeginPlay`). There is no worker-thread actor spawn. What
people call "async spawning" is one of three things:

1. **Async *loading* of the class and its assets** (`FStreamableManager::RequestAsyncLoad` on a soft class): the first
   spawn of a Blueprint with heavy meshes / materials is often slower from *loading* than from spawning. This one is
   genuinely asynchronous.
2. **Time-slicing the spawns** -- a queue drained a few per frame under a budget. Not async, but it spreads the cost
   so no frame spikes. This is the "clamp per tick" the owner suggests.
3. **Pre-warming** -- spawn the pool's actors *before* they are needed (behind a loading screen, or spread over the
   first frames). Moves every spawn cost out of gameplay.

## Proposal (Claude)

Keep the synchronous path (it must stay: some callers need the actor *now*) and add a **budgeted mode** beside it, plus
pre-warming. Spawn mode as an option, per call:

- **`GetObjects(...)` -- unchanged, synchronous.** Returns the amount asked for now, spawning any shortfall. New: it
  logs a **pool miss** (class, how many it had to spawn) so initial sizes can be tuned, and bumps a stat.
- **`RequestObjects(Class, Amount, OnReady)` -- budgeted.** Queues the request; the subsystem (made tickable) fulfils it
  over frames and fires `OnReady(Actors)` when the whole amount is available. Callers that can wait use this.
- **`PrewarmPool(Class, Count)` -- budgeted pre-warm.** Spawns inactive actors in the background, firing a
  `OnPoolReady` when the pool reaches `Count` -- so a loading screen / the board setup can wait for it, or it fills in
  during play.
- **Optional `SoftClass` variants** of both: async-load the class first (`RequestAsyncLoad`), then spawn under the
  budget -- so neither the load nor the spawn hitches.
- **The budget** (developer settings, readable in the editor): **`MaxSpawnMillisecondsPerTick`** (default around 1-2 ms;
  a time budget copes with actors of very different cost) and an optional **`MaxSpawnsPerTick`** cap. **At least one
  spawn per tick** is always allowed, so a request can never starve. Requests are served first-in-first-out, with an
  optional priority (visible things first).
- **Sizing (the part that avoids most hitches):** pools are pre-warmed from known counts at level start -- pieces ≈ the
  tile count, tiles for the next layout, a few cursors per action -- so gameplay rarely spawns at all. Sizes come from the
  level config / the tile and piece registries, not hard-coded.
- **Also pool tiles** (the phased-levels note said it is easy to add): a layout change then takes tiles from the pool
  instead of spawning a burst in one frame.

### How it fits the rest

- **With the board visualizer's completion handle** ([board-visualizer-plan](board-visualizer-plan.md)): a visualizer
  function that needs actors can `Hold()` the event, call `RequestObjects`, and `Release()` in `OnReady` -- the event
  queue waits for the spawns, so a budget never shows up as a missing piece. If the pool is already warm, `OnReady`
  fires at once.
- **With the action cursors plan:** cursors come from the pool synchronously (a pre-warmed handful), so no change.
- The server and clients each run their own pools; nothing replicates through the pool.

### Costs / caveats

- A budgeted request means "not ready yet" must be handled by the caller (hold the event, show a placeholder, or wait).
- The time budget measures only the spawn call; a Blueprint `BeginPlay` that does heavy work still lands on that frame
  (so keep pooled actors' construction / `BeginPlay` light and put per-use work in `ActivatePoolObject`).
- `ActorPool` itself is a plain `UObject` created with `NewObject<UActorPool>()` and no outer; a tickable queue lives on
  the subsystem, not the pool.

## Decided (owner, 2026-10-29): a handle-based async request beside the synchronous calls

- **A call that returns a handle.** The caller binds the handle's **`OnReady`**; when it fires, **`GetActors()`** is safe
  and returns the requested number of actors.
- **How the subsystem serves it:** it **grabs as many pooled actors as the request needs**. If the pool had enough, it
  **waits one tick, then fires `OnReady`**. If not, it **spawns the missing actors one per tick** until it has enough,
  then fires `OnReady`.
- **Spawn clamp: one actor per tick outside initial level loading,** with an **option to change the number for
  testing.**
- **Why the delay is fine:** unless a not-yet-pooled class is requested in the hundreds, it's negligible, and it fits
  the already-async game event system -- the event is held up anyway.
- **The synchronous calls stay** for code that is synchronous by nature.
- **Pre-warming is still the main defence** (warm pools for every expected situation); the async path is the safety net
  for when that falters.

## How it works, with the edge cases (Claude)

**The handle** -- `UActorPoolRequest` (a `UObject`, Blueprint-friendly):
- `OnReady` (a dynamic multicast delegate), `IsReady()`, `GetActors()`, `GetState()` (Pending / Ready / Cancelled /
  Failed), `Cancel()`, and `ReleaseAll()` (a convenience: return every actor of the request to the pool).
- **`OnReady` never fires inside the request call** -- not even when the pool already has the actors -- so the caller
  always has time to bind first (the same reason the pool leaves activation to the caller). `IsReady()` covers a late
  binder.
- Actors come back **inactive**; the caller activates them, and releases them with `ReleaseObject`, exactly as with the
  synchronous path.

**The subsystem:**
- **Reserves at request time.** The pooled actors the request needs are taken out of the inactive list *immediately*, so a
  later synchronous `GetObjects` or another request can't take them while this one waits for its spawns.
- **Ticks only while requests are pending** (a tickable world subsystem whose `IsTickable` is false when idle), so
  there's no cost when the pool is warm.
- **The clamp is global, not per request:** `MaxAsyncSpawnsPerTick` (default **1**, adjustable -- developer setting /
  console variable -- for testing) is shared by all pending requests, served first-in-first-out. Each tick: any request
  that already has enough counts down its one-tick wait and fires; the remaining budget spawns for the oldest incomplete
  request.
- **The clamp applies to the asynchronous path only.** The synchronous calls (`GetObjects`, `CreatePool`) stay
  **unclamped**, so **initial level loading -- warming pools with `CreatePool` -- needs no "loading mode" flag**: it
  simply uses the synchronous path.

**Lifetime and failure:**
- The subsystem holds pending requests (strongly) until they are ready or cancelled, so a caller that drops its
  reference doesn't lose the actors mid-way.
- **`Cancel()` returns everything the request holds** (reserved and spawned) to the inactive pool, undeactivated.
  `Deinitialize` cancels all pending requests. A caller destroyed before `OnReady` (an action deactivated, a level
  change) should cancel -- the plugin's actions would do it for you in their cleanup.
- A failed spawn (null class, world gone) ends the request in **Failed** with an **`OnFailed`** delegate; `GetActors()`
  returns what it had.

**Convenience on top:**
- **`PrewarmPool(Class, Count)`** = a request whose actors are released the moment it's ready: it fills a pool in the
  background, one spawn per tick (e.g. warming the next layout's tiles mid-game).
- A **Blueprint async node** ("Request Pooled Actors", exec pins for Ready / Failed) so visualizer Blueprints don't need
  to bind a delegate by hand -- optional, later.
- **With the board visualizer's `Hold()` / `Release()`:** hold the event, request, release in `OnReady`.
- **Pool-miss logging** on the synchronous `GetObjects` stays in the proposal, to tune pre-warm sizes.

## Settled (owner, 2026-10-29)

Clamp scope agreed; and **level-start warming, pooling tiles now, and the Blueprint async node are all in the first
version.** Plan: [actor-pool-plan](actor-pool-plan.md) (**approved 2026-10-29**, not built).

## Open (what's left -- resolved above)

1. **Confirm the clamp scope:** one global spawn per tick shared by all pending async requests, applying to the async
   path only (the synchronous calls stay unclamped, and level-load warming uses them).
2. Pre-warm at level start from the registries' known counts (synchronously, with `CreatePool`) in the first version?
3. Pool tiles now, or only when layout changes prove a hitch?
4. The Blueprint async node: with the first version, or later?

## Earlier proposal and options (kept for the record)


1. Add the budgeted path (`RequestObjects`, `PrewarmPool`) -- yes? And is the async class load worth including in the
   first version, or later?
2. The default budget (ms per tick, and whether to keep a count cap).
3. Pre-warm at level start from the registries' known counts -- include in the first version?
4. Pool tiles now, or only when layout changes prove a hitch?
