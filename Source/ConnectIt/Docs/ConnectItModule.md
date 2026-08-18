# ConnectIt Module — Class Catalogue

`Source/ConnectIt/` is the game module — the project-specific layer that consumes the six generic plugins ([UnrealGridMechanics](../../../Plugins/UnrealGridMechanics/Docs/README.md), [UnrealTurnBasedMechanics](../../../Plugins/UnrealTurnBasedMechanics/Docs/README.md), [UnrealGameMechanics](../../../Plugins/UnrealGameMechanics/Docs/README.md), [UnrealGameIntelligence](../../../Plugins/UnrealGameIntelligence/Docs/README.md)) and implements the actual "Connect Four"-style game rules, board, and networking. This catalogue covers the **live, networked pipeline** only — everything below is actively referenced and maintained. The separate, unreferenced legacy pipeline (state machine / facade / view-model / player data) has its own full write-up in [LegacyPipeline.md](LegacyPipeline.md) and is intentionally excluded here.

For the request→server→board flow these classes participate in, see [ServerAuthoritative-ActionRequest.md](Workflows/ServerAuthoritative-ActionRequest.md) and [SingleSourceOfTruth-Replication.md](Workflows/SingleSourceOfTruth-Replication.md).

## Top-Level Types

| Type | File | Purpose |
|---|---|---|
| `ConnectIt_GameplayTags.h/.cpp` | `Public/ConnectIt_GameplayTags.h` | Native `UE_DECLARE_GAMEPLAY_TAG_EXTERN` tags in three families: match-phase/request-routing (`ConnectIt.Game.Pre/Post/SelectTile/PlacePiece/UpdateBoard/Shift`), tile-visual (`ConnectIt.Tile.ValidHover/InvalidHover/Occupied/Default`), and board-event (`ConnectIt.Event.PiecePlaced/LineScored/PlayerWin/Shift/TurnEnd`). See [Conventions.md](Conventions.md#gameplaytag-conventions) for the naming rule. |
| `FConnectItTileData` | `Public/ConnectIt_Structs.h` | Per-tile payload: `FactionPiece` (`-1` = empty), `Multiplier`, `bIsActive`. |
| `FConnectItBoardState` | `Public/ConnectIt_Structs.h` | The full replicable board state: parallel arrays `TilePositions`/`TileDataArray` (a `TMap` doesn't replicate, hence parallel arrays), `FactionTurn`, `ScoreBoard`, `LastModifiedTurn`, `bGameOver`, `WinningFactionSlot`, plus helpers (`GetTileData`, `SetTileData`, `IsTileOccupied`, `IsTileValidForPlacement`, `GetScore`). |
| `FConnectItBoardChangeEvent` | `Public/ConnectIt_Structs.h` | Describes exactly what changed on the last `SetBoardState` call — piece-placed / line-scored / game-won / shift-applied, each with its own payload fields. Edge-triggered for `bGameWon`; placement fields and shift fields are always disjoint (see [GameplayTag-EventSequencing.md](Workflows/GameplayTag-EventSequencing.md)). |
| `FConnectItBoardStateSnapshot` | `Public/ConnectIt_Structs.h` | `{ PreviousState, CurrentState, ChangeEvent }` — the single struct that is the one `Replicated` property on `UConnectIt_BoardStateComponent`. |
| `FConnectItRequestPlacePiece`, `FConnectItRequestBoardShift` | `Public/ConnectIt_Structs.h` | Payload structs unwrapped from `FTurnActionRequest.Payload` (an `FInstancedStruct`, from `UnrealTurnBasedMechanics`) by `AConnectIt_BoardManager::ProcessRequest`. |

## Action/

`UTurnBasedAction`/`UTurnBasedSpectatorAction` subclasses (from `UnrealTurnBasedMechanics`) — one per player action.

| Class | Base | Purpose |
|---|---|---|
| `UConnectIt_DefaultViewerAction` | `UTurnBasedSpectatorAction` | Minimal action active while it's the opponent's turn; enables cursor, binds to `UConnectIt_BoardStateComponent`'s change signal to drive observation UI. Cannot mutate board state. |
| `UConnectIt_PlacePieceAction` | `UTurnBasedAction` | The "place a piece" action. Validates hover/selection, sends gameplay tags to tiles for hover feedback, builds an `FTurnActionRequest` and fires `OnChangeRequested`. Has no knowledge of pools/piece-actors/state mutation — that's server-side, in `AConnectIt_BoardManager`. Key props: `Tag_ValidHover`, `Tag_InvalidHover`, `Tag_Default`, `Tag_RequestType` (must match `ConnectIt_Game_PlacePiece`). |
| `UConnectIt_ShiftAction` | `UTurnBasedAction` | The "shift a row/column" action. Deliberately doesn't decide which row/column/direction; exposes `RequestShift(FShiftOperation)` (`BlueprintCallable`) for UI/input to call. `Tag_RequestType` must match `ConnectIt_Game_Shift`. |
| `UConnectIt_TutorialShiftIntroAction` | `UTurnBasedAction` | Scripted tutorial variant of the shift action with an internal two-state state machine (`ShowingInfo` → `SelectingShiftTarget`). Shows a widget (`InfoWidgetClass`), dismissed via `NotifyInfoDismissed()`. `GetShiftOperationForTile` is a `BlueprintNativeEvent` deliberately left unimplemented in the base (returns an invalid op + warns) as a design placeholder for tutorial-specific Blueprint logic, not a bug. |

All four resolve `AConnectIt_BoardManager`/`UTurnBasedParticipantComponent` once in `PostInitialiseAction_Implementation` — an eager, fail-loud resolution pattern that replaced a prior repeated-lookup-on-`Activate` pattern.

## Board/

The central board orchestrator and its components.

| Class | Base | Purpose |
|---|---|---|
| `AConnectIt_BoardManager` | `AActor`, `Blueprintable` | The board orchestrator. Owns/creates every board-related component as default subobjects (`TileRegistryComponent`, `PieceRegistryComponent`, `ConnectItConfigComponent`, `BoardStateComponent`, `BoardShiftComponent`, `BoardRulesComponent`, plus interpreters). Public API: `ProcessRequest(FTurnActionRequest)` — the single entry point for all board mutations, dispatched by `RequestType` tag to private, server-only, `HasAuthority()`-gated `HandlePlacePieceRequest`/`HandleShiftRequest`; `InitialiseBoard(int32 NumFactions)`. Has no turn-end-specific code at all — `UTurnBasedParticipantManagerComponent::EndTurn` triggering `ConnectIt_Event_TurnEnd` is guaranteed to fire after every board-event tag ahead of it in `UGameEventTaskSubsystem`'s own queue has completed, by ordering alone (see `Workflows/GameEventSubsystem_Workflow.txt`'s "TURN SYSTEM WAITS ON THIS" section) — an earlier persistent polling task that checked a board sequencer component's idle state was removed as redundant once that queue existed. Does **not** maintain its own `OnPiecePlaced`/`OnLineScored`/etc. delegates — those were removed in favor of gameplay tags. |
| `UConnectIt_BoardStateComponent` | `UBoardStateComponentBase` (`UnrealGridMechanics`) | **The single source of truth for board state**, and the only replicated data-carrying component in the module. One property: `UPROPERTY(ReplicatedUsing=OnRep_BoardSnapshot) FConnectItBoardStateSnapshot BoardSnapshot`. Server API: `InitialiseBoardState()`, `SetBoardState(NewState, ChangeEvent)` (captures previous, applies new, fires `OnBoardStateChanged`, then `EnqueueBoardEventTags()` — reads `ChangeEvent`'s flags and calls `UGameEventTaskSubsystem::QueueTagContainer` once per event, in order — immediately on server; `check(IsAuthoritative())`-guarded). Read API: `GetCurrentState()`, `GetPreviousState()`, `GetChangeEvent()` (`BlueprintPure`). Clients receive the same broadcast, and enqueue the same tags themselves, via `OnRep_BoardSnapshot`. |
| `UConnectIt_BoardShiftComponent` | `UActorComponent` | Owns the entire row/column shift operation end-to-end: `ComputeShift` (pure/synchronous position remap), `ApplyShiftToState`, and animation (`TickComponent`, `StartShiftAnimation`, `FinaliseShift`). Self-registers a persistent `UGameEventTask_Async` against `ConnectIt_Event_Shift` in `BeginPlay` so the animation plays as a *reaction* to committed state, not before it. Its doc comment notes it absorbed the generic plugin `UBoardShiftComponent`'s responsibilities since that abstraction only ever had one consumer. |
| `UConnectIt_BoardSequencerComponent` | `UActorComponent` | **Deprecated** — no longer instantiated by `AConnectIt_BoardManager` or anything else. Used to drive the gated visual sequence for board changes via `UGameEventTaskSubsystem::QueueTagSequence` plus its own second, redundant queue on top; both responsibilities were absorbed elsewhere once `QueueTagContainer` became a real FIFO queue in its own right — see `UConnectIt_BoardStateComponent`'s row above and the class's own doc comment. Kept in the codebase rather than deleted. |

### Board/Rules/ — pluggable scoring & win-condition strategy pattern

| Class | Base | Purpose |
|---|---|---|
| `IConnectIt_ScoringRule` | `UInterface` | `ApplyScoring(MutableState, Position, FactionSlot, OutScoringPositions) -> float` (`BlueprintNativeEvent`) -- appends every tile from a completed line to `OutScoringPositions` (left untouched if nothing scored). |
| `IConnectIt_WinCondition` | `UInterface` | `CheckWinCondition(MutableState)` (`BlueprintNativeEvent`), sets `bGameOver`/`WinningFactionSlot`. |
| `UConnectIt_LineScoringRule` | `UObject`, implements `IConnectIt_ScoringRule` | Default N-in-a-row (`ConnectLength`, default 4) scoring — ported unchanged from the board manager's old hardcoded logic. |
| `UConnectIt_ScoreThresholdWinCondition` | `UObject`, implements `IConnectIt_WinCondition` | Default win condition: first faction to reach `WinScoreThreshold` (default 100) wins. |
| `UConnectIt_BoardRulesComponent` | `UActorComponent` | Owns swappable `TScriptInterface<IConnectIt_ScoringRule> ScoringRule` and `TScriptInterface<IConnectIt_WinCondition> WinConditionRule` (both `EditAnywhere`, defaulted to the two classes above in `BeginPlay` if unset); wraps them behind plain `ApplyScoring()`/`CheckWinCondition()` methods so `AConnectIt_BoardManager` never touches the interfaces directly. Explicitly mirrors `UnrealTurnBasedMechanics`'s `ITurnOrderInterface`/strategy pattern. |

## Framework/Controller/

| Class | Base | Purpose |
|---|---|---|
| `AConnectIt_PlayerController` | `ATurnBasedPlayerControllerBase` (`UnrealTurnBasedMechanics`) | Adds ConnectIt-specific plumbing on top of generic turn/action wiring: finds the board manager (`InitialiseFromBoardManager`), loads the player's action loadout, and routes board-change requests to the server via `ServerRouteBoardChangeRequest` (`UFUNCTION(Server, Reliable)`). Server-side implementation validates `ParticipantComponent->IsMyTurn()`, then calls `BoardManager->ProcessRequest(Request)`. **Note**: the file is `ConnectIt_PlayerController.h` / `AConnectIt_PlayerController.cpp` — see [Conventions.md Discrepancy #4](Conventions.md#discrepancies) for the filename/prefix inconsistency this pair has. **Known issue**: see [README Known Issues](README.md#known-issues) — the `FactionID` validation gap in `ServerRouteBoardChangeRequest_Implementation`. |
| `AConnectIt_AIController` | `ATurnBasedAIController` (`UnrealTurnBasedMechanics`) | Registers itself as an AI participant, loads `EnemyLoadout`, exposes `BeginMakeDecision()` as a `BlueprintImplementableEvent` for subclasses to implement actual AI. `CheckAndApplyForcedMove()` is currently a stub returning `false` (blackboard forced-move logic not yet implemented, per its own comment). |

## Framework/Data/ (live class only — see [LegacyPipeline.md](LegacyPipeline.md) for the rest of this folder)

| Class | Base | Purpose |
|---|---|---|
| `UConnectIt_ConfigComponent` | `UActorComponent` | Sits on `AConnectIt_BoardManager`. Holds `PlayerLoadout`/`EnemyLoadout` (`UActionLoadoutDataAsset`, from `UnrealTurnBasedMechanics`), AI search/thread depth, `PieceActorClass`, `PiecePoolInitialSize`. |

## Framework/GameMode/ (live class only — `AConnectIt_GameMode_Play` is legacy, see [LegacyPipeline.md](LegacyPipeline.md))

| Class | Base | Purpose |
|---|---|---|
| `AConnectIt_GameMode` | `ATurnBasedGameMode` (`UnrealTurnBasedMechanics`) | The live game mode. `EConnectItMatchType` enum (`Adventure` = 1 human vs. AI, `Online` = 2 humans via lobby). Sets `GameStateClass = AConnectIt_GameState`, `PlayerStateClass = ATurnBasedPlayerState`, configures `TurnDuration=90`, `ForfeitThreshold=3`, `ReconnectTimeout=30` in its constructor. `PostLogin` tracks `ConnectedHumanCount`, starts a ready-check once the expected count is reached (Online mode) or immediately spawns/registers AI (Adventure mode, via `HandleMatchHasStarted` → `SpawnAndRegisterAI` → `InitialiseBoard`). `InitialiseBoard()` binds `HandleGameOver` to `UGameEventTaskSubsystem::BindOnTagComplete(ConnectIt_Event_PlayerWin, ...)`. `HandleGameOver` reads `WinningFactionSlot` from board state (not a delegate param) and writes it via `AConnectIt_GameState::SetMatchResult`. |

## Framework/GameState/

| Class | Base | Purpose |
|---|---|---|
| `AConnectIt_GameState` | `ATurnBasedGameState` (`UnrealTurnBasedMechanics`) | Replicated match state. `FConnectItMatchResult` struct (`WinningFactionSlot`, `FinalScores`, `TotalTurnsPlayed`, `EndReason` enum `EMatchEndReason{ScoreThresholdReached, OpponentForfeited, OpponentDisconnected, Unknown}`, `bMatchOver`) replicated via `ReplicatedUsing=OnRep_MatchResult`. `SetMatchResult()` is server-only, broadcasts `OnMatchResultUpdated` immediately on the server; `OnRep_MatchResult` re-broadcasts on clients. Also exposes read-only convenience wrappers over board state for UI: `GetFactionScore`, `GetAllScores`, `IsTileOccupied`, `GetBoardSnapshot`, `GetActiveParticipantName`, `IsLocalPlayerTurn`. |

## Framework/Subsystem/

| Class | Base | Purpose |
|---|---|---|
| `UConnectIt_BlackboardSubsystem` | `UWorldSubsystem` | Generic per-participant modifier blackboard (`ApplyModifier`/`ClearModifier`/`HasModifier`/`GetModifier`, keyed by `TargetSlotIndex`) — for things like shard activations. Delegates `OnModifierApplied`/`OnModifierCleared`. |
| `UConnectIt_BoardManagerSubsystem` | `UWorldSubsystem` | Pure cache for the single `AConnectIt_BoardManager` in the level (avoids repeated `TActorIterator` world scans). Populated by `AConnectIt_BoardManager::BeginPlay` on both server and client. `RegisterBoardManager()`, `GetCachedBoardManager()`/`GetBoardManager()` (`BlueprintPure`, silent-null), `OnBoardManagerReady` delegate. Deliberately lives in the *game* module rather than the plugin's `UGridWorldSubsystem`, specifically to avoid a circular plugin↔game-module dependency. See [SubsystemDiscovery-DualAccessPattern.md](Workflows/SubsystemDiscovery-DualAccessPattern.md) for how this is meant to be used alongside `UConnectIt_GameUtilityLibrary::GetBoardManager`. |

## GameIntelligence/InfluenceMap/

| Class | Base | Purpose |
|---|---|---|
| `UPieceControlInfluenceMap` | `UObject`, implements `IGI_FloatGridDataProviderInterface` (`UnrealGameIntelligence`) | `GetData(FIntPoint)` influence-map provider over an internal `TMap<FIntPoint,int32> InfluenceMap`, refreshed via `Update()` from `UConnectIt_BoardStateComponent`. **Known issue**: see [README Known Issues](README.md#known-issues) — `Update()` compares against `FactionTurn`, which is never populated on the real board. |
| `UPieceControlMapVisualizer` | `UObject`, implements `IGI_InfluenceMapVisualiser` (`UnrealGameIntelligence`) | Visualizer wrapper (`GetTag`/`GetDisplayName`/`IsActive`/`Activate`/`Deactivate`) for the map above, for debug display. |

Both classes are unprefixed (no `ConnectIt_` prefix) — see [Conventions.md](Conventions.md#type-prefixes).

## Grid/

| Class | Base | Purpose |
|---|---|---|
| `AConnectIt_GridPiece` | `AGridPieceBase` (`UnrealGridMechanics`), implements `IGridFactionInterface`, `IGridLevelInterface`, `IGridTileHandler`, `IActorPoolInterface` | The visual piece actor. `bReplicates = true` in its constructor; replicates `FactionID` (`ReplicatedUsing=OnRep_FactionID`) and `OccupiedPosition` (`Replicated`). `InitialisePiece(FactionID, Position)` is called server-side on pool retrieval and drives `OnFactionVisualUpdate` (`BlueprintNativeEvent`, Blueprint sets mesh/material/VFX) both there and from `OnRep_FactionID`. **Note**: despite being replicated, this class is only ever spawned client-side by `UConnectIt_PieceSpawnInterpreter` (the server explicitly skips spawning) — see [Duplication.md #6](Duplication.md#6-aconnectit_gridpieces-dead-replication-setup). |

## Interpreter/

`UBoardStateInterpreter` (`UnrealGridMechanics`) subclasses — client-side reactions to board-state changes.

| Class | Purpose |
|---|---|
| `UConnectIt_TileStateInterpreter` | Reads board-state diffs, sends `ConnectIt.Tile.*` gameplay tags to tile actors for visual feedback (occupied/empty/inactive/multiplier-changed). **Known issue**: `FindTileActor` is a confirmed permanent no-op — see [README Known Issues](README.md#known-issues) and [Duplication.md #7](Duplication.md#7-two-near-identical-permanent-no-ops-with-the-same-known-fix). |
| `UConnectIt_PieceSpawnInterpreter` | Spawns/despawns client-local `AConnectIt_GridPiece` actors from a pooled `UActorPool`, keyed by grid position (`ActivePieces` map). Explicitly documented as "client local — never replicated, never exist on server" (`if (GetOwner()->HasAuthority()) return;` guard). `GetWorldPositionForTile` currently always returns `FVector()` — same root cause as `FindTileActor` above, see [Duplication.md #7](Duplication.md#7-two-near-identical-permanent-no-ops-with-the-same-known-fix). |
| `UConnectIt_ScoreInterpreter` | Reads score changes from board state, fires `OnScoreUpdated(FactionSlot, NewScore)` and `OnGameOverDetected(WinningFactionSlot)` delegates for UI binding. |

## Library/

`UBlueprintFunctionLibrary` subclasses.

| Class | Purpose |
|---|---|
| `UConnectIt_GameUtilityLibrary` | The main static query API for the networked pipeline — explicitly documented as *"Replaces UConnectIt_GameFacade for the networked game"* (see [LegacyPipeline.md](LegacyPipeline.md)). Large surface: board manager/state accessors, tile queries (`GetAllGridTiles`, `GetEmptyGridTiles`, `IsTileEmpty`, `GetGridTilesWithFactionPieces`, `GetTileAtPosition`, `GetRandomEmptyGridTile`, `IsGameBoardFull`, `HasFactionWon`), game state/turn accessors (`GetConnectItGameState`, `GetParticipantManager`, `GetCurrentTurnPhase`), local-player helpers (`GetLocalPlayerSlotIndex`, `IsLocalPlayerTurn`), subsystem accessors (`GetBlackboardSubsystem`, `GetGridSubsystem`). |
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
