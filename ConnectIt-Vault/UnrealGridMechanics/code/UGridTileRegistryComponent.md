---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/Tile/GridTileRegistryComponent.h
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Private/Tile/GridTileRegistryComponent.cpp
reconciled: 2026-09-06
commit: ec992a2
---

# UGridTileRegistryComponent

`UActorComponent`. Owns the authoritative list of grid tiles in the level, discovers them
(level-placed **and** spawned/streamed later), registers each with
[[UnrealGridMechanics/code/GridTypes|the hover subsystem]], and answers grid queries —
including **board dimensions derived from actual tile positions**, no hardcoded size.

## When you touch this

- Converting grid ↔ world (`GridSize`).
- Asking "what tile is here", "give me row/column N", "how big is the board".

## Entry points

- **Config:** `GridSize` (world units per cell, default 200).
- **Conversion:** `GridPositionToWorld`, `WorldToGridPosition`.
- **Tile queries:** `GetTileAtPosition`, `GetPositionOfTile`, `GetRow` / `GetColumn`
  (+ `GetRowPositions` / `GetColumnPositions`), `GetAllTiles` / `GetAllTilePositions`.
- **Dimensions:** `GetMin/MaxRow`, `GetMin/MaxColumn`, `GetRowCount`, `GetColumnCount` —
  all computed from the registered tile set.

## Collaborators

- On `BeginPlay`: `DiscoverTiles()` sweeps the world for `AGridTileBase`; an
  `OnActorSpawned` handler (`ActorSpawnedHandle`) picks up later ones.
- Resolves & caches `UGridHoverSubsystem`; registers/unregisters each tile there.
- Consumed by
  [[UnrealGridMechanics/code/UGridPieceRegistryComponent|UGridPieceRegistryComponent]]
  (position conversion), `UBoardShiftComponent` (world-position map), the game's board
  manager.

## Gotchas

- Board dimensions are only as correct as the placed tiles — a stray/misplaced tile skews
  `GetMax*`. `ValidateTileAlignment()` exists **`WITH_EDITOR` only** and (per the
  plugin's known rough edges) its body has been commented out at points.
- `GridPositionToWorld` uses `GridSize` — it must match how tiles were actually laid out.
- Discovery is world-wide `AGridTileBase` — multiple boards in one level need care.

## Cross-impact

Change `GridSize` semantics or the discovery mechanism and also update
`UGridPieceRegistryComponent`, `UBoardShiftComponent::BuildWorldPositionMap`, the game
board manager, and `UGridValidatorBase` (editor).

## See also

- In-repo: `Docs/README.md` → *Tile*; `Docs/Systems.md` → registries.
- [[UnrealGridMechanics/systems/hover-relay|systems/hover-relay]]
