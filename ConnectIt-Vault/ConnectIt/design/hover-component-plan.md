---
type: plan
status: complete
approved: true
approved_on: 2026-10-28
completed_on: 2026-10-28
started:             2026-10-28          # YYYY-MM-DD, when the first stage was built (blank until then)
stages: 4
current_stage: complete
owner_done: false          # the owner's editor checks for this plan (a checkbox; separate from the code stages)
playtested: false
design: "[hover-component](hover-component.md)"
question:
decision: "[2026-10-28-hover-through-a-component](../_decisions/2026-10-28-hover-through-a-component.md)"
---

# Plan: the grid hover component and RefreshFromBoard

Design: [hover-component](hover-component.md) (settled 2026-10-28); background:
[editor-notes-2026-10-28](editor-notes-2026-10-28.md). Written 2026-10-28 in a design-only session; **approved by the owner
2026-10-28**. **Built 2026-10-28** (all stages; Stage 4 by the owner).

## Context

Settled with the owner:

- **A hover component in `UnrealGridMechanics`**, reusing **`UGridHoverSubsystem`**. It raises hover begin / end and
  click **in C++** from its owner's cursor events -- no Blueprint has to broadcast anything (today only the tile
  Blueprint does, which is why a new tile Blueprint can go unhovered).
- **On tiles by default, not pieces.** The piece on a tile is found through the hovered tile; **pieces ignore the cursor
  trace channel** so the tile under them is hit. The piece hover relay is retired.
- **Indirection stays:** components → `UGridHoverSubsystem` → the listeners (player actions, `HandleValidHover` /
  `HandleHoveredCleared`), which do something bespoke or defer to the tile. Tiles don't react to hover on their own.
- **Clicks go through the component** too.
- **`RefreshFromBoard`** on grid tiles and pieces by default: visualizers call it; the actor updates itself from the
  board, so visualizers don't need to know tile or piece types.

Each stage builds and passes the tests before the next; the editor must be closed for builds; the owner commits between
stages.

## Stage 1: the component (plugin)

- **`UGridHoverComponent`** (`UnrealGridMechanics/Public/Tile/` or a `Hover/` area): on `BeginPlay` binds its owner's
  `OnBeginCursorOver`, `OnEndCursorOver`, `OnClicked` (C++); broadcasts `OnHoverBegin`, `OnHoverEnd`, `OnClicked`
  (component); `SetHoverEnabled` / `IsHoverEnabled`; a **category** gameplay tag; a Blueprint-native **`CanHover()`**
  veto (default true).
- **Editor validation** (`IsDataValid` on the owning actor class, or the component's `CheckForErrors`): warn when no
  primitive component of the owner blocks the cursor trace channel.
- **`AGridTileBase`** creates one in its constructor (`CreateDefaultSubobject`), so every tile class has it.
- Tests: a component with hover disabled doesn't relay; `CanHover` false vetoes (unit-level, no world where possible).
- **As built (2026-10-28):** `Public/Hover/GridHoverComponent.h`. Also: `IsHovered`; public `NotifyHoverBegin` /
  `NotifyHoverEnd` / `NotifyClicked` (what the owner's cursor events call -- other input, and the tests, use the same path);
  disabling during a hover ends it; `HoverCategory` tag; `CursorTraceChannel` (default Visibility) for the check, which is
  the component's map check (`CheckForErrors`). The grid plugin's first tests: `UnrealGridMechanics.Hover.Component`
  (run with `Automation RunTests ConnectIt+UnrealGridMechanics`). `InputCore` added to the plugin's public dependencies.

## Stage 2: the subsystem binds components

- `UGridHoverSubsystem::RegisterTile` / `UnregisterTile` bind / unbind the tile's **component** events (registration
  stays driven by the tile registry, so only board tiles report; registration also enables / disables the component).
- New relays: **`OnGridTileHoverEnded`** and **`OnGridTileClicked`**, next to `OnGridTileHoverChanged`.
- **Actions:** `UTurnBasedPlayerAction` gains `HandleTileClicked(Tile)` (Blueprint-native) beside `HandleValidHover` /
  `HandleHoveredCleared`; the actions' existing hover handling is unchanged.
- **Deprecated, forwarded for one release:** `AGridTileBase::OnGridTileBeginCursorOver` (the component also broadcasts
  it) -- so existing Blueprints keep working until cleaned.
- **Pieces:** `RegisterPiece` / `OnGridPieceHoverChanged` and `AGridPieceBase::OnGridPieceBeginCursorOver` deprecated;
  `AGridPieceBase` sets its primitives to **ignore the cursor channel** by default (a Blueprint can override).
- `UGridTileComponent`'s cursor delegates: removed if nothing references them (the legacy `Connectit_CursorOverComponent`
  asset under `Content/_Legacy` may; checked first).
- Tests: registering a tile binds its component; unregistering unbinds; hover end and click reach the relays.
- **As built (2026-10-28):** the action also handles **hover end** (`OnGridTileHoverEnded`: clears its hovered tile
  and calls `HandleHoverCleared`) -- needed for "hovering off a tile clears the highlight". `HandleTileClicked` does
  nothing by default: clicks already select through the selection input (`HandleValidSelection`), so a default would
  select twice. Pieces: `bBlocksCursorTrace` (default false) + `CursorTraceChannel`; primitives set to ignore it in
  `PostInitializeComponents` (a Blueprint that resets its collision profile later undoes it -- check in Stage 4). The
  deprecated piece relay is untouched otherwise (no asset uses it). `UGridTileComponent`'s delegates removed (no
  asset references the class; the `_Legacy` cursor component uses the tile actor's delegate). `UGridTrackerSubsystem`
  still binds the tile's old delegate, but nothing registers tiles with it. Tests: `UnrealGridMechanics.Hover.Subsystem`.

## Stage 3: RefreshFromBoard

- **`IGridBoardActor`** (plugin interface): `RefreshFromBoard()` (Blueprint-native, no parameters). `AGridTileBase` and
  `AGridPieceBase` implement it as a no-op the game overrides.
- **ConnectIt:** `AConnectIt_TileBase` reads its tile data from the **display board** at its position (multiplier,
  active, lock, chain, power) and pushes it to Blueprint (`OnRefreshedFromBoard(TileData)`); ConnectIt's piece reads
  its faction.
- **Visualizers** call it for the positions an event touched once the event applies to the display board, and for every
  actor on board seed / layout changes -- one helper (`RefreshActorsAt(Positions)`) on the board event library.
- Tests: none without a world; checked in PIE.
- **As built (2026-10-28):** `IGridBoardActor` (`Public/Interface/GridBoardActor.h`). **The display board calls it, not
  the visualizers:** `UConnectIt_BoardStateComponent` refreshes the actors where the display board moved on --
  after each event is applied (positions whose tile data differs; every position if the tiles themselves changed),
  on the catch-up to Current, and on seed / a client's first snapshot (every position, after the builder swap) -- so
  no visualizer has to. `RefreshActorsAt` (board event library) is also Blueprint-callable. The tile finds its
  position through the tile registry, the piece through a new `UGridPieceRegistryBase::FindPositionOfPiece` (a
  piece's own `GridPosition` went stale on moves; the refresh now keeps it in step); refresh never creates a piece
  (`GetPieceAtPosition`, not `GetPiece`). The tile hands Blueprint `OnRefreshedFromBoard(TileData)` (a hook; nothing
  happens until a tile Blueprint implements it). **Also (owner, 2026-10-28): `UGridTrackerSubsystem` removed** --
  nothing read it; `AGridUnitBase` no longer registers with it.

## Owner checks: ConnectIt clean-up

- `CI_GridTileBase`: remove its manual *On Grid Tile Begin Cursor Over* broadcast and its own hover reactions;
  highlights move to the actions' `HandleValidHover` (or the action defers to a tile function).
- Pieces: confirm they no longer block the cursor; the hovered tile gives the piece.
- Visual Blueprints: replace bespoke per-type updates with `RefreshFromBoard` where they only copy board values.
- Retest the benched **push tile** hover.

## Stage 5: vault

`UnrealGridMechanics` code pages (hover component, subsystem changes); ConnectIt decision note (hover through a
component on tiles; listeners decide; RefreshFromBoard); recipe update for tiles; log.

## Not in this plan

- Map node hover (the adventure hub plan's Stage 4 uses this component with its own category).
- Touch input.

## Verification

- Each stage: build (editor closed); `Automation RunTests ConnectIt` and the grid plugin's tests pass.
- Owner, PIE: a brand-new tile Blueprint (no graph) is hovered and placeable; hovering off a tile clears the highlight
  (hover end); a piece on a tile doesn't block hovering that tile; clicks reach the action; the push tile hovers.

## Settled before approval (owner, 2026-10-28: as written)

1. `RefreshFromBoard` takes no parameters (each game's actor finds its own data).
2. The deprecated actor / piece delegates are kept forwarded for one release, then removed.
