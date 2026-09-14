# CLAUDE.md — UnrealGridMechanics

A generic 2D grid/board framework with no game-specific rules: core coordinate/direction
value types, pure-math Blueprint libraries for grid geometry and N-in-a-row / shape
analysis, tile and piece actors, registry objects that derive board dimensions from actual
tile placement, hover/cursor tracking subsystems, a replicated board-state container, a
compute-and-animate row/column "shift with wraparound" system, and narrow query
interfaces. A separate editor module adds asset validation.

## Domain

- **Source:** `../Plugins/UnrealGridMechanics/Source/` — `UnrealGridMechanics` (Runtime)
  + `UnrealGridMechanicsEditor` (Editor). Sub-areas: `Board/`, `Tile/`, `Piece/`,
  `Cursor/`, `Subsystem/`, `Interface/`, top-level grid math; editor `Validator/`.
- **Status:** active — enabled in `ConnectIt.uproject`. Deps: `GameplayAbilities`,
  `DataValidation`, [UnrealGameMechanics](../UnrealGameMechanics/CLAUDE.md).
- **Type:** game-agnostic plugin, independently extractable.
- **Consumed by:** [UnrealTurnBasedMechanics](../UnrealTurnBasedMechanics/CLAUDE.md),
  [ConnectIt](../ConnectIt/CLAUDE.md).

## Sections

- **`code/`** — per-type pages, `code/systems/`, `code/recipes/`, `code/__INDEX.md`
  (inventory + map). Governed by [`_core/_schema/_code.md`](../_core/_schema/_code.md).
- `logs/` — created on first use ([schema](../_core/_schema/_logs.md)).

## Start here

[code/__INDEX.md](code/__INDEX.md).

## Known rough edges

- Orphaned `IBoardStateInterface` (dead forward-decl); removed `IGridBoardManagerInterface`
  and the interpreter-registry indirection on board state.
- `UGridTrackerSubsystem` / `UGridHoverSubsystem` responsibilities overlap.
- Two non-functional editor validation checks in `GridValidatorBase.cpp` (commented-out
  `ValidateTileAlignment()` body; broken `BoardActor` loop ~line 26) — shared root cause.
- No `operator+=` on `EGridDirection`.
- `IGridTileSelector` supersedes the deprecated `UGridTileSelectorComponent`; migration
  incomplete.
