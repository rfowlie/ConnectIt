---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/GridMechanics_GridLibrary.h
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Private/GridMechanics_GridLibrary.cpp
reconciled: 2026-09-06
commit: ae9c1f0
---

# UGridMechanics_GridLibrary

`UBlueprintFunctionLibrary`. Pure grid math: direction ↔ vector ↔ rotation conversion,
grid-position arithmetic, 8-neighbour traversal, and **N-in-a-row connection analysis**
(Connect-Four-style scoring).

## When you touch this

- Converting between world transforms and grid coordinates / directions.
- Scoring a move by how many length-N lines it could form.

## Entry points

- **Direction:** `GetGridDirectionVector`, `GetGridDirectionFromDegrees`,
  `GetRotationFromGridDirection`, `GetActorGridDirection`, `GetRotatedGridDirection`
  (rotate local→absolute, wrapping), `GetNormalizedGridPositionByDirection`. Static
  `GridDirectionVectors` map (clockwise from Up).
- **Position math:** `CalculateGridPositionFromSize` (world→cell; `Size` clamped ≥10),
  `GetAverageGridPosition` (centroid), `GetClosestNormalizedGridPositionFromPositions`,
  `GetClosestGridDirectionBetweenPositions` (returns `Up` + warns if P1==P2).
- **Traversal:** `GetPositionNeighbors` — 8 neighbours, clockwise from Up.
- **Connections:** `CountValidWindows` (windows of `ConnectLength` through a point along
  one direction; O(L²)), `CountPossibleGridConnections` (sum over 8 dirs; default
  `ConnectLength = 4`; **0 if the point isn't itself in `ValidPositions`**),
  `GetPotentialConnectionCountPerDirection` (per-direction breakdown, for AI),
  `CreateConnectionsFloodMap` (C++-only; sorted `TArray<TPair<int32,FGridPosition>>` for
  whole-board "best move" ranking).

## Collaborators

- Operates on [[UnrealGridMechanics/code/GridTypes|GridTypes]] +
  `TSet<FGridPosition>` / `TArray<AGridTileBase*>`.
- Consumers: the game module's AI/scoring, `UnrealGameIntelligence` influence-map
  providers.

## Gotchas

- Connection functions treat `ValidPositions` as "occupied by the side you're scoring for"
  — **pre-filter by ownership** before calling; the library doesn't.
- `CountPossibleGridConnections` / `GetPotential…` return 0 / empty if the query position
  isn't in `ValidPositions`.
- `CreateConnectionsFloodMap` is deliberately **not** `BlueprintCallable` (return type).
- `GetClosestGridDirectionBetweenPositions` degrades to `Up` on P1==P2.

## Cross-impact

Self-contained (pure functions). Changing the neighbour order or the
`GridDirectionVectors` map would shift every consumer's direction indexing —
[[UnrealGridMechanics/code/GridTypes|EGridDirection]] arithmetic assumes clockwise-from-Up.

## See also

- In-repo: `Docs/README.md` → *Library*; `Docs/Systems.md` → grid math.
- [[UnrealGridMechanics/code/UGridMechanics_ShapeLibrary|UGridMechanics_ShapeLibrary]] (line/shape detection)
