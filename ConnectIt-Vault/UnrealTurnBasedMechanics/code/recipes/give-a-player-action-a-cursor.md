---
schema: recipes
task: Give a player action its own cursors (a follow cursor, markers) and tile cursor states, released and cleared automatically when the action ends.
touches:
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/TurnBasedPlayerAction.h (the cursor helpers, HandleInvalidHover)
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/Tile/GridTileBase.h (SetCursorState, OnCursorStateChanged)
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/Tile/GridCursorStateTags.h (the standard states)
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/Cursor/GridCursorInterface.h (IGridCursor, mandatory)
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/Pooling/ActorPoolSubsystem.h (where cursors come from)
  - the action's subclass and its cursor Blueprints; the tile Blueprint
reconciled: 2026-10-30
commit: 075f269
status: 'current'
---

# Recipe: give a player action a cursor

## Goal

While the action is active, a cursor follows the hovered tile (maybe a different one per tile), markers stay where
the action puts them, and tiles show their state. When the action ends by any route, the cursors go back to the pool
and the tiles to neutral. Decision: [each action is its own cursor manager](../../../ConnectIt/_decisions/2026-10-30-each-action-is-its-own-cursor-manager.md).

## Prerequisites

- An action with selection: [add a turn action](add-a-turn-action.md) (`bRequiresSelection`, the hover hooks).
- **Cursor actors implement `IGridCursor`** (mandatory; any other class is refused with an error):
  - `MoveCursor(Tile, Location, bReactivate)`: go to `Location` (the tile plus the action's `CursorOffset`), or work
    from the tile; replay arrival effects when `bReactivate` is set;
  - `ShowCursor` / `HideCursor`: visibility while the action holds it.
  - The pool's activate / deactivate (`IActorPoolInterface`, optional) is the separate "in use" lifecycle.

## Steps

1. **Declare the classes** as `EditDefaultsOnly` `TSubclassOf<AActor>` properties on the action, e.g.
   `HoverCursorClass`, with `meta = (MustImplement = "/Script/UnrealGridMechanics.GridCursor")` so the picker only
   offers cursors. Treat "none set" as "no cursor".
2. **Follow cursor:**
   - In `HandleValidHover(Tile)`, call `ShowHoverCursor(ClassFor(Tile), Tile)`. A different class swaps the cursor
     (the old one goes back to the pool). The follow cursor is reactivated whenever it lands on a different tile.
   - In `HandleHoverCleared`, call `HideHoverCursor()`.
   - For tiles the action can't use, override `HandleInvalidHover(Tile)`, e.g. `ShowHoverCursor(InvalidClass, Tile)`.
3. **Markers / previews:** `AcquireActionCursor(Class)` gives a hidden cursor. Use
   `ShowCursorAt(Cursor, Tile, bReactivate)` (it calls `MoveCursor`, then `ShowCursor` if it was hidden) and
   `HideCursor(Cursor)`.
   - Keep the pointer, but check `HoldsActionCursor(Cursor)` before reusing it in a later activation; it was released
     when the action ended.
4. **Tile states:** `SetTileCursorState(Tile, State)`, with a `Grid.CursorState.*` tag or your own, and no tag for
   neutral. The action remembers the tile.
5. **Ending:** nothing to do. Complete, cancel and force-deactivate release every cursor and clear every touched tile.
   If you override `ForceDeactivate_Internal`, call Super.
6. **Tile Blueprint:** implement **On Cursor State Changed (Previous, Current)** and switch on `Current`. No tag means
   neutral. Don't react to these in `SendGameplayTag`; nothing forwards there.

## Verify

- PIE: the cursor follows, swaps class on the right tiles, and leaves nothing behind after cancel, ending the turn, or
  switching actions.
- Tests: `UnrealGridMechanics.Tile.CursorState`, `ConnectIt.PlayerAction.Cursors`, `.CursorsAdopted`.

## Gotchas

- **One state per tile at a time:** the last `SetTileCursorState` wins. To keep a mark while hover passes over a tile
  (Swap's first pick), don't overwrite it in the hover hooks.
- **Don't place `AGridCursorManagerBase`** in a map that uses action cursors, or there will be a second cursor.
- **Cursor actors shouldn't block the cursor trace channel** (GridCursor, `UGridMechanicsSettings`), or they take the
  hover from the tile under them. GridCursor ignores everything by default, so this only matters if you set it.
- **No action active, no cursor** (e.g. the opponent's turn).
- Cursors snap to the tile; there's no lerp.
