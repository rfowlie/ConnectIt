---
schema: code
---

# ConnectIt (game module) — code index

Per-type help pages for the `ConnectIt` game module (`Source/ConnectIt/`). Governed by
[[_core/_schema/_code|_core/_schema/_code.md]]. This is the **overlay** — short "what /
why / what changed" notes over the code. The exhaustive per-member reference is the
code itself (`../../Source/ConnectIt/`).

Source root: `Source/ConnectIt/`. Each page carries its own `commit:` anchor; this index
has none of its own (`/process-code` checks the pages).

**The shape today (2026-10-23):**

- **Server authority:** [[AConnectIt_GameMode|AConnectIt_GameMode]] owns the match's
  rule set (`FConnectItRuleSet`: plain, thread-safe structs) and the
  [[UConnectIt_BoardRequestMediator|board request mediator]].
- **Operations are the requests:** a board change is a board operation struct. A player
  action, the Level director or a tile power sends it; the server checks and applies it.
- **Every change resolves in one chain** (tile powers, then scoring, until quiet) and is
  committed as one replicated snapshot: [[UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]].
- **Visuals play the change's self-applying events** one at a time, and read a display
  board.
- **Three directors** (Level, Player, AI) script the match and hold its goals.
- **Tiles carry powers** defined once per tag in a tile catalog ([[tile-power-chain]]).

`status`: **current** = written & reconciled · **stub** = listed, no page yet · **stale**
= a source path is gone · **retired** = the type was removed, page kept as history.

## Inventory

| Type | Kind | Area | Role | Source (`Public/…`) | Status |
|---|---|---|---|---|---|
| [[AConnectIt_GameMode\|AConnectIt_GameMode]] (`EConnectItMatchType`) | UCLASS (AGameMode) | Framework/GameMode | primary (server-only owner) | `Framework/GameMode/ConnectIt_GameMode.{h,cpp}` | current |
| [[AConnectIt_GameState\|AConnectIt_GameState]] (`FConnectItMatchResult`, `EMatchEndReason`) | UCLASS (AGameState) | Framework/GameState | primary | `Framework/GameState/ConnectIt_GameState.{h,cpp}` | current |
| [[UConnectIt_BoardStateComponent\|UConnectIt_BoardStateComponent]] | UCLASS (component) | Board | primary (single source of truth) | `Board/ConnectIt_BoardStateComponent.{h,cpp}` | current |
| [[UConnectIt_BoardRequestMediator\|UConnectIt_BoardRequestMediator]] | UCLASS (UObject) | Board | primary (server-only) | `Board/ConnectIt_BoardRequestMediator.{h,cpp}` | current |
| [[UConnectIt_BoardRules\|UConnectIt_BoardRules]] (`IConnectIt_ScoringRule`, `IConnectIt_WinCondition`, `UConnectIt_LineScoringRule`, `UConnectIt_ScoreThresholdWinCondition`) | UCLASS / UINTERFACE | Board/Rules | primary (pluggable strategies) | `Board/Rules/*.{h,cpp}` | retired |
| [[UConnectIt_BoardRegistrySubsystem\|UConnectIt_BoardRegistrySubsystem]] (`UConnectIt_TileRegistry`, `UConnectIt_PieceRegistry`) | UCLASS (UWorldSubsystem) | Framework/Subsystem, Board | primary (per-world registry owner) | `Framework/Subsystem/ConnectIt_BoardRegistrySubsystem.{h,cpp}`, `Board/ConnectIt_TileRegistry.{h,cpp}`, `Board/ConnectIt_PieceRegistry.{h,cpp}` | current |
| [[ConnectItStructs\|ConnectItStructs]] (`FConnectItBoardState`, `FConnectItTileData`, `FConnectItBoardChangeEvent`, `FConnectItBoardStateSnapshot`) | USTRUCT | Top-level | primary (data) | `ConnectIt_Structs.h` | current |
| [[UConnectIt_State_Game\|UConnectIt_State_Game]] (`UConnectIt_State_Base`, `…_SelectTile`, `…_PlacePiece`, `…_UpdateGameBoard`) | UCLASS | Framework/Game State Machine | **removed 2026-10-23** (legacy MVVM) | `Framework/Game State Machine/*.{h,cpp}` (gone) | retired |
| [[UConnectIt_GameFacade\|UConnectIt_GameFacade]] (`UConnectIt_GameViewModel`) | UCLASS (UObject) | Framework/Data | **removed 2026-10-23** (legacy MVVM) | `Framework/Data/ConnectIt_GameFacade.{h,cpp}`, `.../ConnectIt_GameViewModel.{h,cpp}` (gone) | retired |
| AConnectIt_PlayerController | UCLASS (APlayerController) | Framework/Controller | primary | `Framework/Controller/ConnectIt_PlayerController.{h,cpp}` | stub — covered in systems/place-piece-request |
| AConnectIt_AIController | UCLASS (AAIController) | Framework/Controller | primary | `Framework/Controller/ConnectIt_AIController.{h,cpp}` | stub |
| AConnectIt_PlayerState | UCLASS | Framework/PlayerState | primary | `Framework/PlayerState/ConnectIt_PlayerState.h` | stub |
| UConnectIt_PlacePieceAction / UConnectIt_DefaultViewerAction | UCLASS (`UTurnBasedAction` / `…SpectatorAction`) | Action | primary | `Action/ConnectIt_PlacePieceAction.{h,cpp}`, `Action/ConnectIt_DefaultViewerAction.h` | stub — covered in systems/place-piece-request |
| UConnectIt_LevelConfigDataAsset | UCLASS (UDataAsset) | Framework/Data | primary (config: the board) | `Framework/Data/ConnectIt_LevelConfigDataAsset.h` | stub |
| Libraries (`UConnectIt_BoardStateLibrary`, `…_BoardEventLibrary`, `…_GameUtilityLibrary`, `…_LayoutToolLibrary`) | UCLASS (UBlueprintFunctionLibrary) | Framework/Library | primary | `Framework/Library/ConnectIt_*Library.{h,cpp}` | stub |
| AConnectIt_PieceBase (was `AConnectIt_GridPiece`) | UCLASS (`AGridPieceBase`) | Board/Actor | primary | `Board/Actor/ConnectIt_PieceBase.{h,cpp}` | stub |
| AConnectIt_TileBase | UCLASS (`AGridTileBase`) | Board/Actor | primary (every ConnectIt tile; the tile hooks) | `Board/Actor/ConnectIt_TileBase.{h,cpp}` | stub — covered in recipes/add-a-tile-power |
| AConnectIt_BuilderTile | UCLASS (`AConnectIt_TileBase`, `IConnectIt_TileDataProvider`) | Board/Actor | primary (level blockout; swapped for catalog tiles) | `Board/Actor/ConnectIt_BuilderTile.{h,cpp}` | stub — covered in recipes/add-a-tile-power, recipes/lock-a-tile-or-chain-tiles |
| Tile power (`FConnectItTilePower`, `FConnectItTilePowerEffect`, `FConnectItTilePowerContext`, `EConnectItPowerTrigger`) | USTRUCT / UENUM | Board/Powers | primary (data) | `Board/Powers/ConnectIt_TilePower.h` | stub — covered in systems/tile-power-chain |
| Tile power effects (`…_Push`, `…_Vacuum`, `…_Repel`, `…_Chained`) | USTRUCT ×4 | Board/Powers | primary | `Board/Powers/ConnectIt_TilePowerEffects.h` | stub — covered in recipes/add-a-tile-power |
| UConnectIt_TileCatalog (`FConnectItTileCatalogEntry`) | UCLASS (UDataAsset) | Board/Powers | primary (config) | `Board/Powers/ConnectIt_TileCatalog.h` | stub — covered in recipes/add-a-tile-power |
| FConnectItPowerChain | class (plain C++) | Board/Powers | internal (`Private/`) | `Private/Board/Powers/ConnectIt_PowerChain.h` | stub — covered in systems/tile-power-chain |
| UConnectIt_TileVisualizer / UConnectIt_TileReaction | UCLASS (UObject) | Board | primary (tile hooks, every machine) | `Board/ConnectIt_TileVisualizer.h`, `Board/ConnectIt_TileReaction.h` | stub — covered in systems/tile-power-chain |
| MinMax (`FConnectItMinMaxRules`, `FConnectItMinMaxGeometry`, the evaluation / ordering weights) | class / USTRUCT | MinMax | primary (the game's rules for the AI search; the search itself is `AI/ConnectIt_MinMaxSearch.h`) | `MinMax/*.{h,cpp}` | stub — covered in systems/ai-turn |
| UI (`UConnectIt_DebugStateWidget`, `UDWidget_ConnectIt_BoardStateComponent`, `UDWidget_ConnectIt_GameState`) | UCLASS (UUserWidget / `UDWidgetBase`) | UI | internal (tooling) | `UI/*.{h,cpp}` | stub |
| UConnectIt_MapNodeDataAsset (`FConnectItNodeGate`, `FConnectItNodeRewards`, `EConnectItActionDesign`) | UCLASS (UDataAsset) | Adventure | primary (an Adventure hub node) | `Adventure/ConnectIt_MapNodeDataAsset.{h,cpp}` | stub — see design/adventure-hub-plan |
| UConnectIt_ActionShardTable (`FConnectItActionShardCosts`) | UCLASS (UDataAsset) | Adventure | primary (shards → permanent uses) | `Adventure/ConnectIt_ActionShardTable.{h,cpp}` | stub — see design/adventure-hub-plan |
| FConnectItAdventureProgress / FConnectItNodeRequirementCheck | USTRUCT | Adventure | primary (data: what the save holds) | `Adventure/ConnectIt_AdventureProgress.{h,cpp}` | stub — see design/adventure-hub-plan |
| GameplayTags (`ConnectIt_GameplayTags.h`) | native tag decls | Top-level | primary | `ConnectIt_GameplayTags.h` | stub |

## Map

The suite, seen from the game domain. ConnectIt is a **layered stack of game-agnostic
plugins with the game module on top** — each plugin is reusable in another project and
knows nothing about ConnectIt's rules. The game module (`Source/ConnectIt/`) is the only
place game-specific logic lives.

### Dependency graph

```
                 ConnectIt  (game module — Source/ConnectIt)
                    |
   +----------------+----------------+----------------+
   |                |                |                |
UnrealTurnBasedMechanics   UnrealGameIntelligence   UnrealCodingUtils
   |        |                (engine-only)            (engine-only)
   |        +--------> UnrealGridMechanics
   |                        |
   +------------------------+--------> UnrealGameMechanics  (base)
                                          ^
                                          |  (EnhancedInput)

UnrealAIMechanics  — standalone, DORMANT (not enabled)
UnrealUIMechanics  — standalone, STUB   (not enabled)
AdvancedSessions-5-5 — vendored third-party, not integrated
```

- **[[UnrealGameMechanics/CLAUDE|UnrealGameMechanics]]** — base layer: the gated
  phase-barrier / tag queue (`UGameEventTaskSubsystem`), turn bookkeeping, actor pooling,
  Enhanced-Input tag binder, scoring / state-machine helpers, `UDWidgetBase`.
- **[[UnrealGridMechanics/CLAUDE|UnrealGridMechanics]]** — depends on GameMechanics; adds
  the 2D board: coords + directions, grid/shape math, tile/piece actors, registries,
  hover subsystems, replicated board state, row/col shift, editor validator.
- **[[UnrealTurnBasedMechanics/CLAUDE|UnrealTurnBasedMechanics]]** — depends on both;
  adds the replicated match: per-controller action stack,
  `UTurnBasedParticipantManagerComponent` state machine (ready-check, timers,
  forfeit/reconnect), pluggable turn order, `ATurnBased*` framework bases. Its turn-end
  sequence routes through GameMechanics' `UGameEventTaskSubsystem`.
- **[[UnrealGameIntelligence/CLAUDE|UnrealGameIntelligence]]**,
  **[[UnrealCodingUtils/CLAUDE|UnrealCodingUtils]]** — standalone, engine-only deps.
  ConnectIt uses GameIntelligence's **search**: its MinMax alpha-beta template
  (`GI_MinMaxAlphaBeta`, from `MinMax/ConnectIt_MinMaxRules.h`) and the async search
  (`GI_SearchAsync`, from `AI/ConnectIt_MinMaxSearch.h`), with the game's own rules. (Its
  own influence map was deleted 2026-10-23.)
- **[[UnrealAIMechanics/CLAUDE|UnrealAIMechanics]]**,
  **[[UnrealUIMechanics/CLAUDE|UnrealUIMechanics]]** — not enabled in `ConnectIt.uproject`
  (dormant / stub). `AdvancedSessions-5-5` — vendored, its `OnlineSubsystem` dependency
  commented out pending online-session work.

### Where to start reading

1. **Board authority** — [[AConnectIt_GameMode|AConnectIt_GameMode]] owns
   [[UConnectIt_BoardRequestMediator|UConnectIt_BoardRequestMediator]] + the rule set
   (`FConnectItRuleSet`, server-only; it replaced the retired
   [[UConnectIt_BoardRules|UConnectIt_BoardRules]]). Players' changes enter through
   `AConnectIt_GameMode::ProcessBoardRequest`, the level's through its level changes; what
   follows each change is [[tile-power-chain]].
2. **Board truth** — [[UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]] on
   the GameState holds one replicated snapshot (prev + current + change event) that every
   visual system converges on: [[board-state-single-source-of-truth|systems/board-state-single-source-of-truth]].
3. **A turn** — [[place-piece-request|systems/place-piece-request]]: player picks a tile →
   server validates → board state commits → gated visual sequence → outcome back to client.
4. **Registries** — [[UConnectIt_BoardRegistrySubsystem|UConnectIt_BoardRegistrySubsystem]]:
   one per world, `DuplicateObject`'d at `OnWorldBeginPlay` from `Instanced` templates on
   `UConnectIt_LevelConfigDataAsset`.
5. **Cross-cutting conventions** — gameplay tags drive sequencing
   (`../../Config/DefaultGameplayTags.ini`); `UnrealTurnBasedMechanics` carries
   `FInstancedStruct Payload` on its request/modifier structs so the game module can
   extend turn data without changing the plugin; no `Client_`/`Multicast_` RPCs exist in
   `Source/ConnectIt` — every "tell everyone" need is a replicated-property change
   instead. See [[ConnectIt/_decisions/2026-09-06-suite-conventions|decisions/2026-09-06-suite-conventions]].

### Removed 2026-10-23 (dead code pass)

Deleted with their Blueprint assets; don't reintroduce them:

- **The legacy MVVM pipeline:** `UConnectIt_State_Game` and its four sub-states,
  `UConnectIt_GameFacade` / `UConnectIt_GameViewModel`, `UConnectIt_PlayerData`,
  `AConnectIt_GameMode_Play`, the two `IConnectIt_*Handler` interfaces, and
  `UConnectIt_GridRulesLibrary`. Pages kept as history:
  [[UConnectIt_State_Game|UConnectIt_State_Game]], [[UConnectIt_GameFacade|UConnectIt_GameFacade]],
  [[game-state-machine|systems/game-state-machine]].
- **The influence map** (`UPieceControlInfluenceMap`, `UPieceControlMapVisualizer`) and
  `FConnectItBoardState::FactionTurn`.
- **Unused classes:** `UConnectIt_GameRulesLibrary`, `UConnectIt_GridFloodLibrary`,
  `UConnectIt_ActionRequestMediator`, `UConnectIt_PieceRegistryComponent`,
  `UConnectIt_BlackboardSubsystem`.
- **The old game events:** `UConnectIt_PlacePieceGameEvent`, `UConnectIt_ScoreGameEvent`
  (and its CoreRedirect), and the Mediator's unfinished game-event queue.

Earlier: `AConnectIt_BoardManager` and its subsystem, the old ConnectIt shift pipeline,
the tag-reactive interpreter, `UConnectIt_BoardRules` ([[UConnectIt_BoardRules|retired page]]),
and the two old MinMax implementations. The full list:
[the 2026-10-23-1303 log](../_logs/2026-10-23-1303.md).

## Known issues

Reviewed against the code 2026-10-23. Fixed items stay listed briefly so they aren't
re-reported.

### Open

- **`CI_AIController_Play` loads with a warning:** its `ActionsComponent` is of a class
  that no longer exists (`ConnectIt_AIActionsComponent`). Seen in every editor log of
  2026-10-23, before the dead code pass. Fix in the editor: re-save the Blueprint, or
  remove the stale component.
- **Parked plugin:** `UnrealUIMechanics` has no classes and isn't in `ConnectIt.uproject`.
  Kept on purpose as the future home for UI (owner, 2026-10-23).
- See also [[UnrealGridMechanics/CLAUDE|UnrealGridMechanics]] /
  [[UnrealGameMechanics/CLAUDE|UnrealGameMechanics]] rough edges for the plugin-side
  overlap entries (`UGridTrackerSubsystem` vs `UGridHoverSubsystem`;
  `UGameMechanicsStateBase` vs `…Simple`; `IGridTileSelector` deprecating
  `UGridTileSelectorComponent`; dead commented-out blocks).

### Fixed

- **FactionID spoof** -- the server takes the requester's faction from its own
  PlayerState slot; a request carries no faction
  ([payload is the request, server supplies faction](../_decisions/2026-10-07-payload-is-the-request-server-supplies-faction.md)).
- **Two parallel MinMax implementations** -- both removed; the AI decides through its
  strategies, MinMax being one (`MinMax/ConnectIt_MinMaxRules`).
- **Piece actors replicating** (2026-10-23) -- `AConnectIt_PieceBase` (was
  `AConnectIt_GridPiece`) is local to each machine, like tile actors: every machine takes
  its pieces from its own pool as board events play. It replicated `FactionID` /
  `OccupiedPosition`, so a piece the server pooled could show up on clients beside their
  own.
- **Unprefixed classes** -- the influence-map classes were the only two without
  `ConnectIt_`; deleted 2026-10-23.
- **Influence map reading a field nothing set** (`FactionTurn`) -- both deleted 2026-10-23.
- **`UConnectIt_GameRulesLibrary` unused** -- deleted 2026-10-23.

## Related

- Reference: [[ConnectIt/code/runtime-state|runtime-state.md]] — which accessor to call to
  read live board / turn / action / sequencing state
- Flows: [[ConnectIt/code/systems/__INDEX|systems/__INDEX.md]]
- Recipes: [[ConnectIt/code/recipes/__INDEX|recipes/__INDEX.md]]
- Decisions: [[ConnectIt/_decisions/__INDEX|decisions/__INDEX.md]]
- Domain router: [[ConnectIt/CLAUDE|ConnectIt/CLAUDE.md]]
