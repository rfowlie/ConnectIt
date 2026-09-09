---
schema: code
reconciled: 2026-09-06
commit: ec992a2
---

# UnrealGameMechanics — code index

Per-type help pages for the plugin's public API. Governed by [[_schema/code|_schema/code.md]].
An **overlay** on the authoritative in-repo reference
([`Docs/README.md`](../../../Plugins/UnrealGameMechanics/Docs/README.md),
[`Docs/Systems.md`](../../../Plugins/UnrealGameMechanics/Docs/Systems.md)) — entry points,
collaborators, gotchas, cross-impact, freshness. It does not restate them.

Source root: `Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/`
`status`: **current** = written & reconciled · **stub** = no page yet.

| Type | Kind | Area | Role | Source (`Public/…`) | Status |
|---|---|---|---|---|---|
| [[UnrealGameMechanics/code/UGameEventTaskSubsystem\|UGameEventTaskSubsystem]] | UCLASS (UWorldSubsystem) | GameEvent | primary | `GameEvent/GameEventTaskSubsystem.{h,cpp}` | current |
| [[UnrealGameMechanics/code/UGameEventTaskManager\|UGameEventTaskManager]] | UCLASS (UObject) | GameEvent | primary | `GameEvent/GameEventTaskManager.{h,cpp}` | current |
| [[UnrealGameMechanics/code/UGameEventTask_Async\|UGameEventTask_Async]] | UCLASS (UObject) | GameEvent | primary | `GameEvent/GameEventTask_Async.{h,cpp}` | current |
| [[UnrealGameMechanics/code/UActorPoolSubsystem\|UActorPoolSubsystem]] | UCLASS (UWorldSubsystem) | Pooling | primary | `Pooling/ActorPoolSubsystem.{h,cpp}` | current |
| [[UnrealGameMechanics/code/UInputTagBinder\|UInputTagBinder]] | UCLASS (UObject) | Input | primary | `Input/InputTagBinder.{h,cpp}` | current |
| [[UnrealGameMechanics/code/UStackedStateMachine\|UStackedStateMachine]] | UCLASS (UObject) | State | primary | `State/StackedStateMachine.{h,cpp}` + `State/StackedState.{h,cpp}` | current |
| UActorPool | UCLASS (UObject) | Pooling | primary | `Pooling/ActorPool.{h,cpp}` | covered in UActorPoolSubsystem |
| IActorPoolInterface | UINTERFACE | Pooling | primary (impl to customise) | `Pooling/ActorPoolInterface.h` | covered in UActorPoolSubsystem |
| FInputTagBinding | USTRUCT | Input | primary (data) | `Input/InputTagBinding.h` | covered in UInputTagBinder |
| UStackedState | UCLASS (abstract) | State | primary | `State/StackedState.h` | covered in UStackedStateMachine |
| IGameEventTaskHandler | UINTERFACE | GameEvent | internal (orphaned) | `GameEvent/GameEventTaskHandler.h` | stub |
| FAsyncTaskArray | USTRUCT | GameEvent | internal | `GameEvent/GameEventTaskManager.h` | stub |
| UDWidget_GameEventTaskSubsystem | UCLASS (abstract) | GameEvent/Debug | internal (tooling) | `Debug/DWidget_GameEventTaskSubsystem.h` | stub |
| UGameTurnHandler | UCLASS (UActorComponent) | GameTurn | primary | `GameTurn/GameTurnHandler.{h,cpp}` | stub |
| UGameTurnTracker | UCLASS (UObject) | GameTurn | primary (counter) | `GameTurn/GameTurnTracker.h` | stub |
| UGameTurnParticipant / UGameTurnParticipantComponent | UCLASS | GameTurn | internal (near-duplicate) | `GameTurn/GameTurnParticipant*.h` | stub |
| UGameplayTagBroadcasterComponent | UCLASS (component) | Broadcasters | primary | `Broadcasters/GameplayTagBroadcasterComponent.{h,cpp}` | stub |
| IGameplayTagBroadcaster | UINTERFACE | Broadcasters | primary | `Broadcasters/GameplayTagBroadcaster.h` | stub |
| UGameMechanics_ScoringLibrary | UCLASS (UBlueprintFunctionLibrary) | Scoring | primary | `Scoring/GameMechanics_ScoringLibrary.{h,cpp}` | stub |
| `GetSortedKeysByValue<K,V>` / FObjectScore | free template / USTRUCT | Scoring | primary / internal | `Scoring/GameMechanics_ScoringLibrary.h` | covered in ScoringLibrary (stub) |
| UStackedState machine base pair (`UGameMechanicsStateBase`, `…Simple`, `IGameStateHandlerInterface`) | UCLASS / UINTERFACE | State | primary (single-state pattern) | `State/GameMechanicsState*.h`, `State/GameStateHandlerInterface.h` | stub |
| UDWidgetBase | UCLASS (abstract UUserWidget) | Widget | primary (debug-widget base) | `Widget/DWidgetBase.h` | stub |
| UGameMechanics_Framework | UCLASS (UBlueprintFunctionLibrary) | Library | internal (helper) | `Library/GameMechanics_Framework.{h,cpp}` | stub |
| Enums (`EGamePlayerType`) / Structs (`UnrealGameMechanicsStructs.h`) | UENUM / USTRUCT | Top-level | primary | `UnrealGameMechanicsEnums.h`, `UnrealGameMechanicsStructs.h` | stub |
| FUnrealGameMechanicsModule | class (IModuleInterface) | Top-level | internal | `UnrealGameMechanics.h` | stub |

## Related

- Flows: [[UnrealGameMechanics/systems/index|systems/index.md]]
- Recipes: [[UnrealGameMechanics/recipes/index|recipes/index.md]]
