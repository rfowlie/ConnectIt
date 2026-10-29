---
schema: code
kind: USTRUCT ×5
role: primary
source:
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/ActionConfig.h
reconciled: 2026-10-29
commit: 193c1c9
---

# ActionConfig (permanent actions, items, item budget)

## What it is

The data that says how often a participant may use their actions:
- **permanent actions**: a list, the same every match;
- **item actions**: an inventory, one-time uses the player keeps;
- the match's **item budget**.

The numbers belong to the set-up (a loadout, or a list the game builds), never to the action class.

## When you touch this

- Changing how uses, caps or cooldowns are expressed.
- Seeding a player from something other than a loadout (a save, a map node).

## Entry points

- **`FPermanentActionConfig`** (one per action, in a loadout's `PermanentActions` or a game-built list):
  - `ActionClass`;
  - `MatchUses` (-1 = unlimited);
  - `TurnUses` (at least 1: a turn always has a limit, or it could go on forever);
  - `CooldownTurns` (0 = none);
  - `HasUnlimitedMatchUses()`.
- **`FPermanentActionRuntimeState`** (live, on the PlayerState):
  - `MatchUsesRemaining`;
  - `MaxUsesThisTurn`: the *effective* per-turn cap, seeded from `TurnUses` and not reset each turn, so game logic
    can change it for a while;
  - `UsesThisTurn` (reset each turn);
  - `CooldownTurnsRemaining`, `CooldownTurns`.

  **`FPermanentActionRuntimeEntry`** pairs it with its class; the array replicates.
- **`FItemActionEntry`:** `ActionClass` + `Count` held. It has no match or turn uses of its own.
- **`FItemBudget`** (set per match by the game, e.g. a map node):
  - `MatchUses` (-1 = unlimited) and `TurnUses` (0 = none);
  - both are for **all item actions together**;
  - the default allows no items.

## Collaborators

- [[UnrealTurnBasedMechanics/code/UActionLoadoutDataAsset|UActionLoadoutDataAsset]] holds `PermanentActions`.
- [[UnrealTurnBasedMechanics/code/ATurnBasedPlayerState|ATurnBasedPlayerState]] seeds runtime state from it and
  holds the inventory and budget.
- How many of an item a player can hold is
  [[UnrealTurnBasedMechanics/code/UItemActionInventoryDataAsset|UItemActionInventoryDataAsset]]'s, never the
  entry's.

## Gotchas

- **An action appears once** in a permanent list; a later entry for the same class is ignored.
- `TurnUses` can't be unlimited, and neither can the item budget's.
- An item use spends the item and the budget, never the action's permanent counters.

## Cross-impact

- New fields → the PlayerState's seeding (`InitialiseActionState`), its checks (`CanUseAction`, `CanUseItem`), the
  loadout's `IsDataValid`, and any game that builds its own list (ConnectIt's Adventure: `GetPermanentUses`, the map
  node's `ItemBudget`).

## Changes

- 2026-10-24 -- **one permanent list:** `FNumberedActionConfig` / `NumberedActions` removed. `MaxUsesPerTurn` became
  `TurnUses` (at least 1), `MatchUses` was added (-1 = unlimited), `FItemActionEntry` and `FItemBudget` were added.
  Player-actions plan, Stages 2-3.

## See also

- Decision: [player actions, tile powers, board operations](../../ConnectIt/_decisions/2026-10-23-player-actions-tile-powers-board-operations.md)
