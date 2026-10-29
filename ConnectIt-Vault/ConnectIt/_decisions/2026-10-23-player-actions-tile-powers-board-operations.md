---
Date: 2026-10-23
status: Active
superseded by:
tags:
  - vocabulary
  - conventions
  - actions
  - adventure
---
## Decision

One vocabulary for the plugin suite and the game. The plugin conforms first, then the project.

- **Player Action:** what a participant chooses to do on their turn (Place Piece, Swap, Shift). Not "ability" (it
  would clash with GAS) and not "skill".
- **Tile Power:** what a tile does by itself (Push, Vacuum, Repel, Chained). Always "Tile Power", never just "power".
- **Board Operation:** a change to the board.
  - Player Actions and Tile Powers both produce them, and can share the same kind (a Push and a player's move can both
    be a Move Piece).
  - The Level director applies them too.
- **No "skill":** Adventure's "skills" are Player Actions. The term is retired from code, design notes and UI.
- **Two kinds of Player Action uses: permanent and item.** They differ in how they are tracked and stored:
  - **permanent** uses come back every level (in Adventure, derived from the player's shards; Place Piece's are
    everyone's);
  - **item** uses are one-time: spent, they're gone from the save.
  - This replaces the plugin's *permanent* (unlimited in a match) / *numbered* (a match budget) split, whose meaning
    clashed with Adventure's.
- **Permanent actions** are a per-level list. Each entry has match uses (-1 = unlimited) and turn uses (always
  finite, at least 1: unlimited turn uses would break levels), set by the level's set-up or the save (Adventure:
  match uses from shards), never by the action itself.
- **Item actions sit in the player's inventory** (owner, revised the same day):
  - counts per action, capped by each action's **max held**: the game's static tuning, in one data asset (owner,
    2026-10-24). An item action it doesn't list can't be held. Only held items are kept;
  - no match or turn uses of their own;
  - grants never exceed the cap, never touch the budget, and are kept even on a loss.
- **Each level sets an item budget** (item uses per match and per turn, all items together; 0 = none).
- **Using an item is its own path**, chosen by the player through an inventory: it spends one item and one unit of
  budget, never the action's permanent counters, and can be an action the level's list doesn't have.
- **Turn end:** an item use doesn't count as that action; "an item was used" is its own requirement, alone or
  grouped.
- **Renames:** the plugin's `UTurnBasedAction` → `UTurnBasedPlayerAction`; tags `ConnectIt.Action.*` →
  `ConnectIt.PlayerAction.*`, `ConnectIt.Game.State.*` → `ConnectIt.BoardOperation.*`.

## In code (built 2026-10-24 / 25)

- **Plugin (UnrealTurnBasedMechanics):**
  - [[UnrealTurnBasedMechanics/code/UTurnBasedPlayerAction|UTurnBasedPlayerAction]] (class redirect from
    `UTurnBasedAction`).
  - Permanent: [[UnrealTurnBasedMechanics/code/ActionConfig|`FPermanentActionConfig`]] (`MatchUses`, `TurnUses`,
    `CooldownTurns`) in a loadout's `PermanentActions`, or a list the game builds
    (`ATurnBasedPlayerState::InitialiseActionState(Loadout, List)`).
  - Items: `FItemActionEntry` (class + count) in the PlayerState's inventory. Caps live in
    [[UnrealTurnBasedMechanics/code/UItemActionInventoryDataAsset|UItemActionInventoryDataAsset]] (Project Settings >
    Turn Based Mechanics > Item Inventory). `GrantItems(Class, Amount)`.
  - The per-match `FItemBudget` (`SetItemBudget`).
  - The item path: `UTurnBasedActionsComponent::ActivateItemAction` → a request marked `bIsItemUse` → the server's
    `CanUseItem` / `ConsumeItemUse`.
  - Turn end: the **Item Used** leaf (`UTurnEndRequirementItemUsed`); an Action Used leaf counts permanent uses only.
  - See [[UnrealTurnBasedMechanics/code/ATurnBasedPlayerState|ATurnBasedPlayerState]].
- **ConnectIt:**
  - tags `ConnectIt.PlayerAction.*` / `ConnectIt.BoardOperation.*`, the old ones redirected; "operation tag"
    (`GetOperationTag`); every bare "Power" is "TilePower";
  - the Mediator gates both paths; **the AI never uses items** (owner);
  - Adventure: `UConnectIt_ActionShardTableDataAsset` (shard cost per use), `GetPermanentUses`, and the map node's
    `ActionDesign` / `ItemBudget`.

## Why

- **One word per concept** across the plugin and the game, and nothing that collides with Unreal (GAS "abilities").
- **Naming the producers:** "Tile Power" and "Player Action" say who triggers a Board Operation. The operation is the
  shared result, which is what lets a power and an action reuse the same operation.
- **The old split ended in a contradiction:** the plugin's *numbered* actions were budgeted, while Adventure's
  *numbered* uses were the permanent ones. Permanent / item is the difference that matters: how a use is kept.
- **Uses belong to the set-up:** the same action can be unlimited in one level, budgeted in another, and owned in
  Adventure.

## What Would Change It

- An action whose uses must be dictated by the action itself, whatever the level.
- A third way of keeping uses (e.g. uses that persist across a few levels only).

Plan: [player-actions-and-items-plan](../design/player-actions-and-items-plan.md).
