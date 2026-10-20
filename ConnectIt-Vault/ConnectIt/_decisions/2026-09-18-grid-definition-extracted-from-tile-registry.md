---
Date: 2026-09-18
status: Active
superseded by:
tags:
  - architecture
  - registries
  - unrealgridmechanics
---
## Decision

Grid geometry — cell size, `WorldToGridPosition`/`GridPositionToWorld` — is extracted out
of `UGridTileRegistryBase` into its own class, `UGridDefinition`
([[UnrealGridMechanics/code/UGridDefinition|code page]], plugin `UnrealGridMechanics`).
Both `UGridTileRegistryBase` and `UGridPieceRegistryBase` now hold a public
`GridDefinition` reference as **siblings** — neither depends on the other for it.
`UGridPieceRegistryBase::TileRegistry` (the member this replaces) is removed entirely.
Wired the same way the registries themselves already are: a new `Instanced` template
property on `UConnectIt_LevelConfigDataAsset`, `DuplicateObject`'d per world by
`UConnectIt_BoardRegistrySubsystem::OnWorldBeginPlay` — before either registry's own
`InitialiseRegistry()` runs, since both need it already assigned.

## Why

Found while implementing
[2026-09-18 — Registry mapping refresh on GameEvent complete](2026-09-18-registry-mapping-refresh-on-game-event-complete.md):
`UGridPieceRegistryBase::TileRegistry` existed for exactly one reason — reaching
`WorldToGridPosition` inside `UpdateMappings()` — and was never actually assigned
anywhere in the codebase, so that function null-derefs if called. The deeper problem
wasn't the missing assignment, it was the dependency shape: a piece registry needing a
whole tile registry just to borrow grid-geometry math is "which actor is at this
position" (tile-tracking) and "where in the world is grid position (X,Y)" (geometry)
bundled together, forcing one registry to depend on the other for a concern neither
actually owns.

The plugin's own header comment already singled out `GridPositionToWorld`/
`WorldToGridPosition` as *the* deliberate per-subclass override point for non-square
grids (hex, isometric) — extracting them doesn't lose that; a project now overrides
`UGridDefinition` instead of the tile registry, same capability, correctly-scoped home.
Repo-wide grep confirmed the blast radius was small: zero external callers of
`UGridTileRegistryBase`'s own conversion functions (only internal self-calls via
`GetPositionOfTile`), so removing rather than leaving pass-through wrappers was safe for
C++ call sites — Blueprint-graph call sites can't be grepped, so this needs an editor
check before/alongside build verification (see the code page's Gotchas).

As a side effect, `UpdateMappings()` (both tile- and piece-side) was fixed to read from
`GridDefinition` instead of the null `TileRegistry` — it no longer null-derefs, though
it's still not wired to run automatically; see the companion decision on why not.

## What Would Change It

`UGridTileRegistryComponent`/`UGridPieceRegistryComponent` (the `ActorComponent`
counterparts, not what ConnectIt uses) have their own separate, redundant `GridSize`/
conversion implementation with the identical smell — not touched in this pass. Worth the
same extraction if that pair is ever actually used.
