---
schema: code
kind: UCLASS (APlayerState)
role: primary
source:
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Framework/PlayerState/TurnBasedPlayerState.h
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Private/Framework/PlayerState/TurnBasedPlayerState.cpp
reconciled: 2026-10-29
commit: 636a8ad
---

# ATurnBasedPlayerState

## What it is

The participant's replicated state. Its slot (which doubles as the faction), turns missed, forfeit and ready flags
are written only by the participant manager. It is also **the one source of truth for action uses**: the permanent
actions' runtime state, the item inventory, and the match's item budget. Mutated only on the server; clients read
the replicated copy.

## When you touch this

- Seeding a player's actions or items, granting items, or reading them back after a match.
- Changing how a use is checked or spent, or how turn end is evaluated.

## Entry points

- **Seeding (server):**
  - `InitialiseActionState(Loadout)` seeds from the loadout's `PermanentActions`;
  - `InitialiseActionState(Loadout, PermanentActions)` seeds from a list the game builds (e.g. from a save);
  - `InitialiseInventory(Entries)`;
  - `SetItemBudget(Budget)` (default: none).
- **Permanent actions:**
  - checks: `CanUseAction(Class)` (has it, cooldown clear, under the turn cap, match uses left);
  - spending: `ConsumeActionUse(Class)` (server);
  - reads: `GetActionUsesThisTurn`, `GetActionMaxUsesPerTurn`, `GetMatchUsesRemaining`, `GetPermanentRuntimeState`,
    `FindActionClassByTag`, `HasActionConfig`.
- **Items:**
  - checks: `CanUseItem(Class)` (one held and budget left this turn), `HasItemBudgetLeft`;
  - spending: `ConsumeItemUse(Class)` (server: the item, plus the budget's turn and match counts);
  - adding: `GrantItems(Class, Amount)` (server: clamped to the cap, returns how many were added);
  - reads: `GetInventory`, `GetItemCount`, `FindItemActionClassByTag`, `GetItemBudget`, `GetItemUsesThisTurn`,
    `GetItemUsesThisMatch`.
- **Turn end:** `AreTurnEndRequirementsMet(Loadout)` and `CanEndTurn()`. These evaluate the loadout's tree against
  this turn's permanent uses and item uses (`FTurnEndUses`). It's the one place: the human actions component and AI
  controllers both call it.
- **Turn boundary (server, from the manager's `StartTurn`):** `TickActionCooldowns`, `ResetActionTurnCounters`
  (item uses this turn too).
- **Delegates:** `OnActionRuntimeStateUpdated`, `OnLoadoutChanged`, `OnInventoryChanged`, `OnTurnsMissedChanged`,
  `OnForfeited`, `OnReadyChanged`.
- `GetActionStateRevision()`: bumped on every action or item change. The actions component's limbo waits on it.

## Collaborators

- [[UnrealTurnBasedMechanics/code/ActionConfig|ActionConfig]] (the data);
  [[UnrealTurnBasedMechanics/code/UItemActionInventoryDataAsset|UItemActionInventoryDataAsset]] (caps).
- [[UnrealTurnBasedMechanics/code/UTurnBasedActionsComponent|UTurnBasedActionsComponent]] builds one action instance
  per action this state has, permanent or item.
- The project's server gate spends uses. In ConnectIt that's the Mediator: an item request (`bIsItemUse`) goes
  through `FindItemActionClassByTag` / `CanUseItem` / `ConsumeItemUse`, the rest through the permanent ones.
- [[UnrealTurnBasedMechanics/code/UTurnBasedParticipantManagerComponent|UTurnBasedParticipantManagerComponent]]
  ticks and resets at turn start, and writes the slot and flags.

## Gotchas

- **An item use never touches the action's permanent counters**, and can be an action the player has no permanent
  uses of.
- An item action only counts for an **Item Used** turn-end leaf. An **Action Used** leaf counts permanent uses only.
- Seeding with a permanent list: an action appears once (a later duplicate is ignored). Inventory: an empty entry is
  dropped, and so is an action the caps asset doesn't list (with a warning).
- Uses are spent by the server gate when a request commits, never client-side.

## Cross-impact

- New action-state fields → replication (`GetLifetimeReplicatedProps`), the revision bump, the actions component's
  limbo, and the debug widgets.

## Changes

- 2026-10-24 -- one permanent list (`MatchUses`, `TurnUses`), the item inventory, the item budget, `GrantItems`,
  the Item Used turn-end input. Player-actions plan, Stages 2-3.
- 2026-10-22 -- `AreTurnEndRequirementsMet` became the one turn-end evaluation (shared with AI).

## See also

- Decisions: [single authoritative counter with limbo](../../ConnectIt/_decisions/2026-09-18-action-state-single-authoritative-counter-with-limbo.md),
  [player actions, tile powers, board operations](../../ConnectIt/_decisions/2026-10-23-player-actions-tile-powers-board-operations.md)
