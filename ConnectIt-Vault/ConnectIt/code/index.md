---
schema: code
reconciled: 2026-09-07
commit: 668872e
---

# game (ConnectIt module) — code index

Per-type help pages for the `ConnectIt` game module (`Source/ConnectIt/`). Governed by
[[_schema/code|_schema/code.md]]. An **overlay** on the module's own (extensive) in-repo
docs: [`old/Source/ConnectIt/Docs/README.md`](../../../old/Source/ConnectIt/Docs/README.md),
[`old/Source/ConnectIt/Docs/Conventions.md`](../../../old/Source/ConnectIt/Docs/Conventions.md),
[`old/Source/ConnectIt/Docs/RuntimeStateAccess.md`](../../../old/Source/ConnectIt/Docs/RuntimeStateAccess.md),
[`old/Source/ConnectIt/Docs/UIValueCatalogue.md`](../../../old/Source/ConnectIt/Docs/UIValueCatalogue.md),
[`old/Source/ConnectIt/Docs/Duplication.md`](../../../old/Source/ConnectIt/Docs/Duplication.md),
[`old/Source/ConnectIt/Docs/LegacyPipeline.md`](../../../old/Source/ConnectIt/Docs/LegacyPipeline.md), and
[`old/Source/ConnectIt/Docs/Workflows/`](../../../old/Source/ConnectIt/Docs/Workflows/) (six workflow docs).

Source root: `Source/ConnectIt/`. Anchor commit `668872e` = *Board Manager Refactor*, the
last commit to touch this module — the game is mid–"board architecture overhaul": the
`AConnectIt_BoardManager` actor is retired, replaced by server-only UObjects on
`AConnectIt_GameMode`; several game-event bodies are commented out pending rewiring.
`status`: **current** = written & reconciled · **stub** = no page yet.

| Type | Kind | Area | Role | Source (`Public/…`) | Status |
|---|---|---|---|---|---|
| [[AConnectIt_GameMode\|AConnectIt_GameMode]] (`EConnectItMatchType`) | UCLASS (AGameMode) | Framework/GameMode | primary (server-only owner) | `Framework/GameMode/ConnectIt_GameMode.{h,cpp}` | current |
| [[AConnectIt_GameState\|AConnectIt_GameState]] (`FConnectItMatchResult`, `EMatchEndReason`) | UCLASS (AGameState) | Framework/GameState | primary | `Framework/GameState/ConnectIt_GameState.{h,cpp}` | current |
| [[UConnectIt_BoardStateComponent\|UConnectIt_BoardStateComponent]] | UCLASS (component) | Board | primary (single source of truth) | `Board/ConnectIt_BoardStateComponent.{h,cpp}` | current |
| [[UConnectIt_BoardRequestMediator\|UConnectIt_BoardRequestMediator]] | UCLASS (UObject) | Board | primary (server-only) | `Board/ConnectIt_BoardRequestMediator.{h,cpp}` | current |
| [[UConnectIt_BoardRules\|UConnectIt_BoardRules]] (`IConnectIt_ScoringRule`, `IConnectIt_WinCondition`, `UConnectIt_LineScoringRule`, `UConnectIt_ScoreThresholdWinCondition`) | UCLASS / UINTERFACE | Board/Rules | primary (pluggable strategies) | `Board/Rules/*.{h,cpp}` | current |
| [[ConnectItStructs\|ConnectItStructs]] (`FConnectItBoardState`, `FConnectItTileData`, `FConnectItBoardChangeEvent`, `FConnectItBoardStateSnapshot`, `FConnectItRequest*` payloads) | USTRUCT ×~12 | Top-level | primary (data) | `ConnectIt_Structs.h` | current |
| [[UConnectIt_State_Game\|UConnectIt_State_Game]] (`UConnectIt_State_Base`, `…_SelectTile`, `…_PlacePiece`, `…_UpdateGameBoard`) | UCLASS | Framework/Game State Machine | primary | `Framework/Game State Machine/*.{h,cpp}` | current |
| [[UConnectIt_GameFacade\|UConnectIt_GameFacade]] (`UConnectIt_GameViewModel`) | UCLASS (UObject) | Framework/Data | primary (read / write model) | `Framework/Data/ConnectIt_GameFacade.{h,cpp}`, `.../ConnectIt_GameViewModel.{h,cpp}` | current |
| AConnectIt_PlayerController | UCLASS (APlayerController) | Framework/Controller | primary | `Framework/Controller/ConnectIt_PlayerController.{h,cpp}` | stub — covered in systems/place-piece-request |
| AConnectIt_AIController | UCLASS (AAIController) | Framework/Controller | primary | `Framework/Controller/ConnectIt_AIController.{h,cpp}` | stub |
| AConnectIt_GameMode_Play | UCLASS | Framework/GameMode | primary | `Framework/GameMode/ConnectIt_GameMode_Play.h` | stub |
| AConnectIt_PlayerState | UCLASS | Framework/PlayerState | primary | `Framework/PlayerState/ConnectIt_PlayerState.h` | stub |
| UConnectIt_PlacePieceAction / UConnectIt_DefaultViewerAction | UCLASS (`UTurnBasedAction` / `…SpectatorAction`) | Action | primary | `Action/ConnectIt_PlacePieceAction.{h,cpp}`, `Action/ConnectIt_DefaultViewerAction.h` | stub — covered in systems/place-piece-request |
| UConnectIt_PlacePieceGameEvent / UConnectIt_LineScoreGameEvent | UCLASS (`UTurnBasedGameEvent`) | GameEvent | primary (mid-rewrite — bodies stubbed) | `GameEvent/ConnectIt_*GameEvent.{h,cpp}` | stub |
| UConnectIt_PieceRegistryComponent | UCLASS (component) | Board | primary | `Board/ConnectIt_PieceRegistryComponent.{h,cpp}` | stub |
| UConnectIt_BlackboardSubsystem | UCLASS (UWorldSubsystem) | Framework/Subsystem | primary (turn modifiers) | `Framework/Subsystem/ConnectIt_BlackboardSubsystem.{h,cpp}` | stub |
| UConnectIt_LevelConfigDataAsset / UConnectIt_LevelConfigSettings / UConnectIt_PlayerData | UCLASS (UDataAsset / config / UObject) | Framework/Data | primary (config) | `Framework/Data/ConnectIt_LevelConfig*.h`, `.../ConnectIt_PlayerData.h` | stub |
| Libraries (`UConnectIt_BoardStateLibrary`, `…_GameRulesLibrary`, `…_GameUtilityLibrary`, `…_GridFloodLibrary`, `…_GridRulesLibrary`) | UCLASS (UBlueprintFunctionLibrary) | Library | primary | `Library/ConnectIt_*Library.{h,cpp}` | stub |
| AConnectIt_GridPiece | UCLASS (`AGridPieceBase`) | Grid | primary | `Grid/ConnectIt_GridPiece.{h,cpp}` | stub |
| Influence map (`UPieceControlInfluenceMap`, `UPieceControlMapVisualizer`) | UCLASS | GameIntelligence/InfluenceMap | primary | `GameIntelligence/InfluenceMap/*.{h,cpp}` | stub |
| MinMax (`UConnectIt_MinMaxManager`, `ConnectIt_MinMaxTreeBuilder`, `ConcreteMinMaxExample`) | UCLASS / templates | MinMax | primary (game's own AI search) | `MinMax/*.{h,cpp}` | stub |
| UI (`UConnectIt_DebugStateWidget`, `UDWidget_ConnectIt_BoardStateComponent`, `UDWidget_ConnectIt_GameState`) | UCLASS (UUserWidget / `UDWidgetBase`) | UI | internal (tooling) | `UI/*.{h,cpp}` | stub |
| Interfaces (`IConnectIt_FacadeHandlerInterface`, `IConnectIt_GameStateHandlerInterface`) | UINTERFACE | Framework/Interface | primary | `Framework/Interface/*.h` | stub |
| GameplayTags (`ConnectIt_GameplayTags.h`) | native tag decls | Top-level | primary | `ConnectIt_GameplayTags.h` | stub |

## Related

- Flows: [[ConnectIt/systems/index|systems/index.md]]
- Recipes: [[ConnectIt/recipes/index|recipes/index.md]]
- Suite context: [[architecture/overview|architecture/overview.md]]
