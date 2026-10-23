---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/Registry/GridDefinition.h
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Private/Registry/GridDefinition.cpp
reconciled: 2026-10-23
commit: 6abda4a
---

# UGridDefinition

A plain `UObject` (`Blueprintable, EditInlineNew, DefaultToInstanced`) holding a grid's **geometry**: the cell size and
the conversion between grid positions and world locations. It answers "where in the world is grid position (X,Y)";
the registries answer "which actor is there". Usable as is, with no subclass.

## When you touch this

- Converting between grid positions and world locations, anywhere: spawning a tile or piece at a cell, finding the
  cell an actor stands on.
- A grid that isn't square (hex, isometric): subclass it and override the two conversions.
- Setting up a board: its instance is a template on the level config, next to the registries.

## Entry points

- **`GridSize`** (`int32`, world units per cell, default 200).
- **`GridPositionToWorld(Position)`** → `FVector`: `(X × GridSize, Y × GridSize, 0)`.
- **`WorldToGridPosition(WorldLocation)`** → `FGridPosition`: each axis divided by `GridSize`, rounded to the nearest
  cell.
- Both are `BlueprintNativeEvent` and `BlueprintPure`: the one override point for a different grid shape.

## Collaborators

- **The registry bases hold it as siblings:** `UGridTileRegistryBase::GridDefinition` and
  `UGridPieceRegistryBase::GridDefinition`. Tile discovery and registration map each tile actor by
  `WorldToGridPosition(ActorLocation)`; the piece registry maps pieces the same way. Without it, discovery still lists
  the tiles but the position map stays empty (logged).
- **ConnectIt:**
  - The template lives on `UConnectIt_LevelConfigDataAsset::GridDefinition` (`Instanced`).
  - [[ConnectIt/code/UConnectIt_BoardRegistrySubsystem|UConnectIt_BoardRegistrySubsystem]] duplicates it per world
    **first**, then hands that one instance to both registries before either initialises.
  - The tile registry spawns layout tiles at `GridPositionToWorld(Cell.Position)`.
  - The layout tool captures cells with `WorldToGridPosition` and places builder tiles with `GridPositionToWorld`.
  - A builder tile turns its chain partner picker into a position with `WorldToGridPosition`.
- Decision: [grid definition extracted from the tile registry](../../ConnectIt/_decisions/2026-09-18-grid-definition-extracted-from-tile-registry.md).

## Gotchas

- **Z is always 0** in the default `GridPositionToWorld`: a plain UObject has no owner to anchor to (the old component
  version used its owner's Z). A board that isn't at Z = 0 needs a subclass, or the caller sets Z.
- **No origin offset:** cell (0,0) is the world origin. Tiles placed elsewhere still work (rounding finds their cell),
  but a board must sit on the `GridSize` lattice from the origin.
- **Rounding, not flooring:** an actor half a cell off lands in the nearest cell; one placed exactly between two can go
  either way.
- **Assign it before the registries initialise:** a registry that initialises without it never fills its position
  map. ConnectIt's subsystem order (definition first) is load-bearing.
- **Template vs instance:** the level config's object is a shared template. Use the per-world duplicate (the
  subsystem's `GetGridDefinition()`), except in editor tools that only read `GridSize` and convert (the layout tool,
  builder tiles).

## Cross-impact

- Changing the conversion moves every tile and piece mapping: both registry bases, ConnectIt's tile registry spawn,
  the layout tool (captured layouts are stored as positions), and the builder tile's chain partner.
- A new geometry field (an origin offset, a Z) belongs here, not on a registry; both registries and the editor tools
  then pick it up.

## Changes

- 2026-10-23 — page written. The class was extracted from `UGridTileRegistryBase` on 2026-09-18 (`6abda4a`), a pure
  relocation: the registries now hold it as siblings.

## See also

- [UnrealGridMechanics code index](__INDEX.md)
- [[ConnectIt/code/UConnectIt_BoardRegistrySubsystem|UConnectIt_BoardRegistrySubsystem]] (how ConnectIt wires it)
- [[UnrealGridMechanics/code/UGridTileRegistryComponent|UGridTileRegistryComponent]] (the older component that held
  its own `GridSize`)
