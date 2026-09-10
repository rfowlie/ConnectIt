---
schema: code
reconciled: 2026-09-10
commit: ec992a2
---

# UnrealGameMechanics — code index

Inventory of every public type + a map of the module. Governed by
[[_core/_schema/_code|_core/_schema/_code.md]]. Domain overview:
[[UnrealGameMechanics/CLAUDE|CLAUDE.md]].

`status`: **current** = page written & reconciled · **stub** = no page yet · **covered
in X** = documented inside another type's page.

## Inventory

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
| Single-state pattern (`UGameMechanicsStateBase`, `…Simple`, `IGameStateHandlerInterface`) | UCLASS / UINTERFACE | State | primary | `State/GameMechanicsState*.h`, `State/GameStateHandlerInterface.h` | stub |
| UDWidgetBase | UCLASS (abstract UUserWidget) | Widget | primary (debug-widget base) | `Widget/DWidgetBase.h` | stub |
| UGameMechanics_Framework | UCLASS (UBlueprintFunctionLibrary) | Library | internal (helper) | `Library/GameMechanics_Framework.{h,cpp}` | stub |
| Enums (`EGamePlayerType`) / Structs | UENUM / USTRUCT | Top-level | primary | `UnrealGameMechanicsEnums.h`, `UnrealGameMechanicsStructs.h` | stub |
| FUnrealGameMechanicsModule | class (IModuleInterface) | Top-level | internal | `UnrealGameMechanics.h` | stub |

## Map

One Runtime module, `Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/`. Sub-areas:

- **`GameEvent/`** — the plugin's centrepiece and what most consumers use. A
  **phase-barrier**: `UGameEventTaskManager` holds tasks that must all complete before it
  fires `OnManagerComplete`; `UGameEventTaskSubsystem` (`UWorldSubsystem`) adds a
  per-gameplay-tag registry + serialized FIFO queue (`QueueTagContainer`). See
  [[UnrealGameMechanics/code/systems/gated-event-tag-queue|systems/gated-event-tag-queue]].
  `UnrealTurnBasedMechanics`' turn-end gate is a consumer.
- **`Pooling/`** — `UActorPoolSubsystem` + `UActorPool` + `IActorPoolInterface`. Get →
  activate → use → deactivate → release; get/activate are **separate calls** so callers
  control activation timing. See
  [[UnrealGameMechanics/code/systems/actor-pooling-lifecycle|systems/actor-pooling-lifecycle]].
- **`Input/`** — `UInputTagBinder`: one mapping context built from data-only
  `FInputTagBinding` entries, scoped to an explicit `BindAll()`/`UnbindAll()` lifetime;
  every action dispatches through one `OnInputTagTriggered` delegate keyed by tag.
- **`State/`** — two independent patterns: single-state (`UGameMechanicsStateBase` /
  `…Simple`) and stack-based (`UStackedStateMachine` / `UStackedState`).
- **`GameTurn/`, `Scoring/`, `Broadcasters/`, `Widget/` (`UDWidgetBase`), `Debug/`** —
  smaller helpers; see the inventory.

**Start at:** `UGameEventTaskSubsystem` → the two `systems/` flows.

## Related

- Flows: [[UnrealGameMechanics/code/systems/index|code/systems/index.md]]
- Recipes: [[UnrealGameMechanics/code/recipes/index|code/recipes/index.md]]
