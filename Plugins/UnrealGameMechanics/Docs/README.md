# UnrealGameMechanics

## Purpose

UnrealGameMechanics is a generic, reusable gameplay-mechanics scaffolding layer: gated/sequenced async task execution keyed by gameplay tags, turn bookkeeping primitives, a generic actor object pool, a simple state-machine base, and scoring helpers. It exists as a separate plugin so that mechanics which are broadly applicable to many turn-based / phase-based games (not tied to any one specific board game) can be developed and reused independently of any one consumer's rules. It is the backbone that `UnrealTurnBasedMechanics` and any consuming project's own board-sequencing/turn-resolution workflows are built on top of.

## Module(s)

| Module | Type | LoadingPhase | Notes |
|---|---|---|---|
| `UnrealGameMechanics` | Runtime | Default | Single module; entry point `FUnrealGameMechanicsModule` (`Public/UnrealGameMechanics.h`). |

## Key Dependencies

- Depends on: `Core`, `GameplayTags`, `InputCore`, `EnhancedInput` (Public); `CoreUObject`, `Engine`, `Slate`, `SlateCore` (Private). No plugin-level dependencies declared in the `.uplugin`.
- Depended on by: `UnrealTurnBasedMechanics` (Public dependency in its `.Build.cs`) and the consuming game module (not named here, to keep this plugin's documentation project-agnostic) — that consumer's board sequencing/turn-resolution workflows are built directly on this plugin's `UGameEventTaskSubsystem`.

## When to Use It

- Gating a sequence of async steps behind gameplay tags so that dependent logic only fires once every registered task/object reports completion (see `GameEvent/`).
- Queuing one or more tag-groups to fire in order via a serialized FIFO queue (`QueueTagContainer`) -- a caller with several events to fire in order just calls it once per event, back to back; the queue's own ordering does the rest. (`QueueTagSequence`, an older single-call chaining primitive, is deprecated in favour of this.)
- Tracking turn state (turn counters, begin/end/pause/resume delegates) for a turn-based or phase-based game.
- Pooling actors that are spawned/despawned frequently (board pieces, projectiles, VFX actors).
- Building a lightweight state machine node hierarchy that doesn't need a full Gameplay Ability System / Behavior Tree.
- Scoring and ranking a set of candidate objects generically (top-N selection, weighted random noise).

## When NOT to Use It / Scope Boundaries

- AI decision-making, tree search, and influence-map visualization belong in `UnrealGameIntelligence`, not here.
- Utility-AI action scoring/evaluation (weighted GameplayTag-based action selection) belongs in `UnrealAIMechanics`, not here.
- Turn-based-specific action framework classes (the actual `TurnBasedAction`/`TurnBasedActionsComponent` types) belong in `UnrealTurnBasedMechanics`, which builds on top of this plugin rather than living inside it.
- Small, generic, non-gameplay C++ helpers (e.g. authority checks) belong in `UnrealCodingUtils`, not here.
- A consuming project's own board/piece logic should consume this plugin's generic primitives (`UGameEventTaskSubsystem`, `UActorPool`, etc.) rather than reimplementing gating/pooling/turn-tracking inside its own game module.

## Notable Design Patterns

- Gated task/phase-barrier pattern (`GameEvent/`): `UGameEventTaskManager` tracks a set of tasks that must ALL complete before firing `OnManagerComplete`, and `UGameEventTaskSubsystem` layers a per-tag registry plus a serialized FIFO queue (`QueueTagContainer`) on top -- the actual single-tag trigger (`TriggerTag`) is private, reachable only internally, so every external caller goes through the queue. This is the reusable core of the project-level "Gated Event Sequencing via Tags" workflow — see [GameplayTag-EventSequencing.md](../../../Source/ConnectIt/Docs/Workflows/GameplayTag-EventSequencing.md).
- Tag-broadcast pattern: `IGameplayTagBroadcaster` / `UGameplayTagBroadcasterComponent` expose a simple register/unregister + multicast-delegate broadcast keyed by gameplay tag.
- UFUNCTION `Category` strings are inconsistent across the plugin (e.g. `"Game Mechanics | Turn"`, `"GameMechanics|Framework"`, `"Actor Pool"`, `"Turn Completion"`, `"Scoring | Selectors"` — mixed spacing/style). See the project-level [naming conventions notes](../../../Source/ConnectIt/Docs/Conventions.md).

## Classes

### Broadcasters

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `IGameplayTagBroadcaster` | Native Interface | Register/Unregister an `FOnGameplayTagDelegate` callback for tag broadcasts. | `Public/Broadcasters/GameplayTagBroadcaster.h`. |
| `UGameplayTagBroadcasterComponent` | `UActorComponent` | Concrete component exposing a BlueprintAssignable multicast delegate plus `Broadcast(Tag)`. | `Public/Broadcasters/GameplayTagBroadcasterComponent.h`. |

### GameEvent

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `IGameEventTaskHandler` | Native Interface | Contract to fetch a `UGameEventTaskManager*` by gameplay tag. | `Public/GameEvent/GameEventTaskHandler.h`. |
| `UGameEventTaskManager` | `UObject` | Tracks a set of objects/async tasks that must all complete before firing `OnManagerComplete`; supports phased async tasks via `AsyncTaskMap` keyed by phase int. | `Public/GameEvent/GameEventTaskManager.h`. Key API: `Create()`, `RegisterTask`, `RegisterAsyncTask`, `UnregisterTask`, `InitiateAllTasks`, `OnManagerBegin`/`OnManagerComplete`. |
| `UGameEventTaskSubsystem` | `UWorldSubsystem`, `IGameEventTaskHandler` | Per-world registry of tag-keyed `UGameEventTaskManager`s; a genuine serialized FIFO queue (`QueueTagContainer`) is the primary way to fire a tag from outside this class -- the underlying single-tag trigger (`TriggerTag`) is private. | `Public/GameEvent/GameEventTaskSubsystem.h`. Extensively documented. Key API: `RegisterAsyncTask`, `QueueTagContainer`, `BindOnTagBegin`/`BindOnTagComplete`, `GetGameEventTaskManagerByTag_Implementation`. `QueueTagSequence` (an older multi-step chaining primitive) is deprecated -- kept, not deleted, with zero remaining callers in this project. This is the central class powering the project-level "Gated Event Sequencing via Tags" reusable workflow — see [GameplayTag-EventSequencing.md](../../../Source/ConnectIt/Docs/Workflows/GameplayTag-EventSequencing.md). |
| `UGameEventTask_Async` | `UObject` | Simple async-task payload object. | `Public/GameEvent/GameEventTask_Async.h`. Key API: `OnComplete` delegate, `OnExecuteDelegate`, `bIsPersistentTask` flag. |

### GameTurn

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `UGameTurnHandler` | `UActorComponent` | Turn-start/turn-end pre/post-async delegate hub, wired to two `UGameEventTaskManager`s. | `Public/GameTurn/GameTurnHandler.h`. **Known bug**: line 83 declares `UPROPERTY(EditAnywhere, BlueprintreadWrite, ...)` — the `BlueprintreadWrite` specifier has a casing typo (should be `BlueprintReadWrite`), which is case-sensitive to UHT. See Known Discrepancies. |
| `UGameTurnParticipant` | `UObject` | Turn begin/end/pause/resume delegates as a plain UObject. | `Public/GameTurn/GameTurnParticipant.h`. Near-duplicate of `UGameTurnParticipantComponent` below — see Known Discrepancies. |
| `UGameTurnParticipantComponent` | `UActorComponent` | Same 4 delegates (Begin/End/Pause/Resume turn) as `UGameTurnParticipant`, but as a component, with a different delegate typedef and no shared interface between the two. | `Public/GameTurn/GameTurnParticipantComponent.h`. See Known Discrepancies. |
| `UGameTurnTracker` | `UObject` | Trivial turn counter. | `Public/GameTurn/GameTurnTracker.h`. Key API: `Reset`, `Increment`, `GetCount`. |

### Pooling

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `UActorPool` | `UObject` | Per-class actor pool (active/inactive sets), grows on demand. | `Public/Pooling/ActorPool.h`. Used by the current consuming project's piece-spawn interpreter. |
| `IActorPoolInterface` | Native Interface | `ActivatePoolObject`/`DeactivatePoolObject` BlueprintImplementableEvents for pooled actors to hook their own show/hide logic. | `Public/Pooling/ActorPoolInterface.h`. |
| `UActorPoolSubsystem` | `UWorldSubsystem` | World-level registry of `UActorPool`s keyed by actor class, plus reverse actor-to-pool lookup for `ReleaseObject`. | `Public/Pooling/ActorPoolSubsystem.h`. |

### Scoring / State / Library / Misc

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `UGameMechanics_ScoringLibrary` | `UBlueprintFunctionLibrary` | `FObjectScore` struct plus a generic `GetSortedKeysByValue<K,V>` template and `ScoreSelect_TopN`/`ScoreModify_RandomNoise` selection/modifier helpers. | `Public/Scoring/GameMechanics_ScoringLibrary.h`. |
| `UGameMechanicsStateBase` | `UObject` (Abstract) | State-machine node: `Enter()`/`Exit()` public wrappers calling protected `Enter_Internal`/`Exit_Internal` BlueprintNativeEvents; `OnGameStateEnter`/`OnGameStateExit` delegates. | `Public/State/GameMechanicsStateBase.h`. Different pattern from `UGameMechanicsStateSimple` below — see Known Discrepancies. |
| `UGameMechanicsStateSimple` | `UObject` | State-machine node with a simpler pattern: `Enter()`/`Exit()` are directly BlueprintNativeEvents (no internal-wrapper indirection), single `OnStateComplete` delegate, not marked Abstract. | `Public/State/GameMechanicsStateSimple.h`. A legacy state machine in the current consuming project derives from this simpler class, not `GameMechanicsStateBase` — see Known Discrepancies. |
| `IGameStateHandlerInterface` | Native Interface | `GetGameStateTag()` accessor contract. | `Public/State/GameStateHandlerInterface.h`. |
| `UGameMechanics_Framework` | `UBlueprintFunctionLibrary` | `GetGameInstance`/`GetGameMode` world-context convenience wrappers. | `Public/Library/GameMechanics_Framework.h`. |
| `EGamePlayerType` (enum) | — | `Player`/`Opponent`/`Observer`. | `Public/UnrealGameMechanicsEnums.h`. |
| `FGameplayTagValuePair`, `FGameplayTagValueProbability` (structs) | — | Generic tag+value(+probability) pair structs for weighted-tag systems. | `Public/UnrealGameMechanicsStructs.h`. |

## Known Discrepancies / Issues in This Plugin

- `UGameTurnHandler` has a casing typo on a `UPROPERTY` specifier (`BlueprintreadWrite` instead of `BlueprintReadWrite`) at `Public/GameTurn/GameTurnHandler.h` line 83. See the project-level [naming conventions notes](../../../Source/ConnectIt/Docs/Conventions.md#discrepancies) #2.
- `UGameTurnParticipant` and `UGameTurnParticipantComponent` are near-duplicate turn-participant delegate hubs with no shared interface. See the project-level [duplication notes](../../../Source/ConnectIt/Docs/Duplication.md).
- `UGameMechanicsStateBase` and `UGameMechanicsStateSimple` are two inconsistent state-machine base-class patterns, and a legacy state machine in the current consuming project derives from the simpler one rather than the Abstract base. See the project-level [duplication notes](../../../Source/ConnectIt/Docs/Duplication.md).
- UFUNCTION `Category` string inconsistency across the plugin. See the project-level [naming conventions notes](../../../Source/ConnectIt/Docs/Conventions.md).
