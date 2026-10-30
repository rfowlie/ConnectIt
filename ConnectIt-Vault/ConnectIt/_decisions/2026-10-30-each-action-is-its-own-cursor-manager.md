---
Date: 2026-10-30
status: Active
superseded by:
tags:
  - architecture
  - hover
  - cursors
summary: 'Player actions own cursors: each UTurnBasedPlayerAction takes cursor actors from the actor pool and sets tiles'' cursor states (AGridTileBase::SetCursorState, a gameplay tag, its own call apart from SendGameplayTag); everything is released and every touched tile goes back to neutral when the action ends. Tiles never choose a cursor; AGridCursorManagerBase is left unused'
---
## Decision

**Each player action is its own cursor manager, and a tile shows a cursor state it is given.**

- **Tile cursor state:** `AGridTileBase` has a `CursorState` (a gameplay tag; no tag = neutral).
  - `SetCursorState` stores it and raises the Blueprint-native `OnCursorStateChanged(Previous, Current)` plus the
    `OnTileCursorStateChanged` delegate. Setting the same state again does nothing.
  - It's **its own call, apart from `SendGameplayTag`**, so a tile knows it's being given a cursor state.
  - The standard tags are `Grid.CursorState.Hovered / Selected / Valid / Invalid`; an action may set any tag.
- **Every cursor implements `IGridCursor`** (UnrealGridMechanics `Cursor/GridCursorInterface.h`; owner,
  2026-10-30). It's **mandatory, with no fallback**:
  - `AcquireActionCursor` refuses any other class with a logged error, so it gets fixed right away;
  - cursor class properties carry `MustImplement`, so the class picker only offers cursors.
  - The interface: `MoveCursor(Tile, Location, bReactivate)` (the tile, and the tile's location plus the action's
    offset; `bReactivate` replays arrival effects when the cursor lands on a different tile), `ShowCursor`,
    `HideCursor`.
  - Two lifecycles: the **pool** means in use (activated on acquire, deactivated on release), and **Show / Hide**
    means visible while held.
- **Helpers on `UTurnBasedPlayerAction`** (so every action in any game has them):
  - cursors from the actor pool: `AcquireActionCursor`, `ShowCursorAt`, `HideCursor`, `ReleaseActionCursor(s)`;
  - the follow cursor: `ShowHoverCursor(Class, Tile)` swaps the class per tile, `HideHoverCursor`;
  - tile states: `SetTileCursorState`, which remembers the tiles it touched.
- **Cleanup is automatic.** Complete, cancel and force-deactivate return every cursor to the pool and every touched
  tile to neutral.
- **A new hover hook:** `HandleInvalidHover(Tile)` fires when the cursor moves onto a tile the action can't use, so an
  action can show an "invalid" cursor.
- **ConnectIt's actions:**
  - Place: a hover state + `HoverCursorClass`;
  - Swap: a hover state; the follow cursor's class depends on the tile (own piece, opponent's piece, invalid); a
    first-pick state + `MarkerCursorClass`;
  - Shift: the hovered line's state; no cursor of its own.
  - No class set = no cursor.
- **`AGridCursorManagerBase` is untouched and unused.** Don't place it in a map that uses action cursors.

## Why

- The action already receives every hover and click and knows the situation (which pick, whose piece). A separate
  manager would "pull control away from the actions" (owner, 2026-10-29).
- Tiles then only need to show a state, not know every action's situation.
- With a cached state, a tile has something to compare against, and nothing has to re-send tags just to keep a look
  (Swap's re-sent first-pick tag is gone).
- Automatic cleanup means an action that forgets can't leak a cursor or leave a tile stuck.
- **Simple first:** style tables, role tags, selection objects and level overrides stay parked in
  [hover-cursor](../design/hover-cursor.md) until the details are clear.

## Consequences

- The actions' `TagActionGridState` properties are gone, since neutral is now no tag. `TagActionMouseHover` /
  `TagFirstSelection` stay as the cursor-state values; their Blueprint values (`ConnectIt.Tile.ValidHover`) carry
  over.
- **Nothing forwards to the old `SendGameplayTag` path.** Tile Blueprints must handle `OnCursorStateChanged`, or
  tiles show no hover look.

## Links

- Plan: [action-cursors-plan](../design/action-cursors-plan.md); design: [hover-cursor](../design/hover-cursor.md).
- Recipe: [give a player action a cursor](../../UnrealTurnBasedMechanics/code/recipes/give-a-player-action-a-cursor.md).
- Related: [hover through a component](2026-10-28-hover-through-a-component.md).
