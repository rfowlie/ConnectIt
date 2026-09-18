---
schema: code
reconciled: 2026-09-10
commit: 6477d5d
---

# UnrealGridMechanics — code index

Inventory + module map. Governed by [[_core/_schema/_code|_core/_schema/_code.md]].
Domain overview: [[UnrealGridMechanics/CLAUDE|CLAUDE.md]].

`status`: **current** = written & reconciled · **stub** = no page yet · **covered in X**
= documented inside another type's page.

## Inventory

| Type | Kind | Area | Role | Source (`Public/…`) | Status |
|---|---|---|---|---|---|
| [[UnrealGridMechanics/code/GridTypes\|GridTypes]] (`FGridPosition`, `EGridDirection`, `FGridMovement`, `FGridPair`, `FGridDirectionVector`) | USTRUCT / UENUM | Top-level | primary (value types) | `GridMechanicsBaseStructs.h`, `GridMechanicsBaseEnums.h` | current |
| [[UnrealGridMechanics/code/UGridMechanics_GridLibrary\|UGridMechanics_GridLibrary]] | UCLASS (UBlueprintFunctionLibrary) | Top-level | primary | `GridMechanics_GridLibrary.{h,cpp}` | current |
| [[UnrealGridMechanics/code/UGridMechanics_ShapeLibrary\|UGridMechanics_ShapeLibrary]] | UCLASS (UBlueprintFunctionLibrary) | Top-level | primary | `GridMechanics_ShapeLibrary.{h,cpp}` | current |
| [[UnrealGridMechanics/code/UGridTileRegistryComponent\|UGridTileRegistryComponent]] | UCLASS (component) | Tile | primary | `Tile/GridTileRegistryComponent.{h,cpp}` | current |
| [[UnrealGridMechanics/code/UGridPieceRegistryComponent\|UGridPieceRegistryComponent]] | UCLASS (component) | Piece | primary | `Piece/GridPieceRegistryComponent.{h,cpp}` | current |
| [[UnrealGridMechanics/code/UBoardShiftComponent\|UBoardShiftComponent]] | UCLASS (component) | Board/Shift | primary | `Board/Shift/BoardShiftComponent.{h,cpp}` | current |
| UGridMechanics_GridShiftLibrary | UCLASS (UBlueprintFunctionLibrary) | Board/Shift | primary (pure math) | `Board/Shift/GridMechanics_GridShiftLibrary.{h,cpp}` | covered in UBoardShiftComponent |
| Shift types (`FShiftOperation`, `FShiftResult`, `FTileShiftInstruction`, `EShiftAxis`, `EShiftDirection`) | USTRUCT / UENUM | Board/Shift | primary (data) | `Board/Shift/GridShiftTypes.h` | covered in UBoardShiftComponent |
| UBoardStateComponentBase (`FTileData`, `OnBoardStateChanged`) | UCLASS (abstract component) | Board | primary | `Board/BoardStateComponentBase.h` | stub |
| UGridHoverSubsystem | UCLASS (UWorldSubsystem) | Subsystem | primary | `Subsystem/GridHoverSubsystem.{h,cpp}` | covered in systems/hover-relay |
| UGridTrackerSubsystem | UCLASS (UWorldSubsystem) | Subsystem | internal (overlaps hover) | `Subsystem/GridTrackerSubsystem.h` | stub |
| Registry bases (`UGridTileRegistryBase`, `UGridPieceRegistryBase`) | UCLASS (UObject) | Tile / Piece | primary (the variant ConnectIt subclasses) | `Tile/GridTileRegistryBase.h`, `Piece/GridPieceRegistryBase.h` | stub |
| [[UnrealGridMechanics/code/UGridDefinition\|UGridDefinition]] | UCLASS (UObject) | Registry | primary (grid geometry, shared by both registry bases above — extracted 2026-09-18) | `Registry/GridDefinition.{h,cpp}` | stub |
| AGridTileBase / UGridTileComponent | UCLASS | Tile | primary | `Tile/GridTileBase.h`, `Tile/GridTileComponent.h` | stub |
| AGridPieceBase / AGridUnitBase (GAS) | UCLASS | Piece | primary | `Piece/GridPieceBase.h`, `Piece/Unit/GridUnitBase.h` | stub |
| AGridCursorManagerBase / AGridCursorSimpleBase | UCLASS | Cursor | **unused in this project** (hover is tag-based) | `Cursor/GridCursor*.h` | stub |
| ABoardManagerBase / AGameBoard | UCLASS | Board | prototype (unused) | `Board/BoardManagerBase.h`, `Board/GameBoard.h` | stub |
| Interfaces (`IGridFactionInterface`, `IGridLevelInterface`, `IGridPieceHandler`, `IGridTileHandler`, `IGridShiftInterface`, `IGridTileSelector`, `IBoardStateInterface`) | UINTERFACE ×7 | Interface / Board | mixed — `IGridFactionInterface` / `IGridShiftInterface` primary; `IBoardStateInterface` orphaned | `Interface/*.h`, `Board/BoardStateInterface.h`, `Tile/Selector/GridTileSelector.h` | stub |
| UGridTileSelectorComponent | UCLASS (component) | Tile/Selector | deprecated (use `IGridTileSelector`) | `Tile/Selector/GridTileSelectorComponent.h` | stub |
| UGridValidatorBase | UCLASS | Editor/Validator | primary (asset validation) | `UnrealGridMechanicsEditor/Public/Validator/GridValidatorBase.h` | stub |
| Module classes | class (IModuleInterface) | Top-level | internal | `UnrealGridMechanics.h`, `UnrealGridMechanicsEditor.h` | stub |

## Map

`Plugins/UnrealGridMechanics/Source/` — `UnrealGridMechanics` (Runtime) +
`UnrealGridMechanicsEditor` (Editor). Sub-areas:

- **Top-level grid math** — [[UnrealGridMechanics/code/GridTypes|GridTypes]]
  (`FGridPosition`, `EGridDirection`) are the currency;
  [[UnrealGridMechanics/code/UGridMechanics_GridLibrary|UGridMechanics_GridLibrary]]
  (direction/vector/rotation conversions, N-in-a-row connection analysis) and
  [[UnrealGridMechanics/code/UGridMechanics_ShapeLibrary|UGridMechanics_ShapeLibrary]]
  (line / square detection) are pure functions on them.
- **`Tile/` + `Piece/`** — `AGridTileBase` / `AGridPieceBase` actors and their
  **registries** (actor↔position tracking, board dimensions from placement, piece pool
  retrieval + hover registration). The `…RegistryBase` (UObject) variant is what ConnectIt
  subclasses; the `…RegistryComponent` variant is older.
- **`Subsystem/`** — `UGridHoverSubsystem` relays cursor-over for **registered** tiles/
  pieces only (pooled-inactive pieces don't fire). See
  [[UnrealGridMechanics/code/systems/hover-relay|systems/hover-relay]].
  `UGridTrackerSubsystem` overlaps it (rough edge).
- **`Board/`** — `UBoardStateComponentBase` (a replicated tile-data container consumers
  subclass) and **`Board/Shift/`**: compute-then-animate row/column wraparound. See
  [[UnrealGridMechanics/code/systems/board-shift|systems/board-shift]].
- **`Cursor/`** — `AGridCursor*` actors are **not wired up in this project**.
- **Editor module** — `UGridValidatorBase` asset validation (two checks non-functional).

**Start at:** `GridTypes` → the registries → `hover-relay`.

## Related

- Flows: [[UnrealGridMechanics/code/systems/__INDEX|code/systems/__INDEX.md]]
- Recipes: [[UnrealGridMechanics/code/recipes/__INDEX|code/recipes/__INDEX.md]]
