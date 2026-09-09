# UnrealGridMechanics — high level

## Purpose

A generic 2D grid/board framework with no game-specific rules. It supplies the core
coordinate and direction value types, pure-math Blueprint libraries for grid geometry and
N-in-a-row / shape analysis, tile and piece actors, registry components that derive board
dimensions from actual tile placement, hover/cursor tracking subsystems, a replicated
board-state container, a compute-and-animate row/column "shift with wraparound" system,
and a set of narrow query interfaces. A separate editor module adds asset validation.

## Status

Active — enabled in `ConnectIt.uproject`. Depends on
[[../UnrealGameMechanics/high-level|UnrealGameMechanics]].

## Key modules

- `UnrealGridMechanics` (Runtime). Sub-areas: `Board/`, `Tile/`, `Piece/`, `Cursor/`,
  `Subsystem/`, `Interface/`, plus top-level grid math.
- `UnrealGridMechanicsEditor` (Editor, PostEngineInit). `Validator/`.
- Plugin dependencies: `GameplayAbilities`, `DataValidation`, `UnrealGameMechanics`.

## Public API surface

- **Value types:** `FGridPosition` (full operator set + `GetTypeHash`), `EGridDirection`
  (8-way compass with wraparound arithmetic), `FGridMovement`, `FGridPair`.
- **Math libraries:** `UGridMechanics_GridLibrary` (`CountValidWindows`,
  `CountPossibleGridConnections`, `CreateConnectionsFloodMap`);
  `UGridMechanics_ShapeLibrary` (`GetLongestLine`, `GetLinesOfLength`, `IsSquare`);
  `UGridMechanics_GridShiftLibrary` (row/col shift with wraparound).
- **Actors:** `AGridTileBase`, `AGridPieceBase`, `AGridUnitBase` (GAS-enabled).
- **Registries:** `UGridTileRegistryComponent` (position <-> world conversion,
  board-dimension queries), `UGridPieceRegistryComponent` (`GetPiece(Position)`, pool
  retrieve/release).
- **Subsystems / cursor:** `UGridHoverSubsystem` (`OnGridTileHoverChanged` /
  `OnGridPieceHoverChanged`), `UGridTrackerSubsystem`, `AGridCursorManagerBase` /
  `...SimpleBase`.
- **Board state:** `UBoardStateComponentBase` + `FTileData`, broadcasts
  `OnBoardStateChanged`.
- **Shift:** `UBoardShiftComponent` + `IGridShiftInterface`.
- **Interfaces:** `IGridFactionInterface`, `IGridLevelInterface`, `IGridPieceHandler`,
  `IGridTileHandler`, `IGridShiftInterface`, `IGridTileSelector` (supersedes the
  deprecated `UGridTileSelectorComponent`).
- **Editor:** `UGridValidatorBase`.

## Consumers

`UnrealTurnBasedMechanics` (depends on it) and the `ConnectIt` game module.

## In-repo reference

- [`../../old/Plugins/UnrealGridMechanics/Docs/README.md`](../../old/Plugins/UnrealGridMechanics/Docs/README.md)
- [`../../old/Plugins/UnrealGridMechanics/Docs/Systems.md`](../../old/Plugins/UnrealGridMechanics/Docs/Systems.md)

## Vault code docs

Overlay on the in-repo docs: per-type entry-point/gotcha pages, flow diagrams, task
recipes, git-anchored for freshness. Partial (see each index for status).

- [[UnrealGridMechanics/code/index|code/index.md]] ([[_schema/code|schema]])
- [[UnrealGridMechanics/systems/index|systems/index.md]] ([[_schema/systems|schema]])
- [[UnrealGridMechanics/recipes/index|recipes/index.md]] ([[_schema/recipes|schema]])

## Known rough edges

- Orphaned `IBoardStateInterface`.
- `UGridTrackerSubsystem` / `UGridHoverSubsystem` responsibilities overlap.
- `ValidateTileAlignment()` body is commented out.
- Non-functional `BoardActor` loop in `GridValidatorBase.cpp` (~line 26).
- No `operator+=` on `EGridDirection`.

_Verify against the in-repo `old/Plugins/UnrealGridMechanics/Docs/` before acting on this list — it reflects a past snapshot._
