---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/TurnBasedAction.h
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Private/Action/TurnBasedAction.cpp
reconciled: 2026-09-18
commit: 9187568
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

- **Config (`EditAnywhere`):** `bIsRequired`, `bIsCancellable`, `bRequiresSelection`,
  `bAllowsOptionalInterrupt`, `MaxCompletionsPerTurn` (0 = unlimited), `CooldownTurns`,
  `InputBindings` (`TArray<FInputTagBinding>`), presentation `DisplayName` / `Description`
  / `Icon`.
- **Call:** `Complete()` (increments `CompletionsThisTurn`), `Cancel()` (no increment),
  `RequestNextAction(TSubclassOf<UTurnBasedAction>)`, `CanActivate()`, `IsComplete()`.
- **Override (BlueprintNativeEvent):** `OnCompleted` / `OnCancelled`; selection hooks
  `IsValidHoverTile`, `IsValidSelectionTile`, `HandleValidHover`, `HandleHoverCleared`,
  `HandleValidSelection`, `ClearSelectionState`; `ConstructInputBindings()` (populates
  `InputBindings`, called once from `InitialiseAction` before the input binder is built —
  see Collaborators); `ShouldTickCooldown(bool)`.
- **Protected helpers:** `RequestBoardChange(FTurnActionRequest)` (call from
  `HandleValidSelection`), `BindInput()` / `UnbindInput()`, `BindGridSubsystem()` /
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

- `CompletionsThisTurn` increments **only** on `Complete()` — never on `Activate` or
  `Cancel`. `ResetTurnState()` clears it at turn start.
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
(`Actions` array or a system slot). Adding `InputBindings` needs matching
`FInputTagBinding` entries; new gameplay-tag request types need a project `USTRUCT` for
`FTurnActionRequest::Payload`.

## Changes

- 2026-09-18 — **Input binding rewritten**: the old shared `OnInputTagTriggered`/
  `OnBoundInputTriggered` broadcast is gone — `ConstructInputBindings()` now populates
  `InputBindings` up front (called from `InitialiseAction`, before `InputTagBinder` is
  built), and each `FInputTagBinding` dispatches straight to its own
  `InputActionDelegate`. (`process-code` sweep — commit `9187568`.)
- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[UnrealTurnBasedMechanics/CLAUDE|UnrealTurnBasedMechanics overview]] → *Action*.
