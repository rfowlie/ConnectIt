# UnrealTurnBasedMechanics

## Purpose

UnrealTurnBasedMechanics is a full, network-replicated turn-based game framework: a designer-configurable action stack per participant (root/idle/spectator/pause actions plus runtime "turn actions" with cooldowns and completions), a match-phase/turn-phase state machine with ready-check, timeout, forfeit, and reconnect handling, pluggable turn-order strategies, and a controller-coordinator "mediator" pattern that bridges Player and AI controllers to a shared actions component. It is its own plugin because turn/match orchestration is generic to any turn-based multiplayer game — it has no knowledge of grids, pieces, or any specific game's rules, and depends on `UnrealGridMechanics` only for the grid-tile selection pipeline used by `UTurnBasedAction`, not the other way around. Keeping it separate lets the turn/action/replication machinery be reused independently of any one consumer's board logic.

## Module(s)

| Module | Type | LoadingPhase | Notes |
|---|---|---|---|
| `UnrealTurnBasedMechanics` | Runtime | Default | Turn/action/match framework. Only plugin among the project's plugins with its own dedicated log category (`LogTurnBasedMechanics`). |

## Key Dependencies

- **Depends on:**
  - Plugin-level: `EnhancedInput`, `UnrealGridMechanics`
  - Public: `Core`, `InputCore`, `EnhancedInput`, `GameplayTags`, `UnrealGridMechanics`, `UnrealGameMechanics`
  - Private: `CoreUObject`, `Engine`, `Slate`, `SlateCore`, `UMG`, `EnhancedInput`, `AIModule`, `UnrealGridMechanics`, `UnrealGameMechanics`
- **Depended on by:**
  - The consuming game module (not named here, to keep this plugin's documentation project-agnostic) — nearly every controller, game mode, game state, and player state class there derives from or composes with this plugin's classes.

Notes: `UnrealTurnBasedMechanics.Build.cs` lists `UnrealGridMechanics` (and `UnrealGameMechanics`) in both `PublicDependencyModuleNames` and `PrivateDependencyModuleNames`, which is redundant — the Public listing alone already satisfies private use. Minor, not functionally harmful.

## When to Use It

- Building a turn-based match's controller/game-mode/game-state/player-state scaffolding (`ATurnBasedPlayerControllerBase`, `ATurnBasedAIController`, `ATurnBasedGameMode`, `ATurnBasedGameState`, `ATurnBasedPlayerState`).
- Defining a per-turn player action with cooldowns, input bindings, and grid-tile hover/selection validation (`UTurnBasedAction`).
- Defining a non-turn-owning controller view such as idle, spectator, or pause (`UTurnBasedSpectatorAction`, `UTurnBasedPauseAction`).
- Needing a designer-facing config asset that assembles a participant's permitted actions (`UActionLoadoutDataAsset`).
- Needing match-level state machine behavior: ready checks, turn timeouts, forfeit-on-disconnect, reconnect handling, a gameplay-tag-gated turn-end sequence (`UTurnBasedParticipantManagerComponent`).
- Needing a pluggable turn order (sequential or random, or a custom `ITurnOrderInterface` implementation).
- Extending turn actions or modifiers with project-specific payloads via `FInstancedStruct Payload` on `FTurnActionRequest`/`FTurnModifier`.
- Blocking a participant's action stack while a board-change request it just sent is in flight, and resolving it -- complete on success, reactivate to retry on failure -- once the server answers (`UTurnBasedActionsComponent::NotifyBoardChangeOutcome`, `AwaitingConfirmationActionClass`).

See [Systems.md](Systems.md) for a narrative, system-by-system walkthrough of how these fit together and this plugin's own internal conventions — this README stays the exhaustive per-class reference.

## When NOT to Use It / Scope Boundaries

- Grid/board representation, tile and piece actors, and grid math belong in `UnrealGridMechanics`, not here — this plugin only consumes grid-tile selection via `UTurnBasedAction`'s Enhanced Input pipeline; it does not own grid concepts itself.
- Concrete gameplay actions (piece placement rules, shift rules, win-condition checks) belong in the consuming game module as subclasses of `UTurnBasedAction` — this plugin should only ever provide the generic action lifecycle, cooldown, and input-binding scaffolding, never game-specific rule logic.
- AI decision-making/heuristics belong outside this plugin — `ATurnBasedAIController` provides the controller scaffolding and PlayerState auto-creation, but actual move-selection intelligence belongs in a dedicated AI module/plugin, not in this framework.
- UI/widget content (beyond the generic `PauseWidgetClass` hook on `UTurnBasedPauseAction`) belongs in the consuming project's UMG assets, not as new C++ widget logic here.
- New turn-order algorithms should implement `ITurnOrderInterface` rather than being hardcoded into `UTurnBasedParticipantManagerComponent` — the strategy pattern exists specifically so ordering logic doesn't need to live in the state machine.

## Notable Design Patterns

- **Dual delegate declaration**: nearly every delegate is declared twice — once as a dynamic multicast (`BlueprintAssignable`) for Blueprint/replication use, and once as a plain "_Native" multicast delegate for C++-only, lambda-friendly, higher-performance binding. This is a deliberate convention and arguably the pattern other plugins in this suite should adopt too. See the project-level [naming conventions notes](../../../Source/ConnectIt/Docs/Conventions.md).
- **Mediator/coordinator pattern**: `UTurnBasedControllerCoordinatorComponent` bridges `UTurnBasedParticipantComponent` and `UTurnBasedActionsComponent`, added identically to both `ATurnBasedPlayerControllerBase` and `ATurnBasedAIController` since `APlayerController`/`AAIController` cannot share a common intermediate base class.
- **Strategy pattern**: turn order is pluggable via `ITurnOrderInterface`, with `USequentialTurnOrderStrategy` (default) and `URandomTurnOrderStrategy` as built-in implementations.
- **Generic project-extension hook**: `FInstancedStruct Payload` on `FTurnActionRequest`/`FTurnModifier` lets any consumer attach project-specific data without modifying this plugin — the struct comments reference a consumer's own project-specific types as the intended usage example, so this is a deliberate extension point, not orphaned generality.
- **Gated turn-end sequencing**: turn-end advancement no longer uses a bespoke hold-counter API. `UTurnBasedParticipantManagerComponent::EndTurn` enqueues a single designer-set `TurnEndEventTag` on `UnrealGameMechanics`' `UGameEventTaskSubsystem` (`QueueTagContainer`), and `AdvanceToNextParticipant` is bound as that tag's completion handler (`BindOnTagComplete`) in `BeginPlay` -- it only runs once every task any system has registered against that tag has finished, or immediately if none are registered (or if `TurnEndEventTag` is left unset, which also logs an error). This replaced an earlier `BeginResolutionHold`/`EndResolutionHold` counter API, itself a replacement for a still-earlier fixed-duration `TurnResolutionDuration` timer -- neither exists in current source. Turn-end is now just one more instance of the same tag-gating pattern this subsystem provides everywhere else; see the project-level [Gated Event Sequencing via Tags workflow](../../../Source/ConnectIt/Docs/Workflows/GameplayTag-EventSequencing.md) and [Systems.md](Systems.md#turn-order--the-participant-state-machine) for the full mechanism.
- **Awaiting-confirmation guard**: `UTurnBasedActionsComponent` blocks its own stack mutation -- `PushAction`, `SafePopAction`, `TryPushActionByRef`, and `ClearAndPush` all no-op -- for as long as `bAwaitingRequestConfirmation` is true, i.e. between firing `OnBoardChangeRequested` and the caller reporting the server's verdict back via `NotifyBoardChangeOutcome`. An optional `AwaitingConfirmationActionClass` action can be pushed for the duration to give the player mid-request feedback. This is this plugin's side of the project-level [Server-Authoritative Action Request workflow](../../../Source/ConnectIt/Docs/Workflows/ServerAuthoritative-ActionRequest.md); see [Systems.md](Systems.md#the-action-stack) for more.
- This plugin is the best-documented in the project (dense inline rationale comments throughout) — worth treating as the reference model for documentation style when writing or reviewing other plugins' code comments.

## Classes

### Action

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `UTurnBasedActionBase` | `UObject` (Abstract) | Root of the action hierarchy: `Activate`/`ForceDeactivate` lifecycle, with `Activate_Internal`/`Deactivate_Internal`/`ForceDeactivate_Internal`/`PostInitialiseAction` BlueprintNativeEvent hooks and both dynamic and "_Native" delegate variants. | Key API: `Activate`, `ForceDeactivate`, `IsActive`, `GetPlayerController`. |
| `UTurnBasedAction` | `UTurnBasedActionBase` (Abstract) | The per-turn action: cooldowns (`MaxCompletionsPerTurn`, `CooldownTurns`), Enhanced Input-driven grid-tile selection pipeline (hover validation → selection validation → `HandleValidSelection`), generic `InputBindings` array (`FTurnBasedActionInputBinding`) so new bindings need zero new C++. | Key API: `Complete`, `Cancel`, `CanActivate`, `RequestNextAction`, `IsValidHoverTile`/`IsValidSelectionTile`/`HandleValidHover`/`HandleHoverCleared`/`HandleValidSelection` (NativeEvent), `BindInput`/`UnbindInput`, `CreateInputMappingContext`. Base class for the current consuming project's own piece-placement/shift action subclasses. |
| `UTurnBasedSpectatorAction` | `UTurnBasedActionBase` (Abstract) | Passive/non-turn-owning controller action (idle/spectator/pause views); never completes naturally, no cooldown tracking. | |
| `UTurnBasedPauseAction` | `UTurnBasedSpectatorAction` | Concrete framework-provided pause action: spawns/removes a configurable `PauseWidgetClass`, disables/enables controller input. | |
| `UTurnBasedActionsComponent` | `UActorComponent` | The stack manager: push/pop/clear, root/idle/spectator/pause/awaiting-confirmation action wiring from a `UActionLoadoutDataAsset`, turn lifecycle notifications (`NotifyTurnStarted/Ended`, `NotifyPaused/Unpaused`, `NotifyMatchEnded`), auto-end-turn support, action history log, and a board-change-request confirmation guard. | Large public API (~25 UFUNCTIONs). Key API: `PushAction`, `SafePopAction`, `ClearAndPush`, `TryPushAction(ByRef)`, `CancelTopAction`, `RequestTurnEnd`, `GetTopAction`, `GetAllRuntimeActions`, `FindActionByTag`, `NotifyBoardChangeOutcome`, `IsAwaitingRequestConfirmation`. While `bAwaitingRequestConfirmation` is true (set by `HandleBoardChangeRequested`, cleared by `NotifyBoardChangeOutcome`) every stack-mutating entry point no-ops -- see Notable Design Patterns. Self-acknowledged code smell TODO ("don't love this cast") at `TurnBasedActionsComponent.cpp:74`. |
| `UActionLoadoutDataAsset` | `UDataAsset` | Designer-facing config asset: system action classes (root/idle/spectator/pause/awaiting-confirmation) plus instanced `Actions` array and `BannedActionTags`; vends new instances via `Get*Action(Outer)`. Has `IsDataValid` editor validation. | Key API: `GetRootAction`, `GetIdleViewerAction`, `GetSpectatorAction`, `GetPauseAction`, `GetAwaitingConfirmationAction`, `GetPermittedActions`, `GetRequiredActions`, `GetOptionalActions`, `IsActionPermitted`. |

### Framework/Controller

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `ATurnBasedAIController` | `AAIController` (Abstract) | Base AI controller; auto-creates a PlayerState (AI controllers don't by default) via `EnsurePlayerState`; owns Participant/Actions/Coordinator components. | |
| `UTurnBasedControllerCoordinatorComponent` | `UActorComponent` | Mediator between `UTurnBasedParticipantComponent` and `UTurnBasedActionsComponent`, added identically to both AI and Player controllers. Also optionally routes `ATurnBasedGameState` match-phase changes to the actions component. | |
| `ATurnBasedPlayerControllerBase` | `APlayerController`, `IGridFactionInterface` (Abstract) | Player-side counterpart to `ATurnBasedAIController`; same 3 components; optional auto `ServerNotifyReady` on BeginPlay. | `IGridFactionInterface` (`UnrealGridMechanics`) is implemented only for `GetFactionId_Implementation`, returning `ParticipantComponent->GetActiveParticipantSlotIndex()` -- `GetFactionTag`/`SetFactionId` are not overridden. `ATurnBasedAIController` does not implement this interface. |
| `ATurnBasedGameMode` | `AGameMode` | Match config (`TurnDuration`, `ForfeitThreshold`, `ReconnectTimeout`); `RegisterAIParticipant`, `StartReadyCheck`; overrides `PostLogin`/`Logout`/`HandleMatchHasStarted`. | |
| `ATurnBasedGameState` | `AGameState` | Replicates `MatchPhase` (`OnRep_MatchPhase`); owns a `UTurnBasedParticipantManagerComponent`; passthrough accessors read from the manager rather than duplicating state. | `friend class UTurnBasedParticipantManagerComponent` grants write access. |
| `ATurnBasedPlayerState` | `APlayerState` | Per-participant replicated state: `SlotIndex` (doubles as FactionID), `TurnsMissed`, `bForfeited`, `bIsReady`, `ParticipantType`. | Manager-only setters via friend. |

### Turn/Order

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `ITurnOrderInterface` | Native interface | `GetNextParticipantIndex`, `GetFirstParticipantIndex`, `GetStrategyName` — all NativeEvent for BP override. | Strategy-pattern contract. |
| `URandomTurnOrderStrategy` | `UObject` + `ITurnOrderInterface` | Random turn order. | |
| `USequentialTurnOrderStrategy` | `UObject` + `ITurnOrderInterface` | Sequential (round-robin) turn order. | Default strategy. |

### Turn/Participant

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `UTurnBasedParticipantComponent` | `UActorComponent` | Per-controller participant: server RPCs (`ServerNotifyReady`, `ServerSubmitTurnEnd`), client RPC (`ClientReceiveTurnNotification`), local delegates for own-turn/opponent-turn events. | `friend class UTurnBasedParticipantManagerComponent`. |
| `UTurnBasedParticipantManagerComponent` | `UActorComponent` | The match-level state machine: replicated `CurrentPhase`/`ActiveParticipantIndex`/`TurnNumber`/`Participants`; ready-check, turn timeout, forfeit-on-disconnect, and reconnect-timeout handling; gates turn-end advancement on a designer-set `TurnEndEventTag` via `UnrealGameMechanics`' `UGameEventTaskSubsystem` instead of a bespoke hold API. | Large private state-machine surface: `SetPhase`, `StartTurn`, `EndTurn`, `AdvanceToNextParticipant`, `HandleTurnTimeout`, `HandleReconnectTimeout`. `EndTurn` calls `QueueTagContainer(TurnEndEventTag)`; `AdvanceToNextParticipant` is bound as that tag's `BindOnTagComplete` handler in `BeginPlay` and only runs once every task registered against the tag completes (or immediately if `TurnEndEventTag` is unset, which also logs an error). Central to the project-level [Gated Event Sequencing via Tags workflow](../../../Source/ConnectIt/Docs/Workflows/GameplayTag-EventSequencing.md) -- see Known Discrepancies for the retired mechanism this replaced. |

### Debug

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `UDWidget_TurnBasedParticipantManagerComponent` | `UDWidgetBase` (`UnrealGameMechanics`) | Tracks `UTurnBasedParticipantManagerComponent` -- caches turn phase, active participant index, turn number, and the participant list; refreshes on any of its five state-change delegates. | `Public/Debug/DWidget_TurnBasedParticipantManagerComponent.h`. |
| `UDWidget_TurnBasedActionsComponent` | `UDWidgetBase` (`UnrealGameMechanics`) | Tracks `UTurnBasedActionsComponent` -- caches top/root action tag (never the live action pointer), stack depth, and awaiting-confirmation state. | `Public/Debug/DWidget_TurnBasedActionsComponent.h`. Resolved per-owning-player, not as a world/GameState singleton. |

Both live here (not in a consuming game module) because the classes they track are defined in this plugin -- see the consuming project's [DebugWidgets.md](../../../Source/ConnectIt/Docs/Workflows/DebugWidgets.md) workflow doc for the full pattern and the complete class↔widget↔plugin mapping.

### Top-level (Delegates/Enums/Library/Structs/Module)

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `FOnTurnNotification_Native`, `FOnOpponentTurnStarted_Native` | Native multicast delegates | Non-dynamic delegate declarations for C++-only binding. | Header has a commented-out block noting "DYNAMIC delegates cannot be declared like this, they do not compile properly" — a documented gotcha worth preserving. |
| `EMatchPhase`, `ETurnPhase`, `ETurnEndReason`, `EParticipantType`, `ETurnBasedActionState`, `ETurnModifierType` | Enums | Core enums driving the state machine and action states. | |
| `UTurnBasedMechanicsLibrary` | `UBlueprintFunctionLibrary` | Currently one function: `IsLocalPlayerActiveParticipant`. | |
| `FTurnParticipantInfo`, `FTurnStartContext`, `FTurnNotification`, `FTurnBasedActionRecord`, `FTurnActionRequest`, `FTurnModifier` | Structs | Core data-transfer structs for turn/participant/action state. | `FTurnActionRequest` carries `FInstancedStruct Payload` for project-specific extension; both its and `FTurnModifier`'s comments reference a consumer's own project-specific types as extension-point examples — an intentional pattern, not orphaned generality. |
| `FUnrealTurnBasedMechanicsModule` + `LogTurnBasedMechanics` | `IModuleInterface` + log category | Module entry point and dedicated log category. | Only plugin in this suite with its own `DECLARE_LOG_CATEGORY_EXTERN`. |

## Known Discrepancies / Issues in This Plugin

- `UTurnBasedActionsComponent`'s self-acknowledged cast code smell at `TurnBasedActionsComponent.cpp:74` ("don't love this cast"). See the project-level [duplication notes](../../../Source/ConnectIt/Docs/Duplication.md).
- Redundant Public+Private dependency listing of `UnrealGridMechanics`/`UnrealGameMechanics` in `UnrealTurnBasedMechanics.Build.cs`. See the project-level [duplication notes](../../../Source/ConnectIt/Docs/Duplication.md).
- Dual dynamic/native delegate declaration convention is deliberate here but inconsistently applied elsewhere in this plugin suite. See the project-level [naming conventions notes](../../../Source/ConnectIt/Docs/Conventions.md).
- The `BeginResolutionHold`/`EndResolutionHold` counter (and the `TurnResolutionDuration` timer before it) that earlier versions of this doc described no longer exist in `UTurnBasedParticipantManagerComponent` -- turn-end advancement is gated by a `TurnEndEventTag` through `UnrealGameMechanics`' `UGameEventTaskSubsystem` instead (see the Turn/Participant table above and [Systems.md](Systems.md#turn-order--the-participant-state-machine)). The project-level Resolution Hold Counter workflow doc this file previously linked has since been removed from the project's Workflows folder entirely; the [Gated Event Sequencing via Tags workflow](../../../Source/ConnectIt/Docs/Workflows/GameplayTag-EventSequencing.md) is the current, accurate reference for this mechanism.
