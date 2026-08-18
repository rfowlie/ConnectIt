# UnrealGameMechanics

## Purpose

UnrealGameMechanics is a generic, reusable gameplay-mechanics scaffolding layer: gated/sequenced async task execution keyed by gameplay tags, turn bookkeeping primitives, a generic actor object pool, a simple state-machine base, and scoring helpers. It exists as a separate plugin so that mechanics which are broadly applicable to many turn-based / phase-based games (not tied to any one specific board game) can be developed and reused independently of any one consumer's rules. It is the backbone that `UnrealTurnBasedMechanics` and any consuming project's own board-sequencing/turn-resolution workflows are built on top of.

## Module(s)

| Module | Type | LoadingPhase | Notes |
|---|---|---|---|
| `UnrealGameMechanics` | Runtime | Default | Single module; entry point `FUnrealGameMechanicsModule` (`Public/UnrealGameMechanics.h`). |

## Key Dependencies

- Depends on: `Core`, `GameplayTags`, `InputCore`, `EnhancedInput`, `UMG` (Public); `CoreUObject`, `Engine`, `Slate`, `SlateCore` (Private). No plugin-level dependencies declared in the `.uplugin`. `UMG` was added specifically for `Widget/DWidgetBase.h` (see Classes below) — it's a Public dependency because that header is included by other modules that subclass it.
- Depended on by: `UnrealTurnBasedMechanics` (Public dependency in its `.Build.cs`) and the consuming game module (not named here, to keep this plugin's documentation project-agnostic) — that consumer's board sequencing/turn-resolution workflows are built directly on this plugin's `UGameEventTaskSubsystem`.

## When to Use It

- Gating a sequence of async steps behind gameplay tags so that dependent logic only fires once every registered task/object reports completion (see `GameEvent/`).
- Queuing one or more tag-groups to fire in order via a serialized FIFO queue (`QueueTagContainer`) -- a caller with several events to fire in order just calls it once per event, back to back; the queue's own ordering does the rest.
- Tracking turn state (turn counters, begin/end/pause/resume delegates) for a turn-based or phase-based game.
- Layering input bindings onto a consumer with an explicit, dynamic bind/unbind lifetime rather than EnhancedInput's usual always-on-at-`BeginPlay` assumption (see `Input/`).
- Pooling actors that are spawned/despawned frequently (board pieces, projectiles, VFX actors).
- Building either a single-state machine node (`State/` — `GameMechanicsStateBase`/`Simple`) or an actual stack-based state machine for nested/resumable sub-flows within a larger action or controller (`State/` — `StackedState`/`StackedStateMachine`) that doesn't need a full Gameplay Ability System / Behavior Tree.
- Scoring and ranking a set of candidate objects generically (top-N selection, weighted random noise).

See [Systems.md](Systems.md) for a narrative, system-by-system walkthrough of how these fit together and this plugin's own internal conventions — this README stays the exhaustive per-class reference.

## When NOT to Use It / Scope Boundaries

- AI decision-making, tree search, and influence-map visualization belong in `UnrealGameIntelligence`, not here.
- Utility-AI action scoring/evaluation (weighted GameplayTag-based action selection) belongs in `UnrealAIMechanics`, not here.
- Turn-based-specific action framework classes (the actual `TurnBasedAction`/`TurnBasedActionsComponent` types) belong in `UnrealTurnBasedMechanics`, which builds on top of this plugin rather than living inside it.
- Small, generic, non-gameplay C++ helpers (e.g. authority checks) belong in `UnrealCodingUtils`, not here.
- A consuming project's own board/piece logic should consume this plugin's generic primitives (`UGameEventTaskSubsystem`, `UActorPool`, etc.) rather than reimplementing gating/pooling/turn-tracking inside its own game module.

## Notable Design Patterns

- Gated task/phase-barrier pattern (`GameEvent/`): `UGameEventTaskManager` tracks a set of tasks that must ALL complete before firing `OnManagerComplete`, and `UGameEventTaskSubsystem` layers a per-tag registry plus a serialized FIFO queue (`QueueTagContainer`) on top -- the actual single-tag trigger (`TriggerTag`) is private, reachable only internally, so every external caller goes through the queue. This is the reusable core of the project-level "Gated Event Sequencing via Tags" workflow — see [GameplayTag-EventSequencing.md](../../../Source/ConnectIt/Docs/Workflows/GameplayTag-EventSequencing.md).
- Tag-broadcast pattern: `IGameplayTagBroadcaster` / `UGameplayTagBroadcasterComponent` expose a simple register/unregister + multicast-delegate broadcast keyed by gameplay tag.
- Stack-based state pattern (`State/`): `UStackedStateMachine` owns an actual stack of `UStackedState` entries — push one on top (pausing or fully tearing down whatever's beneath), pop back down, or replace the whole stack — for nested/resumable sub-flows a single-state model can't express. See [Systems.md](Systems.md) for the full picture, including how it relates to the older single-state pattern below.
- UFUNCTION `Category` strings are inconsistent across the plugin (e.g. `"Game Mechanics | Turn"`, `"GameMechanics|Framework"`, `"Actor Pool"`, `"Turn Completion"`, `"Scoring | Selectors"` — mixed spacing/style). See the project-level [naming conventions notes](../../../Source/ConnectIt/Docs/Conventions.md) and [Systems.md](Systems.md) for this plugin's own fuller convention notes.

## Classes

### Broadcasters

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `IGameplayTagBroadcaster` | Native Interface | Register/Unregister an `FOnGameplayTagDelegate` callback for tag broadcasts. | `Public/Broadcasters/GameplayTagBroadcaster.h`. |
| `UGameplayTagBroadcasterComponent` | `UActorComponent` | Concrete component exposing a BlueprintAssignable multicast delegate plus `Broadcast(Tag)`. | `Public/Broadcasters/GameplayTagBroadcasterComponent.h`. |

### GameEvent

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `IGameEventTaskHandler` | Native Interface | Contract to fetch a `UGameEventTaskManager*` by gameplay tag. | `Public/GameEvent/GameEventTaskHandler.h`. **Orphaned**: `UGameEventTaskSubsystem` no longer implements this (see below), and nothing else in the plugin does either. See [Systems.md](Systems.md#known-rough-edges). |
| `UGameEventTaskManager` | `UObject` | Tracks a set of objects/async tasks that must all complete before firing `OnManagerComplete`; supports phased async tasks via `AsyncTaskMap` keyed by phase int. | `Public/GameEvent/GameEventTaskManager.h`. Key API: `Create()`, `RegisterTask`, `RegisterAsyncTask`, `UnregisterTask`, `InitiateAllTasks`, `OnManagerBegin`/`OnManagerComplete`. |
| `UGameEventTaskSubsystem` | `UWorldSubsystem` | Per-world registry of tag-keyed `UGameEventTaskManager`s; a serialized FIFO queue (`QueueTagContainer`) is the sole public way to fire a tag from outside this class — every tag in one queued container fires in parallel, the queue only advances to the next container once every tag in the current one has fully completed. The underlying single-tag trigger (`TriggerTag`) is private. | `Public/GameEvent/GameEventTaskSubsystem.h`. Extensively documented. Key API: `QueueTagContainer`, `RegisterAsyncTask`, `BindOnTagBegin`/`BindOnTagComplete`, `GetTagsInQueue`. Note: despite `#include`-ing `GameEventTaskHandler.h`, this class no longer inherits `IGameEventTaskHandler` — that interface is now orphaned (see above). An older multi-step chaining primitive, `QueueTagSequence`, has been fully removed from this class (only a few stale comments in the header still name it); `QueueTagContainer` is its full replacement. This is the central class powering the project-level "Gated Event Sequencing via Tags" reusable workflow — see [GameplayTag-EventSequencing.md](../../../Source/ConnectIt/Docs/Workflows/GameplayTag-EventSequencing.md). |
| `UGameEventTask_Async` | `UObject` | Simple async-task payload object. | `Public/GameEvent/GameEventTask_Async.h`. Key API: `OnComplete` delegate, `OnExecuteDelegate`, `bIsPersistentTask` flag. |

### GameTurn

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `UGameTurnHandler` | `UActorComponent` | Turn-start/turn-end pre/post-async delegate hub, wired to two `UGameEventTaskManager`s. | `Public/GameTurn/GameTurnHandler.h`. **Known bug**: line 83 declares `UPROPERTY(EditAnywhere, BlueprintreadWrite, ...)` — the `BlueprintreadWrite` specifier has a casing typo (should be `BlueprintReadWrite`), which is case-sensitive to UHT. See Known Discrepancies. |
| `UGameTurnParticipant` | `UObject` | Turn begin/end/pause/resume delegates as a plain UObject. | `Public/GameTurn/GameTurnParticipant.h`. Near-duplicate of `UGameTurnParticipantComponent` below — see Known Discrepancies. |
| `UGameTurnParticipantComponent` | `UActorComponent` | Same 4 delegates (Begin/End/Pause/Resume turn) as `UGameTurnParticipant`, but as a component, with a different delegate typedef and no shared interface between the two. | `Public/GameTurn/GameTurnParticipantComponent.h`. See Known Discrepancies. |
| `UGameTurnTracker` | `UObject` | Trivial turn counter. | `Public/GameTurn/GameTurnTracker.h`. Key API: `Reset`, `Increment`, `GetCount`. |

### Input

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `UInputTagBinder` | `UObject` | Builds and owns a single `UInputMappingContext` from a designer-authored `TArray<FInputTagBinding>`, scoped to an explicit `Initialise` → `BindAll`/`UnbindAll` lifetime instead of EnhancedInput's usual always-on-at-`BeginPlay` assumption — built for consumers whose own lifetime is itself dynamic (e.g. one entry on an action stack). Every bound `InputAction` funnels through one shared `OnInputTagTriggered` delegate carrying the matching entry's `BindingTag`, so a consumer switches on the tag instead of binding one function per input. | `Public/Input/InputTagBinder.h`. Key API: `Initialise`, `BindAll`, `UnbindAll`, `GetMappingContext` (`BlueprintPure`, lets a consumer layer one more key mapping onto the same context), `OnInputTagTriggered` delegate. Plain `UObject`, not a component/subsystem — both EnhancedInput dependencies are injected via `Initialise`, never resolved via `GetWorld()`. |
| `FInputTagBinding` (+`FOnInputTagTriggered`, `FOnInputTriggered`) | Struct / delegates | One configured input trigger: `BindingTag`, `InputAction`, `TriggerEvent`, `Key`. Adding a new bound input is purely data — no new C++ required per binding. | `Public/Input/InputTagBinding.h`. Has an open TODO questioning whether `BindingTag` needs to be passed at all alongside `FInputActionInstance`, or whether the instance alone could be used to branch. `FOnInputTriggered` is declared but not used by `UInputTagBinder` (which uses the two-param `FOnInputTagTriggered` instead). |

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
| `FGameplayTagValuePair`, `FGameplayTagValueProbability` (structs) | — | Generic tag+value(+probability) pair structs for weighted-tag systems. | `Public/UnrealGameMechanicsStructs.h`. `FGameplayTagValueProbability::operator==` takes an `FGameplayTagValuePair` parameter rather than its own type — likely a copy-paste artifact, harmless since nothing currently invokes it that way. |

### State/ — Stacked State Machine

A second, structurally distinct state pattern from the single-state pair above — see [Systems.md](Systems.md#state-management) for how the two relate and when to reach for which.

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `UStackedState` | `UObject` (Abstract) | One entry on a `UStackedStateMachine`'s stack. `Initialize()`/`Deinitialize()` (`BlueprintNativeEvent`) set up/undo whatever the state needs active while it's on top. Never mutates the stack itself — instead broadcasts one of three transition-request delegates (`OnSetBaseState`, `OnPushState`, `OnPopState`) that the owning machine binds to. | `Public/State/StackedState.h`. Ported from an external project's existing stack-based state pattern, kept functionally as-is and renamed to be project-agnostic; the source project's own input-handling interface dependency was deliberately **not** ported (unknown contract) — concrete states bind whatever input they need themselves via `Initialize`/`Deinitialize`. |
| `UStackedStateMachine` | `UObject` | Owns the actual `StateStack` (`TArray<TObjectPtr<UStackedState>>`). `SetBaseState` empties the stack and pushes one entry; `PushState` layers a new entry on top (optionally deinitializing what's underneath, or leaving it running/paused beneath); `PopState` returns to whatever's below the top (refuses to pop the last remaining entry). `PeakState` reads the top without popping. | `Public/State/StackedStateMachine.h`. Also a verbatim port, "logic unchanged" per its own header comment. **Known rough edges** (all carried over from the port, not introduced by it): `PeakState` is a verbatim misspelling of "Peek"; `PopState()` calls `Deinitialize()` directly rather than `DeinitializeState()`, so a popped state's three transition delegates stay bound to the machine instead of being unbound (self-flagged in a `.cpp` comment as worth revisiting); `SwapState()` is public/`BlueprintCallable` but an unimplemented stub (TODO). See [Systems.md](Systems.md#known-rough-edges). |

### Widget / Debug

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `UDWidgetBase` | `UUserWidget` (Abstract) | Shared scaffolding for the "one debug widget per tracked class" family: `RefreshAll()`, `OnDebugStateUpdated()` (BlueprintImplementableEvent), and `BindDelegates`/`UnbindDelegates`/`RefreshFields` virtuals a concrete subclass overrides. Has no dependency on anything project-specific. | `Public/Widget/DWidgetBase.h`. See the consuming project's [DebugWidgets.md](../../../Source/ConnectIt/Docs/Workflows/DebugWidgets.md) workflow doc for the full pattern and the complete class↔widget↔plugin mapping. |
| `UDWidget_GameEventTaskSubsystem` | `UDWidgetBase` | Tracks `UGameEventTaskSubsystem` -- caches the tags currently firing in its active container (`GetTagsInQueue()`), refreshes on `OnActiveManagerTagsChanged`. | `Public/Debug/DWidget_GameEventTaskSubsystem.h`. Lives here (not in a consuming game module) because the tracked class is defined in this plugin. |

## Known Discrepancies / Issues in This Plugin

- `UGameTurnHandler` has a casing typo on a `UPROPERTY` specifier (`BlueprintreadWrite` instead of `BlueprintReadWrite`) at `Public/GameTurn/GameTurnHandler.h` line 83. See the project-level [naming conventions notes](../../../Source/ConnectIt/Docs/Conventions.md#discrepancies) #2.
- `UGameTurnParticipant` and `UGameTurnParticipantComponent` are near-duplicate turn-participant delegate hubs with no shared interface. See the project-level [duplication notes](../../../Source/ConnectIt/Docs/Duplication.md).
- `UGameMechanicsStateBase` and `UGameMechanicsStateSimple` are two inconsistent state-machine base-class patterns, and a legacy state machine in the current consuming project derives from the simpler one rather than the Abstract base. See the project-level [duplication notes](../../../Source/ConnectIt/Docs/Duplication.md).
- UFUNCTION `Category` string inconsistency across the plugin. See the project-level [naming conventions notes](../../../Source/ConnectIt/Docs/Conventions.md) and [Systems.md](Systems.md#conventions) for this plugin's own verbatim examples.
- `IGameEventTaskHandler` is orphaned — `UGameEventTaskSubsystem` no longer implements it and nothing else does either. See [Systems.md](Systems.md#known-rough-edges).
- `UStackedStateMachine::PopState()` doesn't unbind the popped state's transition delegates (calls `Deinitialize()` rather than `DeinitializeState()`) — a self-flagged, intentionally-preserved carry-over from the ported source. `SwapState()` is a public, `BlueprintCallable`, unimplemented stub. See [Systems.md](Systems.md#known-rough-edges).
