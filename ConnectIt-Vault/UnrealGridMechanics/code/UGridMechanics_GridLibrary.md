---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/GridMechanics_GridLibrary.h
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Private/GridMechanics_GridLibrary.cpp
reconciled: 2026-09-21
commit: aa8373e
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
  (rotate local→absolute, wrapping), `GetNormalizedGridPositionByDirection`,
  `RotateGridDirection(EGridDirection, int32 RotationAmount)` (wraps around the 8-way
  compass via modulus — rotate an `EGridDirection` value itself, distinct from
  `GetRotatedGridDirection`'s local→absolute rotation). Static `GridDirectionVectors` map
  (clockwise from Up). **Convention follows Unreal's axes:** +X is forward/`Up`, +Y is right/`Right`;
  `FGridDirectionVector(Row, Column)` is the (X step, Y step) — `Up {1,0}`, `Right {0,1}`, `Down {-1,0}`,
  `Left {0,-1}`, diagonals combine them. See
  [decision](../../ConnectIt/_decisions/2026-09-20-grid-axes-follow-ue-x-forward-y-right.md).
- **Position math:** `CalculateGridPositionFromSize` (world→cell; `Size` clamped ≥10),
  `GetAverageGridPosition` (centroid), `GetClosestNormalizedGridPositionFromPositions`,
  `GetClosestGridDirectionBetweenPositions` (returns `Up` + warns if P1==P2).
- **Traversal:** `GetPositionNeighbors` — 8 neighbours, clockwise from Up.
- **Connections:** `CountValidWindows` (windows of `ConnectLength` through a point along
  one direction; O(LÂ²)), `CountPossibleGridConnections` (sum over 8 dirs; default
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

## Changes

- 2026-09-21 — `GridDirectionVectors` table flipped to UE axes (X forward); previously `Up {0,1}` etc.
- 2026-09-18 — **`RotateGridDirection` added** (see Entry points). (`process-code` sweep
  — commit `9187568`.)
- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[UnrealGridMechanics/CLAUDE|UnrealGridMechanics overview]] → *Library*.
- [[UnrealGridMechanics/code/UGridMechanics_ShapeLibrary|UGridMechanics_ShapeLibrary]] (line/shape detection)
