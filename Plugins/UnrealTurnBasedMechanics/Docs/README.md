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
- Needing match-level state machine behavior: ready checks, turn timeouts, forfeit-on-disconnect, reconnect handling, turn-resolution holds (`UTurnBasedParticipantManagerComponent`).
- Needing a pluggable turn order (sequential or random, or a custom `ITurnOrderInterface` implementation).
- Extending turn actions or modifiers with project-specific payloads via `FInstancedStruct Payload` on `FTurnActionRequest`/`FTurnModifier`.

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
- **Resolution-hold counter mechanism**: `UTurnBasedParticipantManagerComponent::BeginResolutionHold`/`EndResolutionHold` replaced an older fixed-duration timer (`TurnResolutionDuration`) so resolution-phase visuals can gate turn advancement precisely rather than guessing a duration. See the project-level [Resolution Hold Counter workflow](../../../Source/ConnectIt/Docs/Workflows/TurnResolutionHold.md).
- This plugin is the best-documented in the project (dense inline rationale comments throughout) — worth treating as the reference model for documentation style when writing or reviewing other plugins' code comments.

## Classes

### Action

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `UTurnBasedActionBase` | `UObject` (Abstract) | Root of the action hierarchy: `Activate`/`ForceDeactivate` lifecycle, with `Activate_Internal`/`Deactivate_Internal`/`ForceDeactivate_Internal`/`PostInitialiseAction` BlueprintNativeEvent hooks and both dynamic and "_Native" delegate variants. | Key API: `Activate`, `ForceDeactivate`, `IsActive`, `GetPlayerController`. |
| `UTurnBasedAction` | `UTurnBasedActionBase` (Abstract) | The per-turn action: cooldowns (`MaxCompletionsPerTurn`, `CooldownTurns`), Enhanced Input-driven grid-tile selection pipeline (hover validation → selection validation → `HandleValidSelection`), generic `InputBindings` array (`FTurnBasedActionInputBinding`) so new bindings need zero new C++. | Key API: `Complete`, `Cancel`, `CanActivate`, `RequestNextAction`, `IsValidHoverTile`/`IsValidSelectionTile`/`HandleValidHover`/`HandleHoverCleared`/`HandleValidSelection` (NativeEvent), `BindInput`/`UnbindInput`, `CreateInputMappingContext`. Base class for the current consuming project's own piece-placement/shift action subclasses. |
| `UTurnBasedSpectatorAction` | `UTurnBasedActionBase` (Abstract) | Passive/non-turn-owning controller action (idle/spectator/pause views); never completes naturally, no cooldown tracking. | |
| `UTurnBasedPauseAction` | `UTurnBasedSpectatorAction` | Concrete framework-provided pause action: spawns/removes a configurable `PauseWidgetClass`, disables/enables controller input. | |
| `UTurnBasedActionsComponent` | `UActorComponent` | The stack manager: push/pop/clear, root/idle/spectator/pause action wiring from a `UActionLoadoutDataAsset`, turn lifecycle notifications (`NotifyTurnStarted/Ended`, `NotifyPaused/Unpaused`, `NotifyMatchEnded`), auto-end-turn support, action history log. | Large public API (~25 UFUNCTIONs). Key API: `PushAction`, `SafePopAction`, `ClearAndPush`, `TryPushAction(ByRef)`, `CancelTopAction`, `RequestTurnEnd`, `GetTopAction`, `GetAllRuntimeActions`, `FindActionByTag`. Self-acknowledged code smell TODO ("don't love this cast") at `TurnBasedActionsComponent.cpp:71`. |
| `UActionLoadoutDataAsset` | `UDataAsset` | Designer-facing config asset: system action classes (root/idle/spectator/pause) plus instanced `Actions` array and `BannedActionTags`; vends new instances via `Get*Action(Outer)`. Has `IsDataValid` editor validation. | Key API: `GetRootAction`, `GetIdleViewerAction`, `GetSpectatorAction`, `GetPauseAction`, `GetPermittedActions`, `GetRequiredActions`, `GetOptionalActions`, `IsActionPermitted`. |

### Framework/Controller

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `ATurnBasedAIController` | `AAIController` (Abstract) | Base AI controller; auto-creates a PlayerState (AI controllers don't by default) via `EnsurePlayerState`; owns Participant/Actions/Coordinator components. | |
| `UTurnBasedControllerCoordinatorComponent` | `UActorComponent` | Mediator between `UTurnBasedParticipantComponent` and `UTurnBasedActionsComponent`, added identically to both AI and Player controllers. Also optionally routes `ATurnBasedGameState` match-phase changes to the actions component. | |
| `ATurnBasedPlayerControllerBase` | `APlayerController` (Abstract) | Player-side counterpart to `ATurnBasedAIController`; same 3 components; optional auto `ServerNotifyReady` on BeginPlay. | |
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
| `UTurnBasedParticipantManagerComponent` | `UActorComponent` | The match-level state machine: replicated `CurrentPhase`/`ActiveParticipantIndex`/`TurnNumber`/`Participants`; ready-check, turn timeout, forfeit-on-disconnect, and reconnect-timeout handling; a `BeginResolutionHold`/`EndResolutionHold` counter that replaced an old fixed-duration `TurnResolutionDuration` timer so resolution-phase visuals can gate turn advancement precisely. | Large private state-machine surface: `SetPhase`, `StartTurn`, `EndTurn`, `AdvanceToNextParticipant`, `HandleTurnTimeout`, `HandleReconnectTimeout`. Central to the project-level [Resolution Hold Counter workflow](../../../Source/ConnectIt/Docs/Workflows/TurnResolutionHold.md). |

### Top-level (Delegates/Enums/Library/Structs/Module)

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `FOnTurnNotification_Native`, `FOnOpponentTurnStarted_Native` | Native multicast delegates | Non-dynamic delegate declarations for C++-only binding. | Header has a commented-out block noting "DYNAMIC delegates cannot be declared like this, they do not compile properly" — a documented gotcha worth preserving. |
| `EMatchPhase`, `ETurnPhase`, `ETurnEndReason`, `EParticipantType`, `ETurnBasedActionState`, `ETurnModifierType` | Enums | Core enums driving the state machine and action states. | |
| `UTurnBasedMechanicsLibrary` | `UBlueprintFunctionLibrary` | Currently one function: `IsLocalPlayerActiveParticipant`. | |
| `FTurnParticipantInfo`, `FTurnStartContext`, `FTurnNotification`, `FTurnBasedActionRecord`, `FTurnActionRequest`, `FTurnModifier` | Structs | Core data-transfer structs for turn/participant/action state. | `FTurnActionRequest` carries `FInstancedStruct Payload` for project-specific extension; both its and `FTurnModifier`'s comments reference a consumer's own project-specific types as extension-point examples — an intentional pattern, not orphaned generality. |
| `FUnrealTurnBasedMechanicsModule` + `LogTurnBasedMechanics` | `IModuleInterface` + log category | Module entry point and dedicated log category. | Only plugin in this suite with its own `DECLARE_LOG_CATEGORY_EXTERN`. |

## Known Discrepancies / Issues in This Plugin

- `UTurnBasedActionsComponent`'s self-acknowledged cast code smell at `TurnBasedActionsComponent.cpp:71` ("don't love this cast"). See the project-level [duplication notes](../../../Source/ConnectIt/Docs/Duplication.md).
- Redundant Public+Private dependency listing of `UnrealGridMechanics`/`UnrealGameMechanics` in `UnrealTurnBasedMechanics.Build.cs`. See the project-level [duplication notes](../../../Source/ConnectIt/Docs/Duplication.md).
- Dual dynamic/native delegate declaration convention is deliberate here but inconsistently applied elsewhere in this plugin suite. See the project-level [naming conventions notes](../../../Source/ConnectIt/Docs/Conventions.md).
