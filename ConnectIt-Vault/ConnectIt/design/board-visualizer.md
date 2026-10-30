# Board visualizer: registries, events and the artist entry point

Design note (2026-10-29), from the owner's notes on registries and visualizers. **Decided the same day (all three leans, owner); built
([board-visualizer-plan](board-visualizer-plan.md)); Round 3 built 2026-10-30
([board-visualizer-revision-plan](board-visualizer-revision-plan.md),
[registries own grid positions](../_decisions/2026-10-30-registries-own-grid-positions.md)).** Related: [hover-component](hover-component.md) (`RefreshFromBoard`),
[board-events](board-events.md), [editor-notes-2026-10-28](editor-notes-2026-10-28.md) (event cause helpers).

## Owner's notes (2026-10-29)

**Registry typing.** The tile and piece registries return the plugin base class (`AGridTileBase`), so ConnectIt code
casts to the ConnectIt tile class. Open: ConnectIt subclasses of the registries, or call functions like
`OnTilePowerTriggered()` through interfaces?

**The current visualizer** tries to cover every case but ends up rigid: it only calls the C++-only `On...` functions on
tiles and pieces. **Visualizers should be the entry point for artists and designers** to add visual interest.

**Directions:**

- **Event syncing in C++** -- the visualizer binds to all tags in a tag container, or binds by which interfaces it
  implements, so Blueprint always gets the right struct per event.
- **One visualizer per map** (several per level gets messy), set where the registries are set (the data asset); all its
  functions Blueprint-implementable, **one per board operation, each with its struct**.
- **Visual effects:** the visualizer will create and destroy other effects.
- **Reaction tracking:** `MakeReaction()` with its counter (when to stop / go) is disliked; tracking should follow the
  overall event, as the game event system does, so nothing is erased by accident. *Concession:* the game event system
  already gates events, so today's approach is probably fine.
- **Dispatch:** e.g. `HandlePlacePiece()` -- `Dispatch()` still checks each payload, then passes it to a Blueprint
  event. The visualizer will often call the piece's own `On...` functions so the actor does its own visuals; there must
  also be a way to add **extra or alternate** visuals for any board event.

**Open decisions:** (1) registry subclasses vs interfaces; (2) bind by tag container vs by interface vs one visualizer
per level binding to all tags.

## What exists (Claude, checked in code)

- **The registries already have ConnectIt subclasses:** `UConnectIt_TileRegistry` (`: UGridTileRegistryBase`) and
  `UConnectIt_PieceRegistry` (`: UGridPieceRegistryBase`) -- but their lookups still return `AGridTileBase*` /
  `AGridPieceBase*`.
- **`UConnectIt_TileVisualizer`** -- a plain `UObject` the board registry subsystem creates in C++ (not a class a
  designer picks). It registers one gated task per handled event tag (`GetHandledEventTags`), and when one plays,
  `Dispatch` checks the payload's type and calls the matching **`AConnectIt_TileBase`** hook (`OnTilePowerTriggered`,
  `OnPieceEnter`, `OnPieceExit`, `OnMultiplierChanged`, `OnLockChanged`, `OnActiveChanged`, `OnChainPartnerChanged`) on
  each tile concerned, handing each a fresh `UConnectIt_TileReaction`; the event waits until every reaction finishes
  (`PendingReactions`). Only **one** Blueprint event exists on it (`HandlePlacePiece`).
- **Pieces** are handled separately (the `CI_PieceVisualHandler` Blueprint and the piece registry).

## Recommendations (Claude)

### 1. Registries: typed getters on the subclasses we already have

The ConnectIt registries exist, so add **typed lookups** there -- `GetTileAt(Position) → AConnectIt_TileBase*`,
`GetPieceAt(Position) → AConnectIt_PieceBase*` (Blueprint-pure, the cast done once, inside). Every ConnectIt tile
derives from `AConnectIt_TileBase`, so callers never cast. **Interfaces** stay for **plugin-level contracts** the grid
plugin itself needs to call without knowing the game (`RefreshFromBoard`, hover) -- not for ConnectIt-to-ConnectIt
calls, where a typed class is simpler and safer.

### 2. One visualizer per map, a Blueprint class, binding every board event in C++

- **`AConnectIt_BoardVisualizer`** -- an **actor** (not a bare UObject): Blueprints can use timelines, delays, spawn
  and destroy Niagara / meshes / sounds, and it is cleaned up with the map. **One per map**, its **class set on the
  level config** (next to the registries) and spawned by the board registry subsystem; artists make Blueprint
  subclasses per look.
- **C++ binds every board event, not a hand-kept list:** the base enumerates every `FConnectItBoardEvent` struct by
  reflection (as operations already do for request types) and registers a gated task for each event's tag. A new event
  type is picked up automatically.
- **One Blueprint-native function per event, with its struct** -- `OnPiecePlaced(const FConnectItBoardEvent_PiecePlaced&,
  Handle)`, `OnPieceMoved(...)`, `OnTilePowerTriggered(...)`, `OnScored(...)`... The C++ base does the payload check
  and calls the typed function, so Blueprint always gets the right struct.
- **The default implementation calls the actors' own hooks** (the tile's `OnTilePowerTriggered`, the piece's
  `OnMoved`...) -- so actors keep their personal visuals. A designer **overrides** a function to add extra visuals
  (call the parent, then add) or to **replace** them (don't call the parent). That is the "extra or alternate visuals for
  any board event" requirement, with no `Dispatch` to edit.
- This makes binding "by interface" unnecessary: per-event native functions give the same typed entry points with
  one class to subclass.
- **Pieces fold in:** the same visualizer handles piece events (moving, spawning, removing pieces through the piece
  registry), replacing the separate piece visual handler -- one entry point.

### 3. Completion: one handle per event, held and released explicitly

Keep the event queue's gate (it already waits for visuals), but replace the implicit per-tile reaction counting with
**one completion handle per playing event**: whoever starts a visual that should be waited on calls **`Hold()`** and
later **`Release()`**; the event completes when every hold is released (or at once if nobody held it). Actor hooks get
the same handle. Explicit holds mean a visual can't be forgotten or "erased" by a counter, and extra visuals added by a
designer can hold the event too. (A safety timeout logs and releases a hold that is never released.)

### 4. Related

- `RefreshFromBoard` (hover plan) covers non-timed state; the visualizer covers timed visuals.
- The event cause helpers (editor notes) let a visualizer function ask "was this move caused by a power?".

## Decided (owner, 2026-10-29)

Plan: [board-visualizer-plan](board-visualizer-plan.md) (**approved 2026-10-29**, not built).


All three recommendations above, as written: typed getters on the existing registry subclasses (interfaces only for
plugin-level contracts); one visualizer actor per map, set on the level config, binding every board event in C++ with a
Blueprint-native function per event whose default calls the actors' hooks; completion handles (hold / release).

## Open (resolved)

1. Registry subclasses (typed getters) vs interfaces -- lean: typed getters on the existing subclasses; interfaces
   only for plugin-level contracts.
2. How the visualizer binds -- lean: one visualizer actor per map, C++ binds every board event by reflection, a
   Blueprint-native function per event with its struct, defaults calling the actors' hooks.
3. Completion handles (hold / release) instead of counted reactions -- or keep today's reactions.


## Round 2 (owner's notes, 2026-10-30): too monolithic, and Place Piece

**Owner:**

- The new setup is **one visualizer per level** responsible for every visualization of every board reaction and
  operation. That feels **monolithic and daunting**.
- Easier down the line: **single visualizers** -- a **base visualizer** (the C++ default) plus a **list of single
  visualizers that each override one board operation**, so behaviours swap more easily. A **map of operation tag →
  visualizer actor (single)**.
- **Piece `OnPlaced(Handle)` should probably pass the position.** How do pieces and tiles get information about the
  event if it isn't passed through?
- **Place Piece failed** in the default visualizer: the **piece registry isn't auto-creating pieces**, so when
  responding to place piece **there is never a piece at the placed position** (it returns null every time). In the
  owner's bespoke visualizer, place piece: got a piece from the registry, set its location, called *activate pool
  object*, ended the event hold. "Place piece" implies no piece is on the tile yet, so *instantiate piece* has to run on
  the registry (which sets the faction) -- why the default is confusing for Place Piece, while Swap works because its
  pieces already exist.

### What the code does today for Place Piece (checked 2026-10-30, Claude)

In the committed visualizer (`dfcaaac`, 2026-10-29, "Board Visualizer Update") this is already handled in C++:

1. `Dispatch` first runs the **bookkeeping** (`BeginBookkeeping`), which for a Piece Placed event **spawns the piece**
   (`SpawnPieceAt(Position, Faction)`): it takes one from the pool (`RetrievePiece`), calls `InitialisePiece(Faction,
   Position)`, positions it, **maps it in the registry** (`MapPiece`) and **activates** it (`ActivatePieceAt`).
2. **Then** the Blueprint-native `OnPiecePlaced(Event, Handle)` runs, so `GetPieceAt(Event.Position)` should find the
   piece; the default calls the tile's `OnPieceEnter` and the piece's `OnPlaced(Handle)`. All of this happens in one call,
   so activating the piece before the hook runs cannot show a one-frame pop.
3. Bookkeeping runs even if a Blueprint override doesn't call the parent.

So the description above (the registry never creates the piece; `GetPieceAt` returns null) matches **an earlier state**,
or a case where the spawn fails. `SpawnPieceAt` returns **null with a log** when: the **level config has no Piece Actor
Class** ("no Piece Actor Class on the level config"); `RetrievePiece` fails (**no hover subsystem or no actor pool
subsystem in the world**, or the pool couldn't spawn); or no tile is found at that position ("a piece at (x, y) has no
tile -- not activated": it is mapped but not activated). *To confirm with the owner: the Output Log lines when it
returns null, and that the build includes `dfcaaac`.*

### How hooks get their information (Claude)

Today it is inconsistent: tile hooks get the event's data **as parameters** (`OnPieceEnter(Faction, Cause, Handle)`;
`OnMoved(From, To, Handle)`), but the piece's **`OnPlaced(Handle)`** (and `OnRemoved(Handle)`) get none -- they rely on the
position and faction that bookkeeping stored on the piece (`InitialisePiece`). That is why it looks like "nothing is
passed through".

**Proposed rule:** *a hook receives everything the event says about it as parameters*, and an actor's own stored state is
not the source of truth during a visual (it can differ from the display board mid-chain). Concretely: `OnPlaced(Position,
Faction, Cause, Handle)`, `OnCaptured(Position, NewFaction, Cause, Handle)`, `OnRemoved(Position, Faction, Cause,
Handle)`... -- the same shape as the tile hooks, with the **Cause** tag always present.

### Single-event visualizers (Claude's take)

**Agreed: it stops the visualizer becoming one giant Blueprint, and makes a behaviour swappable.** The built design
already has one function per event; this moves each into its own small class, chosen by data.

- **`UConnectIt_EventVisual`** -- an abstract Blueprintable **actor** (timelines, spawning effects and delays are all
  actor features) that handles **one event kind**. It receives the typed event, the completion `Handle`, and the owning
  visualizer for helpers (`GetTileAt`, `GetPieceAt`, the display board, the registries). Spawned lazily, only for events
  that have an override, and cleaned up with the map.
- **The list lives on the base visualizer / level config:** `TArray<TSubclassOf<UConnectIt_EventVisual>> EventVisuals`.
  **Each class declares which event it handles** (its class defaults), and the visualizer builds `EventTag → class` from
  them -- so the "map key" can never disagree with its value (the same trick as the tile catalog, where the effect owns
  its tag). A typed class per event (generated thin subclasses -- a `PiecePlacedVisual` with `OnPlay(PiecePlaced Event,
  Handle)`) keeps the Blueprint entry **typed**, instead of an untyped payload. Two classes for one event → validation
  error.
- **Resolution per event:** an override for that tag → call it *instead of* the default hooks (it may call
  `PlayDefault()` to include them); no override → the visualizer's own per-event function (today's behaviour, so nothing
  is removed). **Registry bookkeeping still always runs in C++**, so replacing a visual can never desync the registries.
- **Themes (optional, later):** a data asset bundling a set of event visuals, chosen on the level config -- a region can
  share a look and a level can swap the whole look in one pick.
- **Costs:** more small assets to manage; a convention for which events are worth overriding (most levels override a
  handful); and being clear whether the default or the override releases the `Handle`.

### Open

1. Confirm the Place Piece diagnosis (the log lines; is `dfcaaac` in the build; is the Piece Actor Class set on the
   level config).
2. Adopt the hook rule (every hook gets position / faction / cause as parameters), starting with the piece hooks.
3. Adopt single-event visualizers (a class per event kind, a declared tag, an override list, lazy actors) beside today's
   per-event functions -- typed per-event classes vs one generic class using the typed getters.


## Round 3 (owner's decisions, 2026-10-30): event copy to hooks, who owns position, the reconcile fallback

**Supersedes** the "How hooks get their information" rule above (explicit parameters).

### Decided

1. **Hooks get a copy of the event plus the handle.** Not the event broken into parameters: `OnPlaced(Event, Handle)`,
   `OnMoved(Event, Handle)`... The event already carries the position(s), faction, `CauseIndex` and its own tag
   (`GetEventTag()`), so a hook that wants "why" reads it from the event, and a new field on an event reaches every hook
   with no signature change. It is a **copy** -- a hook can never mutate what other hooks, the log or the display board see.
   Needs one typed hook parameter per event kind (the event structs are already distinct USTRUCTs), so Blueprint stays typed.
2. **Single-event visualizers: agreed in full** (a typed class per event kind that declares its tag, an override list on the
   base visualizer / level config, lazy actors, today's per-event functions stay as the default, registry bookkeeping
   always in C++). The event-copy rule applies to them too: `OnPlay(Event, Handle)`.
3. **The registries are the source of truth for where a grid actor is.** They mediate with the board state, they own the
   position -> actor maps, and `RefreshFromBoard` already says "the registry's position is the true one". The piece's
   own fields (`OccupiedPosition` on `ConnectIt_PieceBase` and `GridPosition` on `GridPieceBase`) are a **cache for
   convenience** (hooks, UI, debugging), never read to decide where the actor belongs.
4. **A base-visualizer fallback verifies and corrects actor positions** (design below).

### Who holds the position (Claude's answer to "is it easier on the pieces?")

Keep it in the registries. Putting it on the pieces makes two truths (the board state and the actor) that must be kept in
step by every visual; the registry is the one place that already follows the board. What the pieces need is a cache that
**cannot be forgotten**.

What the code does today (checked 2026-10-30): `InitialisePiece(Faction, Position)` is a plain C++ `BlueprintCallable`,
**not** a Blueprint-overridable event, and it sets **both** `OccupiedPosition` and `GridPosition`. `SetBoardPosition` and
`RefreshFromBoard` do the same. So the miss the owner saw most likely came from the Blueprint side: a Blueprint function
that *shadows* the name, a bespoke handler that retrieves a piece from the pool and never calls `InitialisePiece`, or a
Blueprint child that sets the fields afterwards. *(To confirm with the owner which of these it was.)*

Rule: **the position fields are written only by C++** -- the registry's map / move / swap / shift functions set the
piece's cache in the same call that updates the map (so nothing can map a piece and leave the cache stale), and
`InitialisePiece` stays non-overridable; Blueprint extras go in a separate hook (`OnFactionVisualUpdate` already exists).
The `OccupiedPosition` / `GridPosition` duplication should become one field (a clean-up for the plan).

### The reconcile fallback (the base visualizer)

After the visuals of a board change are done, the base visualizer **checks every grid actor sits where the registry says
and moves it if not**.

- **When:** at the end of each event's visuals (after the handle completes, in `FinishBookkeeping`) for the actors that
  event touched, and once when the board change's event list drains for **all** registered tiles and pieces. Never
  mid-animation: while any handle is held, skip.
- **What it checks:** for every `Position -> Actor` in the tile and piece registries, the actor's world location equals
  `GridPositionToWorld(Position)` (plus the piece offset) within a small tolerance; its cached position fields equal the
  registry's; a registered piece is active and has a tile.
- **If wrong:** log an **error** naming the actor, its current location, and the expected grid position and world location
  ("Piece BP_Piece_C_3 was at (410, 220, 0) but the registry has it at (2, 1) -> (400, 200, 0); moved"), then **move it
  there** and fix the cache. Tiles first, then pieces (pieces rest on tiles) -- the same order `FinishBookkeeping` uses.
- **Relation to `TilesToSnap` / `PiecesToSnap`:** those silently snap only actors the event tracked, as the normal finish.
  The fallback is the net underneath: it catches actors a bespoke visual forgot or moved wrongly, and it is **loud**.
- **Why loud matters:** a bad visual shows up as a clear error line instead of a piece in the wrong place, and a new action
  or power can be tested with **no visual at all** -- the default visualizer moves things correctly, the errors list what a
  real visual still has to animate. (A default-visual move is not an error: the loud case is "an actor was *not* where
  the registry said after the visuals finished".)
- **Controls:** a console variable (off / log only / log and fix, default log and fix), and an automation / PIE check that
  fails a test on any reconcile error.

### Open

1. Which Blueprint path left the position unset (shadowed function, bespoke retrieve without `InitialisePiece`, or a later
   write)? Does `GetPieceAt` still return null after the fix?
2. Typed per-event visual classes (generated thin subclasses) vs one generic class with typed getters. Leaning typed.
3. Collapse `OccupiedPosition` and `GridPosition` into one field as part of this work?
4. Is "error, then fix" right for a tile mid-shift with a Blueprint that animates it long (the check waits on the handle),
   or should the check also have a timeout?

### Round 3 follow-up (2026-10-30)

- **Cause of the Place Piece miss:** the owner's Blueprint override of `GridPieceRegistry::InstantiatePiece` did not set the
  piece's position (not `InitialisePiece`, as guessed above). The registry will write the position cache itself.
- **Position fields:** keep `GridPosition` only; remove `OccupiedPosition`.
- **Reconcile timeout:** added -- it reuses the handle safety timeout.
- Plan written: [board-visualizer-revision-plan](board-visualizer-revision-plan.md) (approved 2026-10-30).
