---
Date: 2026-10-28
status: Active
superseded by:
tags:
  - architecture
  - input
  - hover
  - grid
summary: 'Hover / click come from a C++ component on every tile (pieces ignore the cursor), relayed by the hover subsystem to the player actions, which decide; tiles don''t react on their own; IGridBoardActor::RefreshFromBoard lets actors update themselves from the board; old delegates deprecated one release'
---
## Decision

**Hover and click come from a component on tiles, through the hover subsystem, to listeners that decide what they mean.
Actors refresh themselves from the board.**

- **`UGridHoverComponent`** (UnrealGridMechanics) is on every `AGridTileBase`. It raises hover begin / end and click
  from the tile's cursor events **in C++**, so no Blueprint has to broadcast anything.
- **Tiles only.** Pieces ignore the cursor trace; the piece on a tile is found through the hovered tile.
- **Indirection stays:** components → `UGridHoverSubsystem` (registered board tiles only; it decides nothing) → the
  listeners, i.e. the player actions (`HandleValidHover`, `HandleHoverCleared`, `HandleTileClicked`). They do something
  bespoke or defer to the tile. **Tiles don't react to hover on their own.**
- **`IGridBoardActor::RefreshFromBoard()`** (no parameters) on grid tiles and pieces. ConnectIt's tile reads its data
  from the display board (`OnRefreshedFromBoard`), and its piece reads its faction. The board state component calls it
  where the display board moved on, so visuals don't need to know tile or piece types.
- The old actor delegates and the piece relay are deprecated and forwarded for one release, then removed.

## Why

- **Hover depended on each Blueprint:** only the tile Blueprint broadcast hover, so a new tile Blueprint without that
  call was never hovered (the push tile bug). There was also no hover end and no click.
- **Indirection gives options:** a hover can mean different things per action, and grid actors stay decoupled from
  pieces, units and UI.
- **Refresh by the actor itself:** visualizers that copied board values into each kind of tile had to know every kind.

## What Would Change It

- Touch or gamepad input: it would call the component's `Notify...` functions (same path).
- Hovering something that isn't a tile (map nodes): the component already carries a category tag. The adventure hub
  plan's Stage 4 decides whether that goes through this subsystem or its own listener.

Plan: [hover-component-plan](../design/hover-component-plan.md). Design: [hover-component](../design/hover-component.md).
Code: [[UnrealGridMechanics/code/UGridHoverComponent|UGridHoverComponent]],
[[UnrealGridMechanics/code/systems/hover-relay|hover-relay]].
