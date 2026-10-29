# Board visualizer: registries, events and the artist entry point

Design note (2026-10-29), from the owner's notes on registries and visualizers. **Decided the same day (all three leans, owner); nothing
built.** Related: [hover-component](hover-component.md) (`RefreshFromBoard`),
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
