---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/TurnBasedActionsComponent.h
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Private/Action/TurnBasedActionsComponent.cpp
reconciled: 2026-09-06
commit: ae9c1f0
---

# UTurnBasedActionsComponent

`UActorComponent` on a controller. Owns the **action stack** plus five designer-assigned
system-action slots vended from a [[UnrealTurnBasedMechanics/code/UActionLoadoutDataAsset|loadout]],
and translates turn-lifecycle notifications into stack operations. Largest API surface in
the plugin (~25 UPROPERTY / ~32 UFUNCTION).

## When you touch this

- Wiring a controller for turn-based play (via
  [[UnrealTurnBasedMechanics/code/UTurnBasedParticipantComponent|UTurnBasedParticipantComponent]]
  and the coordinator).
- Changing what happens on turn start/end, pause, match end.
- Adding or reshaping the board-change confirmation handshake.
- Adding a system-action slot (see the recipe cross-impact).

## Entry points

- **Setup:** `InitialiseFromLoadout(UActionLoadoutDataAsset*)`.
- **Turn lifecycle (called by the coordinator):** `NotifyTurnStarted(FTurnStartContext)`,
  `NotifyOpponentTurnStarted`, `NotifyTurnEnded`, `NotifyPaused` / `NotifyUnpaused`,
  `NotifyMatchEnded`.
- **Stack ops:** `PushAction` (force-deactivates old top, no pop), `SafePopAction`
  (refuses to empty the stack), `ClearAndPush` (tear down + new sole root),
  `TryPushAction(FGameplayTag)` / `TryPushActionByRef`, `CancelTopAction`.
- **Turn end:** `CanEndTurn()` → `CanAutoEndTurn()` (BlueprintNativeEvent designer
  override); `RequestTurnEnd()` broadcasts `OnTurnEndRequested`.
- **Board change:** `NotifyBoardChangeOutcome(FTurnActionRequest, bSucceeded)` — the one
  call project glue makes once the server answers.
- **Designer overrides:** `OnTurnStarted`, `OnOpponentTurnStarted`, `CanAutoEndTurn`.
- **Debug seed:** `GetInfo()` → `FTurnBasedActionsComponentInfo`.

## Collaborators

- **Loadout:** `UActionLoadoutDataAsset` — clones `Actions`, vends the 5 system actions.
- **Actions:** creates & owns `UTurnBasedAction` runtime instances +
  `UTurnBasedSpectatorAction` system instances; binds each action's
  `OnChangeRequested` / `OnActionCompleted` / `OnActionCancelled` / `OnNextActionRequested`.
- **Coordinator:** `UTurnBasedControllerCoordinatorComponent` calls every `Notify*` and
  relays `OnBoardChangeRequested` / `OnTurnEndRequested` outward.
- **Fires:** `OnBoardChangeRequested(+_Native)`, `OnTurnEndRequested(+_Native)`,
  `OnTurnEndReady`, `OnActionPushed/Popped/Completed/Cancelled` (+ `…Safe` snapshot
  siblings for observers).
- Flows: [[UnrealTurnBasedMechanics/systems/action-stack-lifecycle|action-stack-lifecycle]],
  [[UnrealTurnBasedMechanics/systems/turn-end-tag-gate|turn-end-tag-gate]].

## Gotchas

- **The stack never empties.** `SafePopAction` blocks the last pop; `RootActionClass` is
  mandatory (author a do-nothing action if the game has no idle state).
- **`bAwaitingRequestConfirmation` freezes the stack.** While a board-change request is in
  flight, `PushAction` / `SafePopAction` / `TryPushActionByRef` / `ClearAndPush` all
  refuse to run. It is cleared inside `NotifyBoardChangeOutcome` *before* that method
  mutates the stack. A lost/never-arriving outcome wedges the component.
- **`AwaitingConfirmationActionClass` ≠ `IdleViewerActionClass`.** Awaiting = mid-turn,
  own request pending; Idle = turn already ended.
- **Observers bind the `…Safe` delegates**, which carry a copied `FTurnActionSnapshot`.
  The raw-pointer `OnAction*` delegates hand out the live action with callable
  `Complete()` / `Cancel()`.
- `NotifyBoardChangeOutcome` no-ops unless `Request` matches the pending one
  (`FTurnActionRequest::operator==`).

## Cross-impact

Change the slot set or lifecycle and also update:
`UActionLoadoutDataAsset` (slot properties + `Get*Action` vending + `IsDataValid`),
`UTurnBasedControllerCoordinatorComponent` (which `Notify*` it calls),
`UDWidget_TurnBasedActionsComponent` (reads `GetInfo()` / the `…Safe` delegates),
`FTurnBasedActionsComponentInfo`.

## See also

- In-repo: `old/Plugins/UnrealTurnBasedMechanics/Docs/README.md` → *Action*; `old/Plugins/UnrealTurnBasedMechanics/Docs/Systems.md` → *The Action Stack*.
- [[UnrealTurnBasedMechanics/recipes/add-a-turn-action|recipes/add-a-turn-action]]
