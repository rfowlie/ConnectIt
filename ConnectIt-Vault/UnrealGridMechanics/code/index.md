---
schema: code
reconciled: 2026-09-06
commit: 3086271
---

# UnrealGridMechanics — code index

Per-type help pages for the plugin's public API (runtime module + `UnrealGridMechanicsEditor`).
Governed by [[_schema/code|_schema/code.md]]. An **overlay** on the authoritative in-repo
reference ([`old/Plugins/UnrealGridMechanics/Docs/README.md`](../../../old/Plugins/UnrealGridMechanics/Docs/README.md),
[`old/Plugins/UnrealGridMechanics/Docs/Systems.md`](../../../old/Plugins/UnrealGridMechanics/Docs/Systems.md)).

Source root: `Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/` (runtime),
`.../UnrealGridMechanicsEditor/` (editor).
`status`: **current** = written & reconciled · **stub** = no page yet.

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
| UGridHoverSubsystem | UCLASS (UWorldSubsystem) | Subsystem | primary | `Subsystem/GridHoverSubsystem.{h,cpp}` | covered in systems/hover-relay (page stub) |
| UGridTrackerSubsystem | UCLASS (UWorldSubsystem) | Subsystem | internal (overlaps hover) | `Subsystem/GridTrackerSubsystem.h` | stub |
| AGridTileBase / UGridTileComponent | UCLASS | Tile | primary | `Tile/GridTileBase.h`, `Tile/GridTileComponent.h` | stub |
| AGridPieceBase / AGridUnitBase (GAS) | UCLASS | Piece | primary | `Piece/GridPieceBase.h`, `Piece/Unit/GridUnitBase.h` | stub |
| AGridCursorManagerBase / AGridCursorSimpleBase | UCLASS | Cursor | primary | `Cursor/GridCursor*.h` | stub |
| ABoardManagerBase / AGameBoard | UCLASS | Board | primary | `Board/BoardManagerBase.h`, `Board/GameBoard.h` | stub |
| Registry bases (`UGridTileRegistryBase`, `UGridPieceRegistryBase`) | UCLASS | Tile / Piece | internal (bases) | `Tile/GridTileRegistryBase.h`, `Piece/GridPieceRegistryBase.h` | stub |
| Interfaces (`IGridFactionInterface`, `IGridLevelInterface`, `IGridPieceHandler`, `IGridTileHandler`, `IGridShiftInterface`, `IGridTileSelector`, `IBoardStateInterface`) | UINTERFACE ×7 | Interface / Board | primary (`IGridFactionInterface`, `IGridShiftInterface`); rest mixed | `Interface/*.h`, `Board/BoardStateInterface.h`, `Tile/Selector/GridTileSelector.h` | stub |
| UGridTileSelectorComponent | UCLASS (component) | Tile/Selector | deprecated (use `IGridTileSelector`) | `Tile/Selector/GridTileSelectorComponent.h` | stub |
| UGridValidatorBase | UCLASS | Editor/Validator | primary (asset validation) | `UnrealGridMechanicsEditor/Public/Validator/GridValidatorBase.h` | stub |
| Enums (`GridMechanicsBaseEnums.h` extras) / module classes | UENUM / class | Top-level | mixed | `GridMechanicsBaseEnums.h`, `UnrealGridMechanics.h`, `UnrealGridMechanicsEditor.h` | stub |

## Related

- Flows: [[UnrealGridMechanics/systems/index|systems/index.md]]
- Recipes: [[UnrealGridMechanics/recipes/index|recipes/index.md]]
