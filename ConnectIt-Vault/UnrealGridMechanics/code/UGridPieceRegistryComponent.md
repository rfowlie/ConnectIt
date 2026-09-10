---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/Piece/GridPieceRegistryComponent.h
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Private/Piece/GridPieceRegistryComponent.cpp
reconciled: 2026-09-10
commit: f694874
---

# UGridPieceRegistryComponent

`UActorComponent`. Owns piece existence end to end: "what piece is at this position"
(`GetPiece`) plus getting one there / taking one away — **pool** retrieval/release
([[UnrealGameMechanics/code/UActorPoolSubsystem|UActorPoolSubsystem]]), hover-subsystem
registration, and the `IActorPoolInterface` activate/deactivate calls all live here.

## When you touch this

- Spawning / removing pieces on the board.
- Querying occupancy or faction ownership.

## Entry points

- **Query:** `GetPiece(Position)`, `GetPositionOfPiece`, `GetAllPositions`,
  `GetAllPositionsOfFaction(FactionId)`, `GetAllPiecesOfFaction(FactionId)`.
- **Convenience:** `SpawnPieceAt(TSubclassOf<AGridPieceBase>, AGridTileBase*)` =
  `RetrievePiece` + `ActivatePieceAt`; `DespawnPieceAt(Position)` = `DeactivatePiece` +
  `ReleasePiece` (no gap).
- **Primitives (use when you must inject work between steps):** `RetrievePiece` (pool +
  hover-register, **not** positioned/activated), `ActivatePieceAt` (pool-activate only),
  `DeactivatePiece`, `ReleasePiece` (unregister + release to pool).
- `InstantiatePiece(Position)` — `BlueprintImplementableEvent`.
- `PieceMap` (`TMap<FGridPosition, AGridPieceBase*>`, `BlueprintReadOnly`).
- Position conversion mirrors the tile registry.

## Collaborators

- `UActorPoolSubsystem` (retrieve/release/activate/deactivate), `UGridHoverSubsystem`
  (register/unregister — pooled inactive pieces are **not** hover-registered),
  [[UnrealGridMechanics/code/UGridTileRegistryComponent|UGridTileRegistryComponent]]
  (`ResolveTileRegistry`).
- Reference callers: `UConnectIt_PlacePieceGameEvent`, `UConnectIt_LineScoreGameEvent`
  (game module) — **both currently stubbed pending rewiring**.

## Gotchas

- **`ActivatePieceAt` does NOT position the piece.** `Tile` is validated but unused; a
  sibling "piece-spawn interpreter" that used to do positioning + visual-wait was removed
  and not replaced. This is a **known gap**, not a simplification — positioning and any
  spawn-in/despawn-out visual wait must be wired up elsewhere (Blueprint or a new
  collaborator).
- Nothing here waits on a visual-completion signal any more.
- Use the four primitives, not `SpawnPieceAt`/`DespawnPieceAt`, when you need to
  initialise a piece between retrieval and activation (so the visual reflects init state).

## Cross-impact

Depends on the pool's get/activate split — see
[[UnrealGameMechanics/code/systems/actor-pooling-lifecycle|actor-pooling-lifecycle]]. Changing
the primitive set affects the game module's place/score game events and any Blueprint
that composes them.

## Changes

- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[UnrealGridMechanics/CLAUDE|UnrealGridMechanics overview]] → *Piece*.
- [[UnrealGridMechanics/code/recipes/place-and-remove-a-piece|recipes/place-and-remove-a-piece]]
