# Hover presentation: tile effects, a pooled cursor, and how they reconcile

Design note, from the owner's notes on the hover framework. **The owner simplified it ("Simpler for now" below); that version is decided and built (2026-10-30,
[each action is its own cursor manager](../_decisions/2026-10-30-each-action-is-its-own-cursor-manager.md)).** The
richer "Revised" model stays parked.
Builds on [hover-component](hover-component.md) (components → `UGridHoverSubsystem` → listeners, approved plan:
[hover-component-plan](hover-component-plan.md)) and [board-visualizer](board-visualizer.md).

## Owner's notes

- **Today** actions tell tiles and pieces they are hovered and expect them to draw it.
- Should we also use the **cursor actor** idea from the other project -- one actor that moves from tile to tile
  instead of tiles changing their own visuals? (Easy with actor pooling.) Can both be incorporated, for maximum
  flexibility? It should also *reduce* complexity: a tile needn't know how to draw every situation; it can rely on
  actions to set the cursor look.
- Example: hovering a **lava tile** -- the generic hover actor moves to it *and* the tile is told it's hovered and plays
  a lava burst.
- To consider: tile hover effects; cursors that differ by tile; cursors that differ by **player action** (Swap: one
  cursor for your own tile, another for an opponent's tile); and **how to reconcile** an action wanting one cursor and a
  tile wanting another -- do actions override tiles?

## What exists (checked in code)

- **`AGridCursorManagerBase`** (plugin, abstract actor): already follows the hovered tile -- it binds
  `UGridHoverSubsystem::OnGridTileHoverChanged` itself and calls `UpdateCursor(Tile)`; has `EnableCursor` (hide) and
  `PauseCursor`. `AGridCursorSimpleBase` is a reference implementation (moves a mesh to the tile). **It knows nothing
  about actions** -- the cursor is a separate, always-on path.
- **Actions draw through tiles:** `UTurnBasedPlayerAction` has `IsValidHoverTile` / `HandleValidHover` /
  `HandleHoverCleared`; ConnectIt's actions implement them by `Tile->SendGameplayTag(TagActionMouseHover | TagFirstSelection
  | TagActionGridState)`, and the **tile Blueprint** turns each tag into a look. (Swap even re-sends a "first selection"
  tag to keep the first-picked tile marked after hover moves off.)
- **Pooling exists** in `UnrealGameMechanics` (`UActorPoolSubsystem`).

So both paths exist, side by side, with no coordination: the cursor ignores the action, and the action's looks are
baked into each tile Blueprint.

## The idea (Claude): three channels, composed -- not competing

Treat "what the player sees on hover" as three separate things, each owned by whoever knows it:

1. **The tile's own effects** (lava burst, a glow) -- *about the tile*. Tile-owned, plays on hover regardless of the
   action. The tile gets `OnTileHovered(Context)` / `OnTileUnhovered`, where **Context** says what the action thinks of
   this tile (valid / invalid / selected, and the action's style tag) so a tile may mute or vary its effect.
2. **The cursor** -- *about the player's intent*. Action-owned: the action chooses **which cursor style** (Place, Swap
   self, Swap opponent, Invalid...). The cursor is a **pooled actor** that sits on the hovered tile.
3. **Markers** -- *persistent* highlights (the first-picked tile in a Swap). Today a tile tag that has to be re-sent;
   better as a **second pooled cursor actor** parked on that tile until cleared.

### Reconciling "action wants one cursor, tile wants another"

Not "action overrides tile" -- **the action picks the row, the tile picks the column** of one table:

- A **cursor style table** (`UConnectIt_CursorStyleTableDataAsset`): rows are `(StyleTag, optional TileHintTag) →
  cursor actor class + settings`.
- The **action** supplies the style (`Cursor.Swap.Self`, `Cursor.Invalid`...); the **tile** may expose a **cursor hint**
  tag (`Cursor.Hint.Lava`). The lookup tries `(style, hint)`, then `(style)`, then the default. A lava tile hovered
  with Place gets the "hot Place cursor" if you author one, and the plain Place cursor if you don't.
- So tiles can influence the cursor without a fight, and actions never need to know about lava. Authoring is in one
  place; nothing is resolved in code but a table lookup.
- **Where something must be forced** (e.g. an invalid target always shows the invalid cursor even on a lava tile) the
  action's style simply has no hint rows -- or a row can be marked "ignore hints".

### Flow

1. The hover component → subsystem relays hover (as already planned).
2. **The cursor manager** (listening to the subsystem on its own, as today) shows the **default cursor** on the hovered
   tile -- this still works with no action active (opponent's turn, a menu).
3. **The active action** (also a listener, as already decided) -- in its default `HandleValidHover` / `HandleHoverCleared`
   -- does two declarative things instead of poking tile tags: **requests a cursor style** (`SetHoverStyle(ActionKey,
   StyleTag)`), and calls the tile's `OnTileHovered(Context)`. Bespoke actions can still override the handlers.
4. The cursor manager resolves the style (priority: default < action), looks up the class in the table (with the
   tile's hint), and **acquires that class from the pool**, releasing the previous one; moving between tiles is a
   transform lerp (the hover plan's lerp component), or a snap.
5. On hover cleared the action releases its request; the cursor falls back to the default (or hides).
6. **Markers:** `SetMarker(Key, Tile, StyleTag)` / `ClearMarker(Key)` -- Swap sets `Swap.First` on the first pick and
   clears it when done.

### Everything is local

The cursor and markers are per local player and never replicated; the server doesn't know about them.

## Trade-offs

- **For:** tiles stay simple (identity effects + optional hint); actions describe intent (a tag) rather than drawing;
  artists get one table to restyle every cursor; pooling keeps it cheap; Swap's first-pick marker stops being a
  re-sent tag.
- **Against:** one more moving part (the table, the request/marker API); two ways to show "this tile is a valid target"
  (the per-tile state tags actions send today, and the cursor) -- guidance: **cursor/markers for pointer and selection,
  tile state tags for region overlays** (e.g. dimming every invalid tile), and the two can coexist.
- **Migration:** additive. Existing tile tag handling keeps working; actions move to cursor requests one at a time;
  `AGridCursorManagerBase` gains the request/marker API behind its current `UpdateCursor` hook.
- Home: the cursor service in `UnrealGridMechanics` (generic, extends `AGridCursorManagerBase`); the style table and the
  action glue in ConnectIt (and a `StyleTag` property on the plugin's `UTurnBasedPlayerAction`).

## Revised (owner's second round, 2026-10-29): actions own the cursor, tiles never do

**Owner's thoughts:** it's tricky to say who controls what. Generally an **action controls the cursor**; Swap needs
**marker cursors plus a hover cursor**, perhaps a specific one. A tile or level could want its own hover look -- tiles
can easily do a visual update when told they're hovered, but a tile-specific cursor during an action is harder; and how
does a tile know which visual update to run -- **specific tags per action?** Using the **same cursors for an action's
parts** is easiest, with the cursor classes living in the action. **Tiles probably shouldn't be responsible for, or
interact with, cursors** at all. Actions tell tiles their state (**hovered, selected, valid, invalid...**). Cursor
selection logic can live in a third-party class the action uses, can have a **per-level override** over a **default**,
works **per action** (Swap defines its own *cursor selection object*), and has a **"no cursor" option** so the tile's own
visual update can carry the look.

**What the code does today (checked):** each action has `EditDefaultsOnly` tag properties it sends to tiles --
`TagActionGridState`, `TagActionMouseHover`, `TagActionMouseHoverInvalid` (Place), plus `TagFirstSelection` (Swap). They
are set per action in Blueprint, so **a tile Blueprint has to know every action's tags** to react -- exactly the problem
you named.

### Claude's take: this simplification is right -- it removes the tile-hint column

Adopt the owner's rule: **tiles never choose or touch cursors.** That drops the `(style, tile hint)` lookup from the
first round; the table becomes per action. Two vocabularies, kept apart:

1. **Tile states -- generic, told to tiles** (so a tile needs no per-action tags): `Hovered`, `Selected`, `Valid`,
   `Invalid` (and `Neutral` to clear). The action also passes an optional **role** tag for the detail
   (`Swap.PlayerSelection`) -- a tile that cares can read it; most just see "Selected". A tile's visual update is
   `OnTileState(State, Role)`. Existing per-action tag properties become the *roles*, not the contract.
2. **Cursor roles -- per action, mapped to cursors:** `Place.Hovered`, `Place.Selected`, `Swap.Hovered`,
   `Swap.PlayerSelection`, `Swap.OpponentSelection` (the owner's list). Each says which **slot** it uses: the **hover**
   slot (follows the mouse) or a **marker** slot (stays on a tile until cleared).

**The cursor selection object** (`UGridCursorSelection`, a Blueprintable data asset / `UObject`): maps `Role →
{cursor actor class, slot}`; one per action, defined in that action's defaults (Place's, Swap's...). It may be
subclassed for logic ("is this the opponent's piece?") when a plain map isn't enough. **The action decides the role**
(it knows validity and ownership); **the object maps role → cursor**. A role mapped to **None** is the "no cursor"
option -- the action still tells the tile its state and the tile's own visual carries the look.

**Per-level override:** one **default** cursor selection (project settings: the fallback for any action); each action's
own object overrides the default; and a **level** (its World Settings / config) can supply **replacement objects per
action** -- e.g. a lava level's hot cursors. Resolution: level override for that action → the action's own → the default.

**The cursor manager** (extending `AGridCursorManagerBase`) stays the presenter: it holds the hover slot and marker
slots, takes the class from the resolved object, and acquires / releases it from the actor pool. Actions call
`ShowRole(Role, Tile)` / `ClearRole(Role)`; the manager does the rest. With no active action, the manager shows the
default object's hover cursor (as today).

**The tile still reacts in its own way:** `OnTileHovered`-style effects (your lava burst) come from the generic state
call, independent of any cursor.

**Limit to accept:** a *tile-specific* cursor during an action isn't supported directly. The way to get one is the "no
cursor" role plus the tile's own visual (the owner's suggestion), or a level override. That keeps tiles and cursors
fully separate.

## Simpler for now (owner, 2026-10-29, third round) -> a plan

The owner chose to **start simpler and expand later, since some of the details above are unclear**:

- **The standard: player actions control cursors**; tiles carry a **`CursorState`** variable that caches which state
  they are in and lets them respond when given a new one (they never choose cursors).
- **Actions set up their own cursors, bespoke.** ~~Through `AGridCursorManagerBase`~~ -- *revised later the same day
  (owner):* **each action is its own cursor manager.** It already receives every hover and click from the hover
  subsystem, and cursors may switch constantly, so the manager "pulls control away from the actions". The action takes
  cursors from the actor pool itself (helpers on the plugin's `UTurnBasedPlayerAction`), shows, hides and moves them,
  and returns them on exit. `AGridCursorManagerBase` stays for now but the new path doesn't use it.
- **A tile's `CursorState` is set explicitly** (`SetCursorState`, its own call) -- it is **not** forwarded to
  `SendGameplayTag`, so a tile knows it is being told a cursor state.
- **Each action decides how its follow cursor works**, including dynamically (e.g. a different cursor class per tile).

Everything in the "Revised" section (style tables, role tags, selection objects, level overrides) is **parked** as the
way to expand later. Plan: [action-cursors-plan](action-cursors-plan.md) (**built 2026-10-30**).

## Open

1. ~~Confirm the composition (action style + tile hint)~~ -- superseded by the owner's "Revised" section: actions own
   the cursor; tiles never do.
2. Evolve the existing `AGridCursorManagerBase` (lean) or add a separate service.
3. Should the AI's chosen tile get a cursor too (showing the opponent's move)? Same markers, a different key.
4. Where the style tags live (`ConnectIt.Cursor.*`) and who defines per-action ones (the action class, in defaults).
5. Confirm the revised model: generic tile states (+ optional role), per-action cursor selection objects mapping roles
   to cursors (with a "no cursor" role), a default plus level overrides.
6. Role tag names and home (`ConnectIt.Cursor.<Action>.<Role>`?), and the generic tile state tags
   (`ConnectIt.TileState.*`?).
