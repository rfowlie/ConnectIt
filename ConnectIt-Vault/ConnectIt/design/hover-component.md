# A shared hover component (tiles, pieces, map nodes)

Design note (2026-10-28), at the owner's request after [editor-notes-2026-10-28](editor-notes-2026-10-28.md) (items 1
and 3). **Settled with the owner the same day (below); nothing built.**

## Why

- **Hover depends on each Blueprint today.** `AGridTileBase::OnGridTileBeginCursorOver` (and the piece's twin) is only
  broadcast from the actor Blueprint's *Begin Cursor Over* event; no C++ raises it. A tile Blueprint that doesn't make
  that call is never hovered (the new push tile's symptom).
- **Only "hover begin" exists.** No unhover and no click are relayed; consumers infer "unhovered" from the next hover.
- **The adventure hub needs the same thing** for map nodes (hover → scale up; select → focus + panel).
- `UGridTileComponent` (an older component with begin / end cursor delegates) overlaps and looks unused.

## What exists (UnrealGridMechanics)

- `UGridHoverSubsystem` -- registered tiles / pieces only (registration by the tile / piece registries, so inactive
  pooled pieces can't hover); chains each actor's begin-cursor delegate into `OnGridTileHoverChanged` /
  `OnGridPieceHoverChanged`; does no hit-testing.
- `AGridCursorManagerBase` -- listens to tile hover and updates a cursor visual.
- ConnectIt's player actions read the hovered tile (through the subsystem) for highlighting and clicks.

## Proposed shape

- **`UHoverComponent`** (game-agnostic, `UnrealGameMechanics`, an `Interaction/` area -- the grid plugin already depends
  on it):
  - binds its **owner's native cursor events in C++** (`OnBeginCursorOver`, `OnEndCursorOver`, `OnClicked`; touch
    later) when registered -- no Blueprint wiring needed;
  - broadcasts **`OnHoverBegin`, `OnHoverEnd`, `OnClicked`** (owner + component);
  - **`bHoverEnabled`** (`SetHoverEnabled`) -- e.g. off while a piece is pooled or a node is hidden;
  - a **category** gameplay tag (`Hover.Tile`, `Hover.Piece`, `Hover.MapNode`...) so listeners can filter;
  - an optional Blueprint hook **`CanHover()`** to veto (a locked tile that shouldn't react, say);
  - **editor validation:** warns when no component of the owner blocks the cursor trace channel (the other silent way
    hover fails).
- **Tiles and pieces carry one** (`AGridTileBase` / `AGridPieceBase` create it in C++, so every subclass has it).
  - `UGridHoverSubsystem` keeps its API and its registry-gated registration, but binds the **component's** events and
    adds **hover end** and **click** relays (`OnGridTileHoverEnded`, `OnGridTileClicked`, same for pieces).
  - The actor delegates (`OnGridTileBeginCursorOver`...) stay for one release, forwarded from the component and marked
    deprecated; the tile / piece Blueprints **drop their manual broadcast** (it would fire twice).
  - `UGridTileComponent`'s cursor delegates: folded in or removed, once confirmed unused.
- **Map nodes** carry one too (alongside the node component from the hub design); the hub's manager / subsystem listens
  for `Hover.MapNode` and drives scale-on-hover, focus and the panel.
- **Local only:** cursor events happen on the machine with the mouse; nothing replicates.

## Related: actors refresh themselves from the board (owner, 2026-10-28)

All grid actors (tiles, pieces) get one function -- e.g. **`RefreshFromBoard()`** -- that the visualizers call to have
the actor update its own data from the board (not timed visual effects). Visualizers then don't need to know what each
kind of tile or piece is or what special handling it needs; the actor looks up its own board values and updates.

*Claude:* the plugin can't know a game's board type, so the plugin declares the call (an interface,
`IGridBoardActor::RefreshFromBoard`, Blueprint-native, no parameters) and each game's actors implement it -- ConnectIt's
tiles read their tile data from the **display board** at their position (multiplier, lock, chain, power), pieces their
faction. Visualizers call it after an event applies (and on board seed / layout changes) for the positions the event
touched.

## Decided (owner, 2026-10-28)

- **Lives in `UnrealGameMechanics`** -- project- and plugin-agnostic.
- **On `AGridTileBase` by default, not on pieces.** Only tiles are hovered; the piece on a tile is found **through the
  hovered tile**. That also helps with collisions: pieces should **not block the cursor trace** (so the tile under them
  gets it) -- piece collision ignores the cursor channel. The piece hover relay is retired.
- **A layer of indirection -- actors don't answer hovers.** Today the tile Blueprint reacts to hover itself
  (highlights). Instead:
  1. **components register with a subsystem** and pass it their hover begin / end / click;
  2. the **subsystem relays** them (it decides nothing);
  3. a **game-specific listener** subscribes to the subsystem and decides what a hover or click *means* -- e.g. in a
     Fire Emblem-style game: highlight the tile and the unit on it and pop up a UI. The tile never has to know about
     pieces, units or UI.
- **Clicks go through the component** too, the same path.
- **`RefreshFromBoard`: agreed** (an interface the plugin declares, each game's actors implement).

### What that means concretely (Claude)

- **`UHoverSubsystem`** (world subsystem, `UnrealGameMechanics`): `RegisterHoverable` / `UnregisterHoverable`
  (components call it on enable / disable), current hovered component, and the relays **`OnHoverBegin`,
  `OnHoverEnd`, `OnClicked`** (component + its category tag). It replaces `UGridHoverSubsystem`'s relay role; the grid
  side keeps only what's grid-specific (e.g. "the tile at a position"), or the grid subsystem becomes a thin typed
  wrapper (`OnGridTileHovered(AGridTileBase*)`) over the generic one -- to settle in the plan.
- **Registry gating stays:** the tile registry enables / disables a tile's component as tiles join / leave the board,
  so only board tiles report.
- **ConnectIt's listener:** one object per local player -- a component on the player controller (or the active player
  action, which already reads the hovered tile) -- that turns a tile hover into "highlight this tile, look up the piece
  at its position, show what the selected action would do", and a click into the action's request. The **hub's**
  listener does the same for `Hover.MapNode` (scale-up, focus, panel).
- **Migration:** the tile Blueprint's own hover reactions (highlights) move into ConnectIt's listener; its manual
  broadcast goes; the actor delegates are forwarded for one release.

## Revised (owner, 2026-10-28, later)

- **The component lives in `UnrealGridMechanics` and reuses `UGridHoverSubsystem`** (replaces "UnrealGameMechanics" and
  the new generic `UHoverSubsystem` above). The subsystem already exists and works.
- **The listeners already exist: player actions.** They subscribe to `UGridHoverSubsystem` and handle
  `HandleValidHover()` / `HandleHoveredCleared()`. An action either does something bespoke or **defers to the tile**
  to handle the hover. tile → subsystem → action → tile looks long, but the indirection is the point: more options, and
  grid actors can be decoupled when wanted.
- **`RefreshFromBoard` is on grid tiles and pieces by default** (the plugin's `AGridTileBase` / `AGridPieceBase`
  implement it, a no-op the game overrides).
- Everything else above stands: tiles only (pieces ignore the cursor), clicks through the component, hover end added,
  registry gating, the tile Blueprint's manual broadcast removed.
- *Claude:* map nodes aren't grid tiles; with the component in the grid plugin, the hub can still use it (ConnectIt
  depends on the grid plugin) through a category tag on the component -- or get its own listener path later. Noted for
  the hub plan's Stage 4.

## Open (resolved)

- ~~Where it lives~~ -- `UnrealGameMechanics`.
- ~~Clicks through the component?~~ -- yes.
- ~~A piece over a tile~~ -- only tiles hover; pieces ignore the cursor.
- ~~`RefreshFromBoard` interface or base class~~ -- interface.

## Open (old list, kept for the record)

1. Where the component lives: `UnrealGameMechanics` (lean) or `UnrealGridMechanics`.
2. Click through the component, or keep clicks on the player controller's input (the actions use the hovered tile)?
3. One hover at a time across categories (a piece over a tile: who wins?) -- today pieces and tiles both relay.
4. `RefreshFromBoard` as an interface (lean) or a base-class function.

## Open: a dedicated Grid cursor trace channel? (owner's question, 2026-10-30)

Today hover rides **Visibility** (`CursorTraceChannel = ECC_Visibility` on `UGridHoverComponent` and `AGridPieceBase`; the
player controller's default click trace). Pieces set themselves to Ignore it so the tile under the piece takes the hover,
and the hover component warns when its owner has no primitive blocking the channel.

- **For a custom channel:** nothing else on the map (decals, VFX, the hub's node actors, a big backdrop mesh) can steal
  or block the cursor; the "pieces ignore Visibility" rule becomes a default profile, not per-instance code; the
  hub (clickable nodes) and the match board can use separate channels.
- **Against:** a trace channel is a *project* setting (`DefaultEngine.ini`), so the plugin cannot own it -- it would stay
  a property whose default the game sets; every tile / piece Blueprint's collision preset and the player controller's
  click trace channel must change once; nothing is broken today.
- **Lean:** yes, but as **one project-wide setting** (the plugin reads a single "grid cursor channel" instead of a
  property per component and per piece), done together with the cursor plan's editor work so the collision presets are
  touched once. Not urgent. Undecided -- owner's call.

**Decided 2026-10-30:** make it. Plan: [grid-cursor-channel-plan](grid-cursor-channel-plan.md) (one plugin setting, the
game defines the channel; a plugin can carry the ini -- UE merges a plugin's `Config/` inis into the project branches --
but the slot number and filename need a test).

Plan approved 2026-10-30, with the base `AGridTileBase` / `AGridPieceBase` setting their collision for the cursor channel in C++.
