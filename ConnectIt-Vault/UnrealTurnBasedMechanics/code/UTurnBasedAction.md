---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/TurnBasedAction.h
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Private/Action/TurnBasedAction.cpp
reconciled: 2026-09-06
commit: acbfa6f
---

# UTurnBasedAction

`Abstract`, extends [[UnrealTurnBasedMechanics/code/UTurnBasedActionBase|UTurnBasedActionBase]].
The per-turn, player-owned action: cooldown / completion config, an Enhanced-Input
grid-tile hover→validate→select pipeline, data-driven named input bindings, board-change
requests, and action chaining. This is the class you subclass most often (usually in
Blueprint).

## When you touch this

- Adding a new player move / ability as a `UTurnBasedAction` subclass — see
  [[UnrealTurnBasedMechanics/recipes/add-a-turn-action|recipes/add-a-turn-action]].
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
  `HandleValidSelection`, `ClearSelectionState`; `OnBoundInputTriggered(FGameplayTag)`;
  `ShouldTickCooldown(bool)`.
- **Protected helpers:** `RequestBoardChange(FTurnActionRequest)` (call from
  `HandleValidSelection`), `BindInput()` / `UnbindInput()`, `BindGridSubsystem()` /
  `UnbindGridSubsystem()`, `CurrentHoveredTile`.

## Collaborators

- **Enhanced Input:** owns a `UInputTagBinder` (from `UnrealGameMechanics/Input`) that
  hosts the `InputBindings` mapping context and dispatches to
  `OnBoundInputTriggered`. `bRequiresSelection` auto-calls `BindInput()` from
  `Activate_Internal`.
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
- Presentation fields are UI-only — the action never reads them; `ActionTag` is an
  identifier, not a display string.
- Board change goes `RequestBoardChange` → `OnChangeRequested` → the component's
  `HandleBoardChangeRequested` (freezes the stack) — see
  [[UnrealTurnBasedMechanics/systems/action-stack-lifecycle|action-stack-lifecycle]].
- Commented-out `SelectionInputAction` / `SelectionInputKey` fields + `TODO`s: the
  selection-input story is still settling; don't assume a default selection `UInputAction`.

## Cross-impact

A new subclass must be registered in
[[UnrealTurnBasedMechanics/code/UActionLoadoutDataAsset|UActionLoadoutDataAsset]]
(`Actions` array or a system slot). Adding `InputBindings` needs matching
`FInputTagBinding` entries; new gameplay-tag request types need a project `USTRUCT` for
`FTurnActionRequest::Payload`.

## See also

- In-repo: `Docs/README.md` → *Action*; `Docs/Systems.md` → *The Action Stack*.
