---
schema: recipes
task: Place a piece on a given tile and later remove it, going through the pool and the piece registry.
touches:
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/Piece/GridPieceRegistryComponent.h
  - your AGridPieceBase subclass (optionally implementing IActorPoolInterface)
  - Blueprint or C++ that positions the piece at its tile (see the known gap)
reconciled: 2026-09-10
commit: f694874
---

# Recipe: place and remove a piece

## Goal

A piece of a chosen class appears on a chosen tile, is queryable via `GetPiece`, and can
be cleanly removed back to the pool.

## Prerequisites

- An actor with a
  [[UnrealGridMechanics/code/UGridPieceRegistryComponent|UGridPieceRegistryComponent]] and
  a [[UnrealGridMechanics/code/UGridTileRegistryComponent|UGridTileRegistryComponent]].
- An `AGridPieceBase` subclass. If it needs custom show/hide, implement
  `IActorPoolInterface::ActivatePoolObject` / `DeactivatePoolObject` on it.
- A `UActorPoolSubsystem` in the world (it is a `UWorldSubsystem` — always present).

## Steps

### Simple placement (no work between retrieve and activate)

1. `AGridTileBase* Tile = TileRegistry->GetTileAtPosition(Pos);`
2. `AGridPieceBase* Piece = PieceRegistry->SpawnPieceAt(MyPieceClass, Tile);`
   — this does `RetrievePiece` (pool + hover-register) then `ActivatePieceAt`
   (pool-activate).
3. **Position the piece.** `SpawnPieceAt` / `ActivatePieceAt` do **not** move the piece to
   the tile (known gap — the old positioning collaborator was removed). Set its transform
   from `TileRegistry->GridPositionToWorld(Pos)` (or the tile's transform) yourself, in
   Blueprint or a small C++ step, *before* any spawn-in visual.
4. Record it if you keep your own map; `PieceRegistry->GetPiece(Pos)` now returns it.

### Placement with initialisation between steps

1. `AGridPieceBase* Piece = PieceRegistry->RetrievePiece(MyPieceClass);` (pool + register,
   inactive, not positioned)
2. Initialise the piece (faction, mesh, data) and position it at the tile.
3. `PieceRegistry->ActivatePieceAt(Piece, Tile);` (pool-activation side effect only)

### Removal

- Immediate: `PieceRegistry->DespawnPieceAt(Pos);` (deactivate + unregister + release, no
  gap).
- Gated on a despawn visual: `PieceRegistry->DeactivatePiece(Piece);` … bind/await your
  visual-complete signal … `PieceRegistry->ReleasePiece(Piece);` (finalises: unregister +
  release to pool). Only after `ReleasePiece` does `GetPiece(Pos)` stop returning it.

## Verify

- `PieceRegistry->GetPiece(Pos)` returns the piece after placement, `nullptr` after
  `ReleasePiece` / `DespawnPieceAt`.
- Hover works on the placed piece (it was `RegisterPiece`'d) and stops after removal.
- Pool counts (`UActorPool::GetActive/InactivePoolCount`) move by 1 on
  place / remove — the actor is reused, not spawned/destroyed.
- `GetAllPiecesOfFaction(FactionId)` includes it once its faction is set.

## Pitfalls

- **Expecting the piece to be at the tile after `SpawnPieceAt`** — it won't be; you must
  position it (step 3 above).
- Calling `DeactivatePiece` without a later `ReleasePiece` — the position stays mapped and
  the pooled actor is never returned.
- Binding a spawn-in completion listener *after* `ActivatePieceAt` when activation can
  finish synchronously — bind first (this is why the pool splits get/activate; see
  [[UnrealGameMechanics/code/systems/actor-pooling-lifecycle|actor-pooling-lifecycle]]).
- Using `SpawnPieceAt` when you needed to initialise the piece first — use the primitives.

## See also

- [[UnrealGridMechanics/code/UGridPieceRegistryComponent|UGridPieceRegistryComponent]] ·
  [[UnrealGameMechanics/code/UActorPoolSubsystem|UActorPoolSubsystem]]
- [[UnrealGridMechanics/code/systems/hover-relay|systems/hover-relay]]
