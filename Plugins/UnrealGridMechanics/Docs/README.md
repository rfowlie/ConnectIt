# UnrealGridMechanics

## Purpose

UnrealGridMechanics is the core 2D grid/board framework: tile and piece actors, grid registries and subsystems, replicated board-state with an observer/interpreter pattern, a board "shift" (row/column push with wrap-around) system, cursor/hover tracking, and pure grid-math function libraries (direction handling, N-in-a-row connection analysis, shape detection). It exists as its own plugin rather than living inside a specific game module because none of this logic is specific to any one set of rules — it's a generic 2D grid toolkit that any grid-based game could consume. Its current consuming project shapes some of the library's conveniences (e.g. `IsSquare`, N-in-a-row scoring), but the plugin boundary keeps grid math and grid actors decoupled from any one game's specific win conditions, piece rules, and visuals.

## Module(s)

| Module | Type | LoadingPhase | Notes |
|---|---|---|---|
| `UnrealGridMechanics` | Runtime | Default | Core grid/board framework. |
| `UnrealGridMechanicsEditor` | Editor | PostEngineInit | Editor-only data validation for grid content; depends on the runtime module. |

## Key Dependencies

- **Depends on:**
  - Plugin-level: `GameplayAbilities`, `DataValidation`
  - `UnrealGridMechanics` (runtime) — Public: `Core`, `GameplayTags`, `GameplayTasks`, `GameplayAbilities`; Private: `CoreUObject`, `Engine`, `Slate`, `SlateCore`
  - `UnrealGridMechanicsEditor` — Public: `Core`; Private: `Core`, `CoreUObject`, `Engine`, `UnrealEd`, `DataValidation`, plus the `UnrealGridMechanics` runtime module
- **Depended on by:**
  - `UnrealTurnBasedMechanics` (declared at both plugin level and in its Build.cs Public *and* Private dependency lists — see Notes below)
  - The consuming game module (not named here, to keep this plugin's documentation project-agnostic)

Notes: `UnrealTurnBasedMechanics.Build.cs` lists `UnrealGridMechanics` in both `PublicDependencyModuleNames` and `PrivateDependencyModuleNames`, which is redundant (the Public listing alone is sufficient to also satisfy private use). Minor, not functionally harmful.

## When to Use It

- Building any actor or system that needs 2D grid coordinates, neighbor traversal, or direction math (`FGridPosition`, `EGridDirection`).
- Representing tiles and pieces on a board (`AGridTileBase`, `AGridPieceBase`) independent of any specific game's rules.
- Needing row/column-push "shift" mechanics with wraparound, either the pure math (`UGridMechanics_GridShiftLibrary`) or the actor-driven version (`UBoardShiftComponent`).
- Detecting lines, N-in-a-row connections, or shapes on a grid (`UGridMechanics_GridLibrary`, `UGridMechanics_ShapeLibrary`).
- Tracking or reacting to tile hover/cursor state (`UGridWorldSubsystem`, `AGridCursorManagerBase`).
- Needing a generic, replicated board-state container with pluggable observers (`UBoardStateComponentBase` + `UBoardStateInterpreter`).

See [Systems.md](Systems.md) for a narrative, system-by-system walkthrough of how these fit together and this plugin's own internal conventions — this README stays the exhaustive per-class reference.

## When NOT to Use It / Scope Boundaries

- Game-specific win conditions, scoring rules, and turn-driven piece placement logic belong in the consuming game module (or in `UnrealTurnBasedMechanics` for turn/action orchestration), not here — this plugin should only ever know about grids, tiles, and pieces in the abstract.
- Visual/material/VFX specifics for tiles and pieces belong in the consuming project's Blueprint/asset layer; this plugin's C++ classes should stay presentation-agnostic beyond thin hooks (e.g. `PlayShiftAnimation`).
- New AI heuristics or opponent decision-making do not belong here — the grid-connection analysis functions in `UGridMechanics_GridLibrary` are pure board-state math, not AI; actual AI logic belongs in a dedicated intelligence module/plugin.
- Turn sequencing, action stacks, and match state machines belong in `UnrealTurnBasedMechanics`, not here — this plugin has no concept of "turns."
- `UBoardStateComponentBase` is explicitly marked (via its own TODO) as transitional scaffolding intended to eventually move into a consuming game module and be renamed — new generic board-state work should be aware this class's home is not final.

## Notable Design Patterns

- Explicit-injection observer pattern for board state: `UBoardStateInterpreter` subclasses are bound to a `UBoardStateComponentBase` by project wiring code, rather than discovering it automatically. See [Systems.md](Systems.md#board-state) for how the replication + interpreter pattern actually works end to end.
- Subsystem-based discovery/registration: `UGridWorldSubsystem` (and the largely-overlapping `UGridTrackerSubsystem`) provide per-world grid tile/piece registries and hover tracking. See [Systems.md](Systems.md#known-rough-edges) for how the two relate.
- Interface-based deprecation: `IGridTileSelector` supersedes `UGridTileSelectorComponent`, which is marked deprecated via its own `DeprecatedFunction` meta pointing back at the interface. See [Systems.md](Systems.md#tile-selection) for the interface-vs-component pairing.
- Pure-function libraries (`UBlueprintFunctionLibrary`) separate grid math from actor/state concerns, keeping shift, connection, and shape logic independently testable and Blueprint-accessible. See [Systems.md](Systems.md#board-shifting) for how the shift system's compute/animate split works.
- UFUNCTION `Category` strings are inconsistent across the plugin (e.g. `"Grid Mechanics | Library | Direction"`, `"Grid|Shift"`, `"Grid|Registry"`, `"Turn Based | Grid"`, `"Grid Mechanics|Cursor"` — mixed spacing/style). See the project-level [naming conventions notes](../../../Source/ConnectIt/Docs/Conventions.md) and [Systems.md](Systems.md#conventions) for this plugin's own fuller convention notes.

## Classes

### Board

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `UBoardStateComponentBase` (+`FTileData`) | `UActorComponent` (Abstract) | Replicated board-state container; owns a registry of `UBoardStateInterpreter`s and broadcasts `OnBoardStateChanged`. `FTileData` holds generic per-tile payload (faction owner, multiplier, active flag). | Has a TODO to move into a consuming game module and rename — transitional scaffolding. |
| `IBoardStateInterface` | Native interface | Generic observer bind/unbind contract (`BindObserver`/`UnbindObserver`). | Appears unused/orphaned — no implementers found; also has an incorrect dead forward declaration (`class FOnBoardStateChanged;`, which is a delegate macro type, not a class). See [Duplication.md](../../../Source/ConnectIt/Docs/Duplication.md). |
| `UBoardStateInterpreter` | `UActorComponent` (Abstract) | Explicit-injection observer: `BindToBoardStateComponent` wires it up, subclasses override `OnBoardStateChanged_Implementation` to react. | Uses `ClassGroup=(Chimera)` and a "Lives in Chimera Framework" doc comment — naming residue from a predecessor project. See [Conventions.md](../../../Source/ConnectIt/Docs/Conventions.md#discrepancies). |
| `UBoardShiftComponent` | `UActorComponent` | Computes and animates a row/column "shift" with wraparound; `ComputeShift` is pure/synchronous, `PlayShiftAnimation` is presentation-only. | A project-specific shift component in the current consuming project absorbed this component's responsibilities (per its own doc comment, since the abstraction had only one consumer); this class may now be unused by that consumer. |
| `UGridMechanics_GridShiftLibrary` | `UBlueprintFunctionLibrary` | Pure math for shift operations: `ComputeShiftResult`, `ComputeShiftedPosition`, `IsWrapping`, `BuildShiftInstructions`. | |
| `EShiftAxis`, `EShiftDirection`, `FShiftOperation`, `FShiftResult`, `FTileShiftInstruction` | Enums/structs | Pure-data description of a shift operation, its computed remap, and per-tile animation instructions. | |

### Cursor

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `AGridCursorManagerBase` | `AActor` (Abstract) | Binds to `UGridWorldSubsystem::OnGridTileHoverChanged`; subclasses implement `UpdateCursor` for the visual. Owns enabled/paused state. | |
| `AGridCursorSimpleBase` | `AGridCursorManagerBase` | Reference implementation — moves a `UStaticMeshComponent` to the hovered tile. | |

### Base Types (top-level Public/)

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `EGridDirection` (+operators) | Enum | 8-way compass direction with wraparound arithmetic operators. | Actual operator set is `+` (two overloads: `EGridDirection`+`EGridDirection`, and `EGridDirection`+`int32`), `-`, and `-=`; there is no `operator+=`. |
| `FGridDirectionVector`, `FGridPosition` (+`GetTypeHash`), `FGridPositionArray`, `FGridMovement` (+hash), `FGridPair` | Structs | Core coordinate/movement value types with full operator overload sets. | `FGridPosition::GetDistance` uses Chebyshev distance with a `// FOR NOW` comment flagging it as a placeholder metric. |
| `UGridMechanics_GridLibrary` | `UBlueprintFunctionLibrary` | Direction↔rotation conversion, grid-position math, neighbor traversal, and N-in-a-row connection analysis (`CountValidWindows`, `CountPossibleGridConnections`, `GetPotentialConnectionCountPerDirection`, `CreateConnectionsFloodMap`). | This is the N-in-a-row scoring engine a consuming project's board rules can build on. |
| `UGridMechanics_ShapeLibrary` (+`FShapeConfiguration`) | `UBlueprintFunctionLibrary` | Line/shape detection: `GetLongestLine(s)`, `GetLinesOfLength`, `IsSquare`. | |

### Interface

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `IGridFactionInterface` | Native interface | Faction tag/id get/set contract. | |
| `IGridLevelInterface` | Native interface | Generic "level"/stack-height get/set/increment/decrement contract. | |
| `IGridPieceHandler` | Native interface | Has/get/set/remove piece contract for tiles. | |
| `IGridShiftInterface` | Native interface | `PrepareShift`/`ApplyShiftAlpha`/`OnShiftComplete` — per-actor counterpart consumed by `UBoardShiftComponent`. | |
| `IGridTileHandler` | Native interface | `GetGridPosition()` contract. | |

### Piece

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `AGridPieceBase` | `AActor` | Non-GAS piece actor; holds `FGridPosition GridPosition`. | Base class for the current consuming project's own piece actor subclass. |
| `UGridPieceRegistryComponent` | `UActorComponent` | `GetPiece(Position)` lookup via cached `UGridWorldSubsystem`/`UGridTileRegistryComponent`. | |
| `AGridUnitBase` | `AGridPieceBase` + `IAbilitySystemInterface` | GAS-enabled piece/unit; owns an `UAbilitySystemComponent`; `GetFactionTag`/`FaceGridDirection` are BlueprintImplementableEvents. | Not currently used by the current consuming project, which uses the simpler `AGridPieceBase` instead. |

### Subsystem

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `UGridTrackerSubsystem` | `UWorldSubsystem` | Tracks all grid tiles/units plus the currently-hovered tile; `OnGridTileHoveredStart/Stop` delegates. | Has a fully commented-out "Unit hover" section (dead code). Overlaps significantly with `UGridWorldSubsystem` below — only `GridWorldSubsystem` is actually consumed elsewhere in the codebase. See [Duplication.md](../../../Source/ConnectIt/Docs/Duplication.md). |
| `UGridWorldSubsystem` | `UWorldSubsystem` | Registers tiles/pieces and exposes `OnGridTileHoverChanged`. | The de facto canonical subsystem — bound to by `AGridCursorManagerBase` and consumed elsewhere in the codebase. |

### Tile

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `AGridTileBase` | `AActor` | Base tile actor; `SendGameplayTag` BlueprintNativeEvent, `OnGridTileBeginCursorOver` delegate. | Has its own cursor-over delegate distinct from `UGridTileComponent`'s — see Notes below. |
| `UGridTileComponent` | `UActorComponent` | Component variant of the cursor-over concept (`OnGridTileBeginCursorOver`/`OnGridTileEndCursorOver`). | Header comment states "anything that is a grid tile will be required to have this component," yet `AGridTileBase` duplicates the same delegate concept independently. See [Duplication.md](../../../Source/ConnectIt/Docs/Duplication.md). |
| `UGridTileRegistryComponent` | `UActorComponent` | Authoritative row/column/position↔world conversion and board-dimension query API (Min/Max Row/Column, GetRow/GetColumn, GetTileAtPosition, etc.), derived from registered tiles with no hardcoded dimensions. Editor-only `ValidateTileAlignment()` under `WITH_EDITOR`. | In the current consuming project, this is the class its board manager exposes for lookup; every position↔tile resolution there goes through it. **Known bug**: `ValidateTileAlignment()`'s entire body is commented out — it currently does nothing. Its own TODO explains why: it relies on tile actors having a `GridPosition` set in-editor, which isn't happening yet, so the check would always report every tile misaligned. See [Systems.md](Systems.md#known-rough-edges). |
| `IGridTileSelector` (+2 delegate types) | Native interface | Dual-pattern selector contract: Blueprint-friendly dynamic delegate register/unregister, and C++-friendly direct multicast binding (`BindOnGridTileSelected`/`GetOnGridTileSelectedDelegate`). | Has an explicitly `[Deprecated]` `GetGridTileSelectorComponent()` pointing at `UGridTileSelectorComponent`. |
| `UGridTileSelectorComponent` | `UActorComponent` | Simpler component exposing one dynamic multicast delegate. | Being phased out per the interface's own deprecation note; the interface-based pattern above is the intended future direction. See [Duplication.md](../../../Source/ConnectIt/Docs/Duplication.md). |

### Module (UnrealGridMechanicsEditor)

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `UGridValidatorBase` | `UEditorValidatorBase` | Editor-time asset validation hook for grid content (`CanValidateAsset_Implementation`, `ValidateLoadedAsset_Implementation`). | **Known bug**: `GridValidatorBase.cpp:26` has a TODO noting the loop never actually sets `BoardActor`, so the validator's core logic is currently non-functional. |
| `FUnrealGridMechanicsEditorModule` | `IModuleInterface` | Editor module entry point. | |
| `FUnrealGridMechanicsModule` | `IModuleInterface` | Runtime module entry point. | |

## Known Discrepancies / Issues in This Plugin

- `IBoardStateInterface` is unused/orphaned and has an incorrect dead forward declaration. See [Duplication.md](../../../Source/ConnectIt/Docs/Duplication.md).
- `UBoardStateInterpreter`'s "Chimera Framework" naming residue from a predecessor project. See [Conventions.md](../../../Source/ConnectIt/Docs/Conventions.md#discrepancies).
- `UGridTrackerSubsystem` vs `UGridWorldSubsystem` overlap — only the latter is actually consumed. See [Duplication.md](../../../Source/ConnectIt/Docs/Duplication.md).
- `AGridTileBase` vs `UGridTileComponent` cursor-over delegate duplication. See [Duplication.md](../../../Source/ConnectIt/Docs/Duplication.md).
- `IGridTileSelector` vs `UGridTileSelectorComponent` deprecation pairing. See [Duplication.md](../../../Source/ConnectIt/Docs/Duplication.md).
- `UGridValidatorBase`'s non-functional `BoardActor` assignment loop (`GridValidatorBase.cpp:26`) — a real bug, not just a stylistic discrepancy.
- `UGridTileRegistryComponent::ValidateTileAlignment()`'s body is entirely commented out — a second, editor-side non-functional validation check alongside `UGridValidatorBase`'s bug above, both blocked on tile actors not yet being given a `GridPosition` in-editor. See [Systems.md](Systems.md#known-rough-edges).
- `UnrealTurnBasedMechanics.Build.cs` redundantly lists `UnrealGridMechanics` in both Public and Private dependency lists. See [Duplication.md](../../../Source/ConnectIt/Docs/Duplication.md).
- `EGridDirection`'s operator set is `+`/`-`/`-=`, not `+`/`-`/`+=`/`-=` — there is no `operator+=`. See [Systems.md](Systems.md#conventions).
- UFUNCTION `Category` string inconsistency across the plugin. See the project-level [naming conventions notes](../../../Source/ConnectIt/Docs/Conventions.md) and [Systems.md](Systems.md#conventions) for this plugin's own verbatim examples.
