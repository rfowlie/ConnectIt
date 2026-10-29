---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/TurnBasedActionsComponent.h
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Private/Action/TurnBasedActionsComponent.cpp
reconciled: 2026-10-29
commit: 193c1c9
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

- **Setup:** `InitialiseFromLoadout(UActionLoadoutDataAsset*)` -- builds the actions; it does **not** seed the
  PlayerState's action state (the game's server-side setup does that, and the controller builds this component
  *from* the PlayerState's loadout).
- **Turn lifecycle (called by the coordinator):** `NotifyTurnStarted(FTurnStartContext)`,
  `NotifyOpponentTurnStarted`, `NotifyTurnEnded`, `NotifyPaused` / `NotifyUnpaused`,
  `NotifyMatchEnded`.
- **Stack ops:** `PushAction` (force-deactivates old top, no pop), `SafePopAction`
  (refuses to empty the stack), `ClearAndPush` (tear down + new sole root),
  `TryPushAction(FGameplayTag)` / `TryPushActionByRef` / `TryPushActionByClass(TSubclassOf<UTurnBasedPlayerAction>)`
  (added 2026-09-14 — resolves to the already-cloned `RuntimeActions` instance of that
  class, same as `TryPushAction` does by tag; **does not** construct a new instance; returns `TryPushActionByRef`'s
  result — true only if actually pushed, false if no such action or the push was refused by limbo /
  awaiting-confirmation / `CanActivate()`),
  `CancelTopAction`.
- **Items:** `ActivateItemAction(Class)` -- starts an item action from the inventory (e.g. its button): the player
  must hold one and the item budget have a use left; the instance is marked as an item use for this activation, so its
  request spends the item and the budget, never permanent uses. Works for an action with no permanent uses. False if
  refused (also the usual push refusals, or the action already active).
- **Turn end:** `CanEndTurn()` → `CanAutoEndTurn()` (BlueprintNativeEvent) — default asks the owner's
  `ATurnBasedPlayerState::AreTurnEndRequirementsMet(Loadout)` (the same check AI controllers use), which walks
  the loadout's `TurnEndRequirements` tree against the uses this turn; **no tree ⇒ true**. `HasTurnEndRequirementTree()`;
  overrides should defer to `Super` when a tree exists. `RequestTurnEnd()` broadcasts `OnTurnEndRequested`.
  (`GetRequiredActions()` and the `bIsRequired`-based logic are gone.)
- **Post-completion limbo:** `IsAwaitingRuntimeState()`, `StateSyncTimeoutSeconds` (default 3s). After the
  server confirms a request the action completes as normal, *then* the component enters limbo — refusing
  player-initiated pushes — until the PlayerState's `OnActionRuntimeStateUpdated` fires (revision check),
  then checks turn end. A timeout exits limbo anyway so a lost update can't freeze the turn.
  `EnsureBoundToPlayerState()` binds the handler (`AddUniqueDynamic`) from `InitialiseFromLoadout` and
  `NotifyTurnStarted`, since a client's PlayerState may replicate in after init.
- **Board change:** `NotifyBoardChangeOutcome(FTurnActionRequest, bSucceeded)` — the one
  call project glue makes once the server answers.
- **Designer overrides:** `OnTurnStarted`, `OnOpponentTurnStarted`, `CanAutoEndTurn`.
- **Debug seed:** `GetInfo()` → `FTurnBasedActionsComponentInfo`.

## Collaborators

- **Loadout:** `UActionLoadoutDataAsset` — the 5 system actions and the turn-end tree. **Turn actions are built
  from the PlayerState** (`SyncRuntimeActions`): one instance per action it has -- permanent and item -- reused every
  turn; until its state has replicated, per loadout `PermanentActions` entry. Only adds; re-run on every PlayerState
  action update, so a game-built list or a newly granted item gets its instance.
- **PlayerState:** `ATurnBasedPlayerState` holds the authoritative per-action state (seeded by the game, not by
  this component), evaluates turn end (`AreTurnEndRequirementsMet`) and signals via `OnActionRuntimeStateUpdated`. Cooldown ticking (`TickCooldowns`) moved off the
  component to the server's `StartTurn`.
- **Actions:** creates & owns [[UnrealTurnBasedMechanics/code/UTurnBasedPlayerAction|UTurnBasedPlayerAction]] runtime instances +
  `UTurnBasedSpectatorAction` system instances; binds each action's
  `OnChangeRequested` / `OnActionCompleted` / `OnActionCancelled` / `OnNextActionRequested`.
- **Coordinator:** `UTurnBasedControllerCoordinatorComponent` calls every `Notify*` and
  relays `OnBoardChangeRequested` / `OnTurnEndRequested` outward.
- **Fires:** `OnBoardChangeRequested(+_Native)`, `OnTurnEndRequested(+_Native)`,
  `OnTurnEndReady`, `OnActionPushed/Popped/Completed/Cancelled` (+ `…Safe` snapshot
  siblings for observers).
- Flows: [[UnrealTurnBasedMechanics/code/systems/action-stack-lifecycle|action-stack-lifecycle]],
  [[UnrealTurnBasedMechanics/code/systems/turn-end-tag-gate|turn-end-tag-gate]].

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
- **Limbo is after completion, not instead of it.** The action finishes first (so a player can't fire
  repeated requests), then the component waits on the PlayerState; don't check turn end before the update lands.
- `NotifyBoardChangeOutcome` no-ops unless `Request` matches the pending one
  (`FTurnActionRequest::operator==`).

## Cross-impact

Change the slot set or lifecycle and also update:
`UActionLoadoutDataAsset` (slot properties + `Get*Action` vending + `IsDataValid`),
`UTurnBasedControllerCoordinatorComponent` (which `Notify*` it calls),
`UDWidget_TurnBasedActionsComponent` (reads `GetInfo()` / the `…Safe` delegates),
`FTurnBasedActionsComponentInfo`.

## Changes

- 2026-10-24 — `ActivateItemAction`; runtime actions built from the PlayerState's actions (`SyncRuntimeActions`,
  permanent + inventory) instead of the loadout's lists; `UTurnBasedAction` → `UTurnBasedPlayerAction`. Player-actions
  plan, Stages 1-3.
- 2026-10-22 — **no longer seeds the PlayerState** in `InitialiseFromLoadout` (the game's server setup does --
  seeding here would loop, since the controller builds this component from the PlayerState's loadout);
  `CanAutoEndTurn` delegates to `ATurnBasedPlayerState::AreTurnEndRequirementsMet`, shared with AI controllers.
- 2026-09-21 — **turn-end and state moved to the loadout/PlayerState model**: tree-based `CanAutoEndTurn`,
  post-completion limbo + `EnsureBoundToPlayerState`, `TryPushActionByClass` returns the real result,
  `TickCooldowns`/`GetRequiredActions` removed. See
  [legacy removal](../../ConnectIt/_decisions/2026-09-20-legacy-action-system-removed-stage-3.md).
- 2026-09-18 — internal-only: `Action->ActionTag` field reads in log strings became
  `Action->GetActionTag()` calls — no change to this page's public surface or behavior.
  (`process-code` sweep — commit `9187568`.)
- 2026-09-14 — added `TryPushActionByClass` (resolve-by-class, same shape as
  `TryPushAction`'s resolve-by-tag) — for UI code that only has a
  `TSubclassOf<UTurnBasedPlayerAction>` handy, not the action's `ActionTag`.
- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[UnrealTurnBasedMechanics/CLAUDE|UnrealTurnBasedMechanics overview]] → *Action*.
- [[UnrealTurnBasedMechanics/code/recipes/add-a-turn-action|recipes/add-a-turn-action]]
