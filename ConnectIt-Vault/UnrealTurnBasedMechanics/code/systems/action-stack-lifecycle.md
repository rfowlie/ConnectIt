---
schema: systems
spans:
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/TurnBasedActionsComponent.h
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Private/Action/TurnBasedActionsComponent.cpp
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/TurnBasedPlayerAction.h
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/TurnBasedActionBase.h
reconciled: 2026-10-29
commit: cb9f405
---

# Action stack lifecycle

## What happens

Each controller's moment-to-moment behaviour is the top entry of a stack owned by
[[UnrealTurnBasedMechanics/code/UTurnBasedActionsComponent|UTurnBasedActionsComponent]].
The stack is never empty (a mandatory root action sits at the bottom). Turn-lifecycle
`Notify*` calls reshape it with three primitives: **`PushAction`** (force-deactivate the
current top, don't pop, activate the new top), **`SafePopAction`** (pop unless it's the
last entry, reactivate whatever's below), **`ClearAndPush`** (tear the whole stack down,
install one new root). When the active player requests a board change, the component
pushes an *awaiting-confirmation* action and **freezes all stack mutation** until
`NotifyBoardChangeOutcome` arrives; after a success the component waits in **limbo** for the
PlayerState's updated action state before it checks turn end.

## Diagram

```mermaid
stateDiagram-v2
    [*] --> Idle
    Idle --> MyTurn: NotifyTurnStarted → ClearAndPush(RootAction)
    MyTurn --> MyTurn: PushAction(sub-action) / SafePopAction()
    MyTurn --> AwaitingConfirmation: action calls RequestBoardChange\n→ HandleBoardChangeRequested\n→ push AwaitingConfirmationAction (stack frozen)
    AwaitingConfirmation --> MyTurn: NotifyBoardChangeOutcome(req, false)\n→ reactivate requesting action
    AwaitingConfirmation --> Limbo: NotifyBoardChangeOutcome(req, true)\n→ force-complete requesting action
    Limbo --> MyTurn: PlayerState OnActionRuntimeStateUpdated (or timeout)\n→ CheckAutoEndTurn
    MyTurn --> Resolving: NotifyTurnEnded → ClearAndPush(IdleViewerAction)
    Resolving --> OpponentTurn: NotifyOpponentTurnStarted → ClearAndPush(SpectatorViewerAction)
    OpponentTurn --> MyTurn: NotifyTurnStarted → ClearAndPush(RootAction)
    MyTurn --> Paused: NotifyPaused → PushAction(PauseViewerAction)
    Paused --> MyTurn: NotifyUnpaused → SafePopAction()
    Resolving --> MatchEnd: NotifyMatchEnded → ClearAndPush(IdleViewerAction)
    MatchEnd --> [*]
```

## Steps

1. **`InitialiseFromLoadout`** builds the five system-action instances from
   [[UnrealTurnBasedMechanics/code/UActionLoadoutDataAsset|UActionLoadoutDataAsset]] and one runtime action per
   action the PlayerState has, permanent and item (`SyncRuntimeActions`, re-run on every PlayerState update). It
   doesn't seed the PlayerState's action state (the game's server setup does).
2. **`NotifyTurnStarted(FTurnStartContext)`** → binds the PlayerState listener, ends any limbo, then
   `OnTurnStarted` (designer override, default `ClearAndPush(RootAction)`). Per-turn counters and
   cooldowns are the server's, reset / ticked on the PlayerState in `StartTurn`
   ([[UnrealTurnBasedMechanics/code/systems/turn-end-tag-gate|turn-end-tag-gate]]).
3. During the turn, an action chains via `RequestNextAction` → component `TryPushAction`;
   the player backs out via `CancelTopAction` / `SafePopAction`. An **item** is started with `ActivateItemAction`
   (held + budget left), which marks that activation as an item use.
4. **Board change:** the action calls `RequestBoardChange(Payload)` (it builds the
   `FTurnActionRequest`: its tag, the payload, `bIsItemUse`) → `OnChangeRequested` → `UTurnBasedActionsComponent::HandleBoardChangeRequested` sets
   `bAwaitingRequestConfirmation`, stores `PendingRequest`, pushes
   `AwaitingConfirmationAction`. `PushAction` / `SafePopAction` / `TryPushActionByRef` /
   `ClearAndPush` now refuse to run. The component re-broadcasts `OnBoardChangeRequested`
   for project glue to route to the server.
5. **`NotifyBoardChangeOutcome(Request, bSucceeded)`** — no-op unless `Request ==
   PendingRequest`. Clears the freeze first, then: on success force-completes the
   requesting action and (with a loadout that has action config) enters **state-sync limbo**,
   refusing player-initiated pushes until the PlayerState's `OnActionRuntimeStateUpdated` brings a
   newer revision (or `StateSyncTimeoutSeconds` passes); on failure reactivates it for a retry.
6. **`NotifyTurnEnded`** → `ClearAndPush(IdleViewerAction)`; **`NotifyOpponentTurnStarted`**
   → `ClearAndPush(SpectatorViewerAction)`; **`NotifyPaused` / `NotifyUnpaused`** push/pop
   `PauseViewerAction` over the preserved stack; **`NotifyMatchEnded`** →
   `ClearAndPush(IdleViewerAction)`.
7. Auto-end: when limbo ends (or straight after a completion with no action config),
   `CheckAutoEndTurn` → `CanAutoEndTurn()` (default: the PlayerState's
   `AreTurnEndRequirementsMet` against the loadout's `TurnEndRequirements` tree; no tree = true) →
   `OnTurnEndReady` / `RequestTurnEnd` when `bAutoEndTurnOnAllRequiredActionsCompleted`.

## Gotchas

- **Stack never empties** — `SafePopAction` blocks the last pop; `RootActionClass` is
  mandatory.
- **The confirmation freeze is total** — a missing `NotifyBoardChangeOutcome` wedges the
  component with no timeout.
- `PushAction` force-deactivates the old top *without popping* — it's still on the stack
  and reactivates on pop.
- Observers (debug widgets) must bind `OnActionPushedSafe` / `…PoppedSafe` / `…CompletedSafe`
  / `…CancelledSafe` (copied `FTurnActionSnapshot`), not the raw-pointer originals.
- Uses are counted by the **server** when a request commits, not by `Complete()`; the client
  learns them by replication, which is why limbo waits before checking turn end. A turn
  boundary always ends limbo without checking.

## Cross-impact

Changing the primitives or `Notify*` set touches:
[[UnrealTurnBasedMechanics/code/UActionLoadoutDataAsset|UActionLoadoutDataAsset]] (slots),
`UTurnBasedControllerCoordinatorComponent` (caller),
`UDWidget_TurnBasedActionsComponent` + `FTurnBasedActionsComponentInfo` (observers),
[[UnrealTurnBasedMechanics/code/UTurnBasedActionBase|UTurnBasedActionBase]] /
[[UnrealTurnBasedMechanics/code/UTurnBasedPlayerAction|UTurnBasedPlayerAction]] lifecycle hooks.

## See also

- [[UnrealTurnBasedMechanics/code/systems/turn-end-tag-gate|turn-end-tag-gate]] (what "Resolving" waits on)
- [[UnrealTurnBasedMechanics/code/recipes/add-a-turn-action|recipes/add-a-turn-action]]
