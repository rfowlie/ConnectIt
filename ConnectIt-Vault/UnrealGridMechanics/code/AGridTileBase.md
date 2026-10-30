---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/Tile/GridTileBase.h
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Private/Tile/GridTileBase.cpp
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/Tile/GridCursorStateTags.h
reconciled: 2026-10-30
commit: 075f269
---

# AGridTileBase

The board tile actor every game's tiles derive from (ConnectIt: `AConnectIt_TileBase`). It knows nothing about its
position; registries map tiles and positions.

## When you touch this

- A tile's hover or cursor look: implement `OnCursorStateChanged`.
- A new kind of tile: subclass it (or the game's tile base).

## Entry points

- **`HoverComponent`** ([[UnrealGridMechanics/code/UGridHoverComponent|UGridHoverComponent]]) raises hover begin, end
  and click from the tile's cursor events, in C++.
- **Cursor state:**
  - `SetCursorState(Tag)`, `ClearCursorState()`, `GetCursorState()` (no tag = neutral);
  - a change raises `OnCursorStateChanged(Previous, Current)` (BlueprintNativeEvent, protected: the tile shows it
    here), then the `OnTileCursorStateChanged(Tile, Previous, Current)` delegate. Setting the same state again raises
    nothing.
  - Standard tags (`GridCursorStateTags.h`): `Grid.CursorState.Hovered`, `.Selected`, `.Valid`, `.Invalid`. Any tag
    works.
- `SendGameplayTag(Tag)` (BlueprintNativeEvent): a general tag channel, kept for anything else that uses it.
- `IGridBoardActor::RefreshFromBoard`: empty here; a game's tile reads its own board data.

## Collaborators

- **Player actions set cursor states**, never the tile itself (`UTurnBasedPlayerAction::SetTileCursorState`). See
  [[UnrealTurnBasedMechanics/code/recipes/give-a-player-action-a-cursor|give a player action a cursor]].
- Registries: `UGridTileRegistryComponent` discovers tiles and registers them with `UGridHoverSubsystem`
  ([[UnrealGridMechanics/code/systems/hover-relay|hover-relay]]).

## Gotchas

- `SetCursorState` is **not** forwarded to `SendGameplayTag`. A Blueprint that only reacts there shows no cursor
  states.
- One state at a time.
- `OnGridTileBeginCursorOver` is deprecated: listen to `HoverComponent` or the hover subsystem.

## Changes

- 2026-10-30 -- cursor state (`SetCursorState`, `OnCursorStateChanged`, the standard tags), from the
  [action-cursors-plan](../../ConnectIt/design/action-cursors-plan.md). Page written.
- 2026-10-28 -- `HoverComponent`, `IGridBoardActor`.

## See also

- [[UnrealGridMechanics/CLAUDE|UnrealGridMechanics overview]]
