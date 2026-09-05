# ConnectIt Module — Class Catalogue

`Source/ConnectIt/` is the game module — the project-specific layer that consumes the six generic plugins ([UnrealGridMechanics](../../../Plugins/UnrealGridMechanics/Docs/README.md), [UnrealTurnBasedMechanics](../../../Plugins/UnrealTurnBasedMechanics/Docs/README.md), [UnrealGameMechanics](../../../Plugins/UnrealGameMechanics/Docs/README.md), [UnrealGameIntelligence](../../../Plugins/UnrealGameIntelligence/Docs/README.md)) and implements the actual "Connect Four"-style game rules, board, and networking. This catalogue covers the **live, networked pipeline** only — everything below is actively referenced and maintained. The separate, unreferenced legacy pipeline (state machine / facade / view-model / player data) has its own full write-up in [LegacyPipeline.md](LegacyPipeline.md) and is intentionally excluded here.

For the request→server→board flow these classes participate in, see [ServerAuthoritative-ActionRequest.md](Workflows/ServerAuthoritative-ActionRequest.md) and [SingleSourceOfTruth-Replication.md](Workflows/SingleSourceOfTruth-Replication.md).

## Top-Level Types

| Type | File | Purpose |
|---|---|---|
| `ConnectIt_GameplayTags.h/.cpp` | `Public/ConnectIt_GameplayTags.h` | Native `UE_DECLARE_GAMEPLAY_TAG_EXTERN` tags in three families: match-phase/request-routing (`ConnectIt.Game.Pre/Post/SelectTile/PlacePiece/UpdateBoard`), tile-visual (`ConnectIt.Tile.ValidHover/InvalidHover/Occupied/Default`), and board-event (`ConnectIt.Event.PiecePlaced/LineScored/PlayerWin/TurnEnd`). See [Conventions.md](Conventions.md#gameplaytag-conventions) for the naming rule. |
| `FConnectItTileData` | `Public/ConnectIt_Structs.h` | Per-tile payload: `FactionPiece` (`-1` = empty), `Multiplier`, `bIsActive`. |
| `FConnectItBoardState` | `Public/ConnectIt_Structs.h` | The full replicable board state: parallel arrays `TilePositions`/`TileDataArray` (a `TMap` doesn't replicate, hence parallel arrays), `FactionTurn`, `ScoreBoard`, `LastModifiedTurn`, `bGameOver`, `WinningFactionSlot`, plus helpers (`GetTileData`, `SetTileData`, `IsTileOccupied`, `IsTileValidForPlacement`, `GetScore`). |
| `FConnectItBoardChangeEvent` | `Public/ConnectIt_Structs.h` | Describes exactly what changed on the last `SetBoardState` call — piece-placed / line-scored / game-won / etc., each with its own payload fields. Edge-triggered for `bGameWon`; each mutation type's fields are disjoint (see [GameplayTag-EventSequencing.md](Workflows/GameplayTag-EventSequencing.md)). |
| `FConnectItBoardStateSnapshot` | `Public/ConnectIt_Structs.h` | `{ PreviousState, CurrentState, ChangeEvent }` — the single struct that is the one `Replicated` property on `UConnectIt_BoardStateComponent`. |
| `FConnectItRequestPlacePiece` | `Public/ConnectIt_Structs.h` | Payload struct unwrapped from `FTurnActionRequest.Payload` (an `FInstancedStruct`, from `UnrealTurnBasedMechanics`) by `UConnectIt_BoardRequestMediator::ProcessRequest`. |

## Action/

`UTurnBasedAction`/`UTurnBasedSpectatorAction` subclasses (from `UnrealTurnBasedMechanics`) — one per player action.

| Class | Base | Purpose |
|---|---|---|
| `UConnectIt_DefaultViewerAction` | `UTurnBasedSpectatorAction` | Minimal action active while it's the opponent's turn; enables cursor, binds to `UConnectIt_BoardStateComponent`'s change signal to drive observation UI. Cannot mutate board state. |
| `UConnectIt_PlacePieceAction` | `UTurnBasedAction` | The "place a piece" action. Validates hover/selection, sends gameplay tags to tiles for hover feedback, builds an `FTurnActionRequest` and fires `OnChangeRequested`. Has no knowledge of pools/piece-actors/state mutation — that's server-side, in `UConnectIt_BoardRequestMediator`. Resolves its `TileRegistry` from the owning `AConnectIt_PlayerController` (see Framework/Controller/ below) rather than a board actor. Key props: `Tag_ValidHover`, `Tag_InvalidHover`, `Tag_Default`, `Tag_RequestType` (must match `ConnectIt_Game_PlacePiece`). |

Both resolve their `UTurnBasedParticipantComponent`/controller-owned dependencies once in `PostInitialiseAction_Implementation` — an eager, fail-loud resolution pattern that replaced a prior repeated-lookup-on-`Activate` pattern.

## Board/

`AConnectIt_BoardManager` (the old world-placed board orchestrator actor) has been retired entirely — see [Board architecture overhaul](#board-architecture-overhaul) below. Board mutation, board state, and the tile/piece registries now live on three different, purpose-fit owners: `UConnectIt_BoardRequestMediator` on the GameMode (server-only), `UConnectIt_BoardStateComponent` on the GameState (replicated single source of truth), and `TileRegistry`/`PieceRegistry` on `AConnectIt_PlayerController` (per-machine, see Framework/Controller/ below).

| Class | Base | Purpose |
|---|---|---|
| `UConnectIt_BoardRequestMediator` | `UObject` | The board request orchestrator — replaces `AConnectIt_BoardManager`. Constructed via `NewObject<T>(this)` on `AConnectIt_GameMode` in `HandleMatchHasStarted()` (not the constructor, to avoid the CDO/archetype-timing pitfall), so it only ever exists on the server and is structurally unreachable from any client (`GetAuthGameMode()` is null on clients by engine design — no `HasAuthority()` guard needed or present). Public API: `ProcessRequest(FTurnActionRequest)` — the single entry point for all board mutations, dispatched by `RequestType` tag to private `HandlePlacePieceRequest`/etc. (the old shift branch was removed — see [Board architecture overhaul](#board-architecture-overhaul)). Resolves board state via `GetWorld()->GetGameState<AConnectIt_GameState>()->GetBoardStateComponent()` (its `GetWorld()` resolves through the `UObject::GetOuter()` chain to the owning GameMode) and rule strategy via a sibling `UConnectIt_BoardRules*` injected once through `Initialise()`. Does **not** maintain its own `OnPiecePlaced`/`OnLineScored`/etc. delegates — those were removed in favor of gameplay tags. **In progress**: the tag-reactive interpreter pipeline that used to turn board-change tags into piece spawn/despawn calls has been removed project-wide; its replacement is a `TurnBasedGameEventQueue` of `UTurnBasedGameEvent` instances (e.g. `GameEventPlacePiece`), populated by `CreateGameEventsFromBoardUpdate` (`BlueprintNativeEvent`) and meant to be run by `ExecuteGameEvents` — currently an empty stub, so nothing is reactively driven from board-state changes yet. |
| `UConnectIt_BoardStateComponent` | `UBoardStateComponentBase` (`UnrealGridMechanics`) | **The single source of truth for board state**, and the only replicated data-carrying component in the module. Lives on `AConnectIt_GameState` now (a `CreateDefaultSubobject`, reachable via `AConnectIt_GameState::GetBoardStateComponent()`) — genuinely singular the way `UTurnBasedParticipantManagerComponent` already is on the same GameState, rather than tied to a placeable actor instance. One property: `UPROPERTY(ReplicatedUsing=OnRep_BoardSnapshot) FConnectItBoardStateSnapshot BoardSnapshot`. Server API: `InitialiseBoardState()`, `SetBoardState(NewState, ChangeEvent)` (captures previous, applies new, fires `OnBoardStateChanged`, then `EnqueueBoardEventTags()` — reads `ChangeEvent`'s flags and calls `UGameEventTaskSubsystem::QueueTagContainer` once per event, in order — immediately on server; `check(IsAuthoritative())`-guarded). Read API: `GetCurrentState()`, `GetPreviousState()`, `GetChangeEvent()`, `GetInfo()` (all `BlueprintPure` — `GetInfo()` bundles `CurrentState`/`ChangeEvent` into one `FConnectItBoardStateInfo`, used by `DWidget_ConnectIt_BoardStateComponent` to seed its initial push-event values, see `Docs/Workflows/DebugWidgets.md`). Clients receive the same broadcast, and enqueue the same tags themselves, via `OnRep_BoardSnapshot`. |
| `UConnectIt_BoardSequencerComponent` | `UActorComponent` | **Deprecated** — no longer instantiated by anything. Used to drive the gated visual sequence for board changes via `UGameEventTaskSubsystem::QueueTagSequence` plus its own second, redundant queue on top; both responsibilities were absorbed elsewhere once `QueueTagContainer` became a real FIFO queue in its own right — see `UConnectIt_BoardStateComponent`'s row above and the class's own doc comment. Kept in the codebase rather than deleted. |

### Board/Rules/ — pluggable scoring & win-condition strategy pattern

| Class | Base | Purpose |
|---|---|---|
| `IConnectIt_ScoringRule` | `UInterface` | `ApplyScoring(MutableState, Position, FactionSlot, OutScoringPositions) -> float` (`BlueprintNativeEvent`) -- appends every tile from a completed line to `OutScoringPositions` (left untouched if nothing scored). |
| `IConnectIt_WinCondition` | `UInterface` | `CheckWinCondition(MutableState)` (`BlueprintNativeEvent`), sets `bGameOver`/`WinningFactionSlot`. |
| `UConnectIt_LineScoringRule` | `UObject`, implements `IConnectIt_ScoringRule` | Default N-in-a-row (`ConnectLength`, default 4) scoring — ported unchanged from the board manager's old hardcoded logic. |
| `UConnectIt_ScoreThresholdWinCondition` | `UObject`, implements `IConnectIt_WinCondition` | Default win condition: first faction to reach `WinScoreThreshold` (default 100) wins. |
| `UConnectIt_BoardRules` | `UObject` | Renamed and converted from `UConnectIt_BoardRulesComponent` (was a `UActorComponent` on `AConnectIt_BoardManager`). Owns swappable `TScriptInterface<IConnectIt_ScoringRule> ScoringRule` and `TScriptInterface<IConnectIt_WinCondition> WinConditionRule` (both `EditAnywhere`, defaulted to the two classes above by `Initialise()` if unset — the old `BeginPlay` default-selection logic, ported to an explicit call since `UObject` has no `BeginPlay`); wraps them behind plain `ApplyScoring()`/`CheckWinCondition()`/`GetTargetScore()`/`GetActiveWinConditionName()`/`GetActiveScoringRuleName()` methods so `UConnectIt_BoardRequestMediator` never touches the interfaces directly. Constructed as a sibling of the mediator on `AConnectIt_GameMode`, with its rule selection sourced from the current level's `UConnectIt_LevelConfigDataAsset` (see Framework/Data/ below). Explicitly mirrors `UnrealTurnBasedMechanics`'s `ITurnOrderInterface`/strategy pattern. |

### Board architecture overhaul

As of this pass, the old model — a single world-placed `AConnectIt_BoardManager` actor hosting board state, config, rules, and the tile/piece registries all as actor components — has been replaced:

- **Board state** → `UConnectIt_BoardStateComponent`, now on `AConnectIt_GameState` (Board/ table above).
- **Request handling + rules** → `UConnectIt_BoardRequestMediator` + `UConnectIt_BoardRules`, both plain `UObject`s constructed on `AConnectIt_GameMode` — server-only by construction, not by convention.
- **Tile/piece registries** → `TileRegistry`/`PieceRegistry` on `AConnectIt_PlayerController` (Framework/Controller/ below) — per-machine local lookups, no longer reachable from a shared world actor.
- **Per-level config** → `UConnectIt_LevelConfigDataAsset` + `UConnectIt_LevelConfigSettings` (Framework/Data/ below), replacing `UConnectIt_ConfigComponent` and in-level Blueprint configuration of the old board manager.
- **The whole shift pipeline** (`UConnectIt_BoardShiftComponent`, `UConnectIt_ShiftAction`, `UConnectIt_TutorialShiftIntroAction`, `FConnectItRequestBoardShift`, `ConnectIt_Game_Shift`/`ConnectIt_Event_Shift`) was confirmed fully dead (`HandleShiftRequest` always returned `false`, body commented out) and removed outright rather than migrated.
- `UConnectIt_BoardManagerSubsystem` — the world-actor discovery cache this whole change makes unnecessary — was removed along with `AConnectIt_BoardManager` itself.

**Manual editor/content steps this change cannot make from code** (still outstanding): remove the `AConnectIt_BoardManager` placements from `ConnectIt_Level_Sandbox.umap`, `L_NetworkTest.umap`, `L_Sandbox.umap`; the two Blueprint children (`Content/_ConnectIt/Board/ConnectIt_BoardManager.uasset`, `Content/_ConnectItNetworked/Game/Board/CI_BoardManager_Play.uasset`) now have a deleted parent class and need removal/replacement; author at least one `ConnectIt_LevelConfigDataAsset` per level and register it in Project Settings → "ConnectIt Level Config"; remove the SCS-added `UConnectIt_BoardShiftComponent` reference inside the old board manager Blueprint and any loadout-asset references to the deleted shift actions.

## Framework/Controller/

| Class | Base | Purpose |
|---|---|---|
| `AConnectIt_PlayerController` | `ATurnBasedPlayerControllerBase` (`UnrealTurnBasedMechanics`) | Adds ConnectIt-specific plumbing on top of generic turn/action wiring. Owns `TileRegistry`/`PieceRegistry` as `EditAnywhere, Instanced` `UGridTileRegistryBase`/`UGridPieceRegistryBase` properties (relocated from the old `AConnectIt_BoardManager`/`ABoardManagerBase` — per-machine local lookups, initialised/shut down from `BeginPlay`/`EndPlay`), with `GetTileRegistry()`/`GetPieceRegistry()` accessors. `InitialiseFromLevelConfig()` (renamed from `InitialiseFromBoardManager`) resolves the current level's `UConnectIt_LevelConfigDataAsset` via `UConnectIt_GameUtilityLibrary::GetLevelConfig()` and loads the player's action loadout from it. Routes board-change requests to the server via `ServerRouteBoardChangeRequest` (`UFUNCTION(Server, Reliable)`); the server-side implementation validates `ParticipantComponent->IsMyTurn()`, then calls `GetWorld()->GetAuthGameMode<AConnectIt_GameMode>()->ProcessBoardRequest(Request)`. **Note**: the file is `ConnectIt_PlayerController.h` / `AConnectIt_PlayerController.cpp` — see [Conventions.md Discrepancy #4](Conventions.md#discrepancies) for the filename/prefix inconsistency this pair has. **Known issue**: see [README Known Issues](README.md#known-issues) — the `FactionID` validation gap in `ServerRouteBoardChangeRequest_Implementation`. |
| `AConnectIt_AIController` | `ATurnBasedAIController` (`UnrealTurnBasedMechanics`) | Registers itself as an AI participant, loads `EnemyLoadout` from the resolved `UConnectIt_LevelConfigDataAsset` (`InitialiseFromLevelConfig()`, renamed from `InitialiseFromBoardManager`), exposes `BeginMakeDecision()` as a `BlueprintImplementableEvent` for subclasses to implement actual AI. `CheckAndApplyForcedMove()` is currently a stub returning `false` (blackboard forced-move logic not yet implemented, per its own comment). No `TileRegistry`/`PieceRegistry` dependency — AI decision-making works purely off replicated `FConnectItBoardState` values. |

## Framework/Data/ (live classes only — see [LegacyPipeline.md](LegacyPipeline.md) for the rest of this folder)

| Class | Base | Purpose |
|---|---|---|
| `UConnectIt_LevelConfigDataAsset` | `UDataAsset` | Replaces `UConnectIt_ConfigComponent` (retired — was a `UActorComponent` on the now-deleted `AConnectIt_BoardManager`) and in-level Blueprint configuration generally. Holds `PlayerLoadout`/`EnemyLoadout` (`TObjectPtr<UActionLoadoutDataAsset>`, from `UnrealTurnBasedMechanics`), `AISearchDepth`/`AIThreadDepth`, `PieceActorClass`, `PiecePoolInitialSize`, plus `ScoringRule`/`WinConditionRule` as `Instanced` properties (designer-selectable concrete strategy per level, matching `UConnectIt_BoardRules`' own fields). `#if WITH_EDITOR IsDataValid(...)` warns if `PlayerLoadout`/`EnemyLoadout`/`PieceActorClass` is unset, mirroring `UActionLoadoutDataAsset`'s existing validation pattern. One instance authored per level. |
| `UConnectIt_LevelConfigSettings` | `UDeveloperSettings` (`Config=Game, DefaultConfig`) | The level → config mapping: `TMap<FName, TSoftObjectPtr<UConnectIt_LevelConfigDataAsset>> LevelConfigs`, keyed by level name, edited in Project Settings → "ConnectIt Level Config". Resolved via `UConnectIt_GameUtilityLibrary::GetLevelConfig()`, which strips the PIE-instance prefix off `UGameplayStatics::GetCurrentLevelName()` before the lookup. Intentionally symmetric and un-networked — server and client each resolve the same static asset independently, since level config is authored content every machine already has, not runtime server state. |

## Framework/GameMode/ (live class only — `AConnectIt_GameMode_Play` is legacy, see [LegacyPipeline.md](LegacyPipeline.md))

| Class | Base | Purpose |
|---|---|---|
| `AConnectIt_GameMode` | `ATurnBasedGameMode` (`UnrealTurnBasedMechanics`) | The live game mode. `EConnectItMatchType` enum (`Adventure` = 1 human vs. AI, `Online` = 2 humans via lobby). Sets `GameStateClass = AConnectIt_GameState`, `PlayerStateClass = ATurnBasedPlayerState`, configures `TurnDuration=90`, `ForfeitThreshold=3`, `ReconnectTimeout=30` in its constructor. Owns `TObjectPtr<UConnectIt_BoardRequestMediator> BoardRequestMediator` and `TObjectPtr<UConnectIt_BoardRules> BoardRules`, both constructed via `NewObject<T>(this)` in `HandleMatchHasStarted()` (not the constructor — avoids the CDO/archetype-timing pitfall) and initialised from the resolved `UConnectIt_LevelConfigDataAsset`. `ProcessBoardRequest(const FTurnActionRequest&)` is the public forwarding wrapper to `BoardRequestMediator->ProcessRequest(...)`, so callers (`AConnectIt_PlayerController`) depend on the GameMode's surface, not the mediator's existence directly. `PostLogin` tracks `ConnectedHumanCount`, starts a ready-check once the expected count is reached (Online mode) or immediately spawns/registers AI (Adventure mode, via `HandleMatchHasStarted` → `SpawnAndRegisterAI` → `InitialiseBoard`). `InitialiseBoard()` resolves `BoardStateComponent` via `AConnectIt_GameState` and a `TileRegistry` by iterating connected `AConnectIt_PlayerController`s (tile layout is level-authored/deterministic, so any one agrees), and binds `HandleGameOver` to `UGameEventTaskSubsystem::BindOnTagComplete(ConnectIt_Event_PlayerWin, ...)`. `HandleGameOver` reads `WinningFactionSlot` from board state (not a delegate param) and writes it via `AConnectIt_GameState::SetMatchResult`. |

## Framework/GameState/

| Class | Base | Purpose |
|---|---|---|
| `AConnectIt_GameState` | `ATurnBasedGameState` (`UnrealTurnBasedMechanics`) | Replicated match state. Owns `UConnectIt_BoardStateComponent BoardStateComponent` as a `CreateDefaultSubobject` (relocated from the old `AConnectIt_BoardManager` — see [Board architecture overhaul](#board-architecture-overhaul)), exposed via `GetBoardStateComponent()`. `FConnectItMatchResult` struct (`WinningFactionSlot`, `FinalScores`, `TotalTurnsPlayed`, `EndReason` enum `EMatchEndReason{ScoreThresholdReached, OpponentForfeited, OpponentDisconnected, Unknown}`, `bMatchOver`) replicated via `ReplicatedUsing=OnRep_MatchResult`. `SetMatchResult()` is server-only, broadcasts `OnMatchResultUpdated` immediately on the server; `OnRep_MatchResult` re-broadcasts on clients. Also exposes read-only convenience wrappers over board state for UI: `GetFactionScore`, `GetAllScores`, `IsTileOccupied`, `GetBoardSnapshot`, `GetActiveParticipantName`, `IsLocalPlayerTurn`. |

## Framework/Subsystem/

| Class | Base | Purpose |
|---|---|---|
| `UConnectIt_BlackboardSubsystem` | `UWorldSubsystem` | Generic per-participant modifier blackboard (`ApplyModifier`/`ClearModifier`/`HasModifier`/`GetModifier`, keyed by `TargetSlotIndex`) — for things like shard activations. Delegates `OnModifierApplied`/`OnModifierCleared`. |

`UConnectIt_BoardManagerSubsystem` (formerly here — a discovery cache for the single `AConnectIt_BoardManager` in the level) has been removed entirely: board state is reachable via `GetGameState<T>()->GetBoardStateComponent()` directly, registries live on the local `AConnectIt_PlayerController`, and the request mediator/rules are server-only `UObject`s no client code needs a reference to. See [Board architecture overhaul](#board-architecture-overhaul).

## GameIntelligence/InfluenceMap/

| Class | Base | Purpose |
|---|---|---|
| `UPieceControlInfluenceMap` | `UObject`, implements `IGI_FloatGridDataProviderInterface` (`UnrealGameIntelligence`) | `GetData(FIntPoint)` influence-map provider over an internal `TMap<FIntPoint,int32> InfluenceMap`, refreshed via `Update()` from `UConnectIt_BoardStateComponent`. **Known issue**: see [README Known Issues](README.md#known-issues) — `Update()` compares against `FactionTurn`, which is never populated on the real board. |
| `UPieceControlMapVisualizer` | `UObject`, implements `IGI_InfluenceMapVisualiser` (`UnrealGameIntelligence`) | Visualizer wrapper (`GetTag`/`GetDisplayName`/`IsActive`/`Activate`/`Deactivate`) for the map above, for debug display. |

Both classes are unprefixed (no `ConnectIt_` prefix) — see [Conventions.md](Conventions.md#type-prefixes).

## Grid/

| Class | Base | Purpose |
|---|---|---|
| `AConnectIt_GridPiece` | `AGridPieceBase` (`UnrealGridMechanics`), implements `IGridFactionInterface`, `IGridLevelInterface`, `IGridTileHandler`, `IActorPoolInterface` | The visual piece actor. `bReplicates = true` in its constructor; replicates `FactionID` (`ReplicatedUsing=OnRep_FactionID`) and `OccupiedPosition` (`Replicated`). `InitialisePiece(FactionID, Position)` is called server-side on pool retrieval and drives `OnFactionVisualUpdate` (`BlueprintNativeEvent`, Blueprint sets mesh/material/VFX) both there and from `OnRep_FactionID`. Also calls `NotifyActivationVisualComplete`/`NotifyDeactivationVisualComplete` (inherited from `AGridPieceBase`) once its spawn-in/despawn-out visual genuinely finishes, for whatever eventually gates on it. **Note**: the class that used to spawn this (client-side only, server explicitly skipped) has been removed along with the rest of the tag-reactive interpreter pipeline — see [Duplication.md #6](Duplication.md#6-aconnectit_gridpieces-dead-replication-setup) — and its replacement (`UConnectIt_BoardRequestMediator`'s `CreateGameEventsFromBoardUpdate`/`ExecuteGameEvents`) doesn't spawn pieces yet either, so as of this writing nothing in the live pipeline spawns this actor. |

## Library/

`UBlueprintFunctionLibrary` subclasses.

| Class | Purpose |
|---|---|
| `UConnectIt_GameUtilityLibrary` | The main static query API for the networked pipeline — explicitly documented as *"Replaces UConnectIt_GameFacade for the networked game"* (see [LegacyPipeline.md](LegacyPipeline.md)). Large surface: `GetBoardStateComponent` (via `AConnectIt_GameState`), `GetTileRegistry` (via the local `AConnectIt_PlayerController`, falling back to scanning every connected controller server-side), `GetLevelConfig` (via `UConnectIt_LevelConfigSettings`), tile queries (`GetAllGridTiles`, `GetEmptyGridTiles`, `IsTileEmpty`, `GetGridTilesWithFactionPieces`, `GetTileAtPosition`, `GetRandomEmptyGridTile`, `IsGameBoardFull`, `HasFactionWon`), game state/turn accessors (`GetConnectItGameState`, `GetParticipantManager`, `GetCurrentTurnPhase`), local-player helpers (`GetLocalPlayerSlotIndex`, `IsLocalPlayerTurn`), subsystem accessors (`GetBlackboardSubsystem`, `GetGridSubsystem`). `GetBoardManager`/`GetConfigComponent` were removed — nothing left for either to return once `AConnectIt_BoardManager` was retired. |
| `UConnectIt_GameRulesLibrary` | Near-empty: `ConnectIt_Score_Max = 100` constant, `IsGameOver(ScoreBoard)` helper. |
| `UConnectIt_GridFloodLibrary` | Empty stub — no members. |
| `UConnectIt_GridRulesLibrary` | Near-empty: `ConnectIt_Grid_Size_Default = 100.f` constant only. |

## MinMax/

| Class | Purpose |
|---|---|
| `ConcreteMinMaxExample.h` | Entirely commented out — a 3-tier template/`UObject` example (`TMinMaxManagerBase`/`TMinMaxManagerBase2` → `UMinMaxManagerBase` → `UConnectIt_MinMax`) showing async tree build/evaluate via `UE::Tasks::Launch`. Not compiled, not referenced — reference code only. |
| `UConnectIt_MinMaxManager` | Synchronous(ish) min-max wrapper using `ConnectIt::FMinMaxNode` (namespaced struct: `Tiles: TMap<FGridPosition,FTileDescriptor>`, `ScoreBoard`, `FactionTurn`, `Children`, `Score`). `BuildTree`/`EvaluateTree`/`GetMoveScoreInfo`/`GetMoveScores`, delegates `OnTreeBuilt`/`OnTreeEvaluated`. |
| `UConnectIt_MinMaxTreeBuilder` | A second, separate min-max tree implementation using its own node type `FConnectItMinMaxNode` (global namespace). Async via `BuildTreeAsync`/`SolveTreeAsync`, uses `FCriticalSection RootNodeMutex` for thread safety, returns `TArray<FConnectItMoveOutcome> GetMoveSuggestions()`. |

**See [Duplication.md #2](Duplication.md#2-two-parallel-minmax-tree-search-implementations)** — `UConnectIt_MinMaxManager` and `UConnectIt_MinMaxTreeBuilder` are two unreconciled, parallel implementations of the same search problem.
