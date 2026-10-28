---
schema: code
kind: UCLASS (UActorComponent)
role: primary
source:
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/Hover/GridHoverComponent.h
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Private/Hover/GridHoverComponent.cpp
reconciled: 2026-10-28
commit: 742ba9d
---

# UGridHoverComponent

## What it is

Makes its owner hoverable and clickable **with no Blueprint wiring**. On `BeginPlay` it binds the owner's native cursor
events (Begin / End Cursor Over, Clicked) in C++ and broadcasts `OnHoverBegin`, `OnHoverEnd` and `OnClicked`. It
decides nothing: listeners decide what a hover means. Every `AGridTileBase` creates one (`HoverComponent`).

## When you touch this

- A tile (or another actor, e.g. a map node) isn't hovered or clicked.
- Something should stop a tile reacting for a while (`SetHoverEnabled`, or override `CanHover`).
- Hover should come from other input (touch, gamepad): call the `Notify...` functions.

## Entry points

- `OnHoverBegin` / `OnHoverEnd` / `OnClicked` (component as the parameter).
- `SetHoverEnabled` / `IsHoverEnabled`: turning it off during a hover ends the hover (`OnHoverEnd`).
- `IsHovered`.
- `CanHover()` (Blueprint-native, default true): a veto on hover and click.
- `NotifyHoverBegin` / `NotifyHoverEnd` / `NotifyClicked`: the one path the owner's cursor events, other input and the
  tests all use. A repeated begin, or an end with no hover, does nothing.
- `HoverCategory` (gameplay tag, for listeners that filter); `CursorTraceChannel` (default Visibility, used only by
  map check).

## Collaborators

- [[UnrealGridMechanics/code/systems/hover-relay|hover-relay]]: `UGridHoverSubsystem` binds a registered tile's
  component and relays it as tile hover / end / click.
- `AGridTileBase` creates it and forwards its hover begin to the deprecated `OnGridTileBeginCursorOver`, for one
  release.

## Gotchas

- **Something must block the cursor trace.** Hover needs a primitive of the owner that blocks the player controller's
  click trace channel; map check (`CheckForErrors`) warns when none does. Pieces ignore that channel by default
  (`AGridPieceBase::bBlocksCursorTrace`), so the tile under a piece gets the cursor.
- **Local only:** cursor events happen on the machine with the mouse; nothing replicates.
- On a tile, enabling belongs to the hover subsystem's registration (only board tiles report). Toggle it yourself only
  for a tile that is registered.
- Don't broadcast the tile's old `OnGridTileBeginCursorOver` by hand: the component raises hover now.

## Cross-impact

- Changing the delegates → `UGridHoverSubsystem` (`BindTile`) and `AGridTileBase::ForwardHoverBegin`.
- Changing enable semantics → `UGridHoverSubsystem::RegisterTile` / `UnregisterTile`.

## Changes

- 2026-10-28 -- created (hover-component plan, Stage 1): C++ cursor binding, begin / end / click, enable, veto,
  category, map check. On every `AGridTileBase`. Tests: `UnrealGridMechanics.Hover.Component`.

## See also

- [[UnrealGridMechanics/code/systems/hover-relay|systems/hover-relay]]
- Design: `ConnectIt/design/hover-component.md`; decision:
  [[ConnectIt/_decisions/2026-10-28-hover-through-a-component|hover through a component]].
