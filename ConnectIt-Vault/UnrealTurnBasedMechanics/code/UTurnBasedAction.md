---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/TurnBasedAction.h
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Private/Action/TurnBasedAction.cpp
reconciled: 2026-09-21
commit: aa8373e
---

# UTurnBasedAction

`Abstract`, extends [[UnrealTurnBasedMechanics/code/UTurnBasedActionBase|UTurnBasedActionBase]].
The per-turn, player-owned action: cooldown / completion config, an Enhanced-Input
grid-tile hover→validate→select pipeline, data-driven named input bindings, board-change
requests, and action chaining. This is the class you subclass most often (usually in
Blueprint).

## When you touch this

- Adding a new player move / ability as a `UTurnBasedAction` subclass — see
  [[UnrealTurnBasedMechanics/code/recipes/add-a-turn-action|recipes/add-a-turn-action]].
- Changing the selection pipeline or how board changes are requested.

## Entry points

- **Config (`EditAnywhere`):** `bIsCancellable`, `bRequiresSelection`, `InputBindings` (`TArray<FInputTagBinding>`), presentation `DisplayName` / `Description`
  / `Icon`.
- **No budget state on the action.** Uses, per-turn caps, cooldowns and turn-end contribution live in the
  loadout (`PermanentActions` / `NumberedActions` / `TurnEndRequirements`) and, live, on the owner's
  `ATurnBasedPlayerState`; the action only *reads* it.
- **Call:** `Complete()`, `Cancel()`, `RequestNextAction(TSubclassOf<UTurnBasedAction>)`, `CanActivate()`
  (client-side convenience — asks the PlayerState whether uses/cap/cooldown allow it; the server re-checks).
- **Pull accessors (read the PlayerState on every call, return 0/false if none):**
  `GetPermanentRuntimeState(out)`, `GetNumberedRuntimeState(out)`, `GetUsesThisTurn()`,
  `GetMaxUsesPerTurn()` (effective cap, 0 = unlimited/unknown), `GetUsesRemaining()` (numbered only),
  `GetOwnerPlayerState()`. Static `GetTagForClass(TSubclassOf)` reads the tag off the class default object.
- **Override (BlueprintNativeEvent):** `OnCompleted` / `OnCancelled`; `ProducesRequestType(FGameplayTag)` (default true; the Mediator gate checks a request's type against the
  action named by its `ActionTag`, so one action can't spend its budget sending another's request);
  selection hooks
  `IsValidHoverTile`, `IsValidSelectionTile`, `HandleValidHover`, `HandleHoverCleared`,
  `HandleValidSelection`, `ClearSelectionState`; `ConstructInputBindings()` (populates
  `InputBindings`, called once from `InitialiseAction` before the input binder is built —
  see Collaborators).
- **Protected helpers:** `RequestBoardChange(FTurnActionRequest)` (call from
  `HandleValidSelection`; **stamps `Request.ActionTag`** with this action's tag), `BindInput()` / `UnbindInput()`, `BindGridSubsystem()` /
  `UnbindGridSubsystem()`, `CurrentHoveredTile`.

## Collaborators

- **Enhanced Input:** owns a `UInputTagBinder` (from `UnrealGameMechanics/Input`) that
  hosts the `InputBindings` mapping context. No shared tag-broadcast delegate to switch
  on any more — each `FInputTagBinding` carries its own `InputActionDelegate`, dispatched
  directly per-binding by `InputTagBinder`. `bRequiresSelection` auto-calls `BindInput()`
  from `Activate_Internal`.
- **Grid:** `UGridHoverSubsystem` drives `OnGridTileHoverChanged` → the hover hooks;
  `AGridTileBase` is the hover/selection unit (from `UnrealGridMechanics`).
- **Fires:** `OnChangeRequested(+_Native)` (`const FTurnActionRequest&`),
  `OnActionCompleted` / `OnActionCancelled` (typed, + `_Native`),
  `OnNextActionRequested(+_Native)` → component's `TryPushAction`.

## Gotchas

- Uses are counted **by the server**, when the request commits (`ConsumeActionUse` in the Mediator gate) —
  not by `Complete()`. Per-turn counters and cooldowns are ticked/reset server-side in
  `UTurnBasedParticipantManagerComponent::StartTurn`. The old `CompletionsThisTurn`, `TurnsUntilAvailable`,
  `ResetTurnState`, `TickCooldown`, `IsComplete`, `bIsRequired`, `bAllowsOptionalInterrupt`,
  `MaxCompletionsPerTurn`, `CooldownTurns` and `ShouldTickCooldown` are removed.
- An action that defers selection to a later internal state should leave
  `bRequiresSelection = false` and call `BindInput()` itself when ready.
- Presentation fields are UI-only — the action never reads them; `GetActionTag()` (a
  `BlueprintNativeEvent` on `UTurnBasedActionBase`, implemented per-action rather than a
  settable data field) is an identifier, not a display string.
- Board change goes `RequestBoardChange` → `OnChangeRequested` → the component's
  `HandleBoardChangeRequested` (freezes the stack) — see
  [[UnrealTurnBasedMechanics/code/systems/action-stack-lifecycle|action-stack-lifecycle]].
- Commented-out `SelectionInputAction` / `SelectionInputKey` fields + `TODO`s: the
  selection-input story is still settling; don't assume a default selection `UInputAction`.

## Cross-impact

A new subclass must be registered in
[[UnrealTurnBasedMechanics/code/UActionLoadoutDataAsset|UActionLoadoutDataAsset]]
(a `PermanentActions` / `NumberedActions` entry keyed by class, or a system slot), and, if it should gate
turn end, referenced from a `TurnEndRequirements` leaf. Adding `InputBindings` needs matching
`FInputTagBinding` entries; new gameplay-tag request types need a project `USTRUCT` for
`FTurnActionRequest::Payload`.

## Changes

- 2026-09-21 — **budget/cap/cooldown state removed from the action** (moved to loadout config + PlayerState);
  added `GetTagForClass`, `ProducesRequestType`, the pull accessors, `GetOwnerPlayerState`; `RequestBoardChange`
  stamps `ActionTag`. See [legacy removal](../../ConnectIt/_decisions/2026-09-20-legacy-action-system-removed-stage-3.md).
- 2026-09-18 — **Input binding rewritten**: the old shared `OnInputTagTriggered`/
  `OnBoundInputTriggered` broadcast is gone — `ConstructInputBindings()` now populates
  `InputBindings` up front (called from `InitialiseAction`, before `InputTagBinder` is
  built), and each `FInputTagBinding` dispatches straight to its own
  `InputActionDelegate`. (`process-code` sweep — commit `9187568`.)
- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[UnrealTurnBasedMechanics/CLAUDE|UnrealTurnBasedMechanics overview]] → *Action*.
