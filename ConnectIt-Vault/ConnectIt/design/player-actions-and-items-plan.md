---
type: plan
status: not-started
approved: true
approved-on: 2026-10-23
completed-on:
playtested: false
design: "[adventure-hub](adventure-hub.md)"
question:
decision: "[player actions, tile powers, board operations](../_decisions/2026-10-23-player-actions-tile-powers-board-operations.md)"
requires:
---

# Plan: player actions, item actions and the vocabulary

The owner's standard (2026-10-23): the plugin conforms first, then the project. Decision:
[player actions, tile powers, board operations](../_decisions/2026-10-23-player-actions-tile-powers-board-operations.md).
Written 2026-10-23 after the discussion in the logs of that day (1438 and after); **approved by the owner 2026-10-23**
(see "Settled at approval"). Adventure hub Stage 2 waits on it.

## Context

Settled with the owner:

- **Vocabulary:**
  - **Player Action:** what a participant does on their turn.
  - **Tile Power:** what a tile does by itself.
  - **Board Operation:** the change either one produces; both can share one kind.
  - "Skill" is retired, and "ability" isn't used (GAS).
- **Permanent actions** are a per-level list. Each entry: an action, **match uses** (-1 = unlimited), **turn uses**
  (always a finite number, at least 1: unlimited turn uses would break levels), a cooldown. They come back every
  level. The level's set-up sets them, or the save does (Adventure: match uses from shards).
- **Item actions sit in the player's inventory:**
  - counts per action, each capped by that action's **max held**;
  - a grant never goes over the cap, and never touches the item budget;
  - grants are kept even on a loss (the inventory is saved).
  - Item actions have **no match or turn uses of their own**.
- **Each level sets an item budget:** item uses per match and per turn, for all items together. 0 = no items (closed
  levels).
- **Using an item is its own path:**
  - the player opens the inventory and picks an action;
  - one item and one unit of the budget are spent;
  - the action's permanent counters are never touched;
  - an item can be an action the level's permanent list doesn't have.
- **Turn-end requirements:** an item use **doesn't** count as that action. It is its own requirement ("an item was
  used"), which can stand alone or be grouped with action requirements.
- **No "choose items to bring"** before a node: the whole inventory is there, and the budget limits it.
- **Renames:**
  - the plugin's `UTurnBasedAction` → `UTurnBasedPlayerAction`;
  - tags `ConnectIt.Action.*` → `ConnectIt.PlayerAction.*` and `ConnectIt.Game.State.*` → `ConnectIt.BoardOperation.*`;
  - Adventure's "skill" names → "action".
- **The 5 loadout assets are rebuilt by hand** (owner): no migration.

Each stage builds and passes the tests before the next; the editor must be closed for builds; the owner commits between
stages.

## Stage 1: rename the plugin's player action

- `UTurnBasedAction` → **`UTurnBasedPlayerAction`** (`Action/TurnBasedPlayerAction.h/.cpp`), with a class redirect so
  action Blueprints (and every asset naming the class) keep loading.
  - `UTurnBasedActionBase` (the stack entry) and `UTurnBasedSpectatorAction` (viewers) keep their names.
- Every use in the plugin and ConnectIt (48 files name the class), comments included.
- **Tests:** the existing ones, unchanged.

## Stage 2: one permanent action list (plugin)

- **`FPermanentActionConfig`:** `ActionClass`, `MatchUses` (-1 = unlimited), `TurnUses` (at least 1), `CooldownTurns`.
  - `FNumberedActionConfig` and the loadout's `NumberedActions` go.
  - The loadout keeps one `PermanentActions` list; an action appears in it once.
- **Runtime state per action:** match uses left (-1 = unlimited), uses this turn, the turn cap, cooldown. The two
  runtime structs and their getters merge (`GetPermanentRuntimeState`, `GetMatchUsesRemaining`).
- **Seeding from a list, not only an asset:** `InitialiseActionState(Loadout, PermanentActions override)`. A game can
  hand in its own list built from a save (Adventure). With no override, the loadout's list is used.
- **`ConsumeActionUse` / `CanUseAction`** check turn uses, match uses and cooldown in one place.
  - `GrantActionUses` goes: grants are items now (Stage 3). Nothing in ConnectIt calls it.
- **Turn-end requirements:** the action leaf counts permanent uses this turn, as today.
- **Tests:**
  - unlimited match uses;
  - a finite match budget running out;
  - turn uses capping a turn;
  - cooldown;
  - seeding from an override list.

## Stage 3: item inventory and item budget (plugin)

- **Inventory on the PlayerState** (replicated):
  - one entry per item action: `ActionClass`, `Count`, `MaxHeld`;
  - `GetItemCount`, `GrantItems(Class, Amount)` (clamped to `MaxHeld`; returns how many were added);
  - `OnInventoryChanged`.
  - The game seeds it at match start (`InitialiseInventory(Entries)`) and reads it back at the end.
- **Item budget per match:** `ItemMatchUses` / `ItemTurnUses` (0 = no items, the default), set by the game at match
  start. The PlayerState counts item uses this match and this turn (reset each turn).
- **Using an item:**
  - The request says it is an item use: a flag on `FTurnActionRequest`, client-supplied, so the server checks it.
  - The server gate: an item of that class held, the budget not spent, the action's own checks.
  - Then it spends one item and one unit of budget, and never the permanent counters.
  - The actions component can start any held item's action (`ActivateItemAction(Class)`), whether or not it's in the
    permanent list, and the action sends its request as an item use.
- **Turn end:** a new leaf, **Item Used** (at least N item uses this turn, any action), which groups like the others.
- **Tests:**
  - grants clamp to max held;
  - an item use spends an item and the budget, not permanent uses;
  - budget per turn and per match;
  - no items at budget 0;
  - an item action not in the permanent list can be used;
  - the Item Used leaf, alone and in a group.

## Stage 4: ConnectIt conforms

- **Tags:**
  - `ConnectIt.Action.*` → `ConnectIt.PlayerAction.*`;
  - `ConnectIt.Game.State.*` → `ConnectIt.BoardOperation.*`;
  - native tags renamed, with gameplay tag redirects in the ini so saved assets keep their tags.
  - `GetRequestType` → `GetOperationTag` on board operations; "request type" → "operation tag" in comments.
- **The request gate** (the Mediator): permanent and item uses through the plugin's new checks. An item use spends
  from the inventory and budget.
- **Seeding:** the GameMode and the AI controller seed from the loadout as today; the item budget defaults to 0.
- **The AI never uses items** (owner): its item budget stays 0.
- **The action Blueprints and the debug widget** read the merged state.
- **"Power" → "Tile Power" in names** where it stands alone, for example:
  - `FConnectItBoardEvent_PowerTriggered` → `..._TilePowerTriggered` (and its tag, getter and hook);
  - `LevelWidePowers` → `LevelWideTilePowers`;
  - `PowerReadyTurn` → `TilePowerReadyTurn`;
  - `HasTurnStartPowerReady`, `GetPowerTurnsLeft`, `FConnectItPowerChain`, `EConnectItPowerTrigger`.
  - Property and struct redirects where assets store them. **All of them** (owner, at approval): every name with a
    bare "Power" becomes "TilePower".
- **Tests:** the existing ones (renamed tags and types).

## Stage 5: Adventure conforms

- **Renames:**
  - `UConnectIt_SkillShardTable` → **`UConnectIt_ActionShardTable`**; each entry gains **`MaxHeld`**, the item cap;
  - `RequiredSkills` / `RecommendedSkills` → `RequiredActions` / `RecommendedActions`;
  - "numbered uses" → "permanent uses" (`GetPermanentUses`);
  - the Project Settings field.
- **Item budget on the node:** `ItemMatchUses` / `ItemTurnUses`.
  - Validation: a closed node with a budget above 0 is a warning, since closed means the level's own set-up.
- **The adventure-hub plan revised** (its later stages):
  - the save holds the inventory;
  - Stage 3 seeds permanent uses from shards, and the inventory and budget from the save and the node, without the
    "choose items to bring" step;
  - item use and grants are written back on return (grants kept on a loss);
  - the node panel shows the budget.
- **Tests:** the Stage 1 Adventure tests renamed; max held read through the table.

## Stage 6: content (owner, editor)

- **Rebuild the 5 loadouts** (`Content/_ConnectItNetworked/Game/Data/ActionLoadout/`) with the one permanent list.
- **Turn-end trees** that need an Item Used leaf.
- **Check that the action Blueprints load** through the class redirect.
- **An inventory button** in the action UI, for when items are first used.

## Stage 7: vault

- The decision note, updated with the item model.
- **UnrealTurnBasedMechanics code pages:** the actions component, the action stack and turn-end flows, the
  add-a-turn-action recipe, and the inventory and budget.
- **ConnectIt pages:** `runtime-state`, `place-piece-request`, the recipes that name actions, tags or "power"; and the
  domain glossary line in `CLAUDE.md`.
- `design/adventure-hub.md` and its plan; a log.

## Not in this plan

- **The AI using items:** not a design to encourage (owner). A match's AI is given everything it needs.
- **An item turn-end leaf that names a specific action** (any item use for now).
- **The inventory UI itself** beyond the button (owner, editor).

## Verification

- Each stage: build (editor closed); `Automation RunTests ConnectIt` all pass.
- **Owner, PIE, after Stage 6:**
  - a level with Place Piece (unlimited match uses, 1 per turn) and Swap (2 match uses, 1 per turn);
  - an inventory with one Shift item and an item budget of 1 per turn;
  - Swap runs out after two uses;
  - Shift can be used from the inventory once, without touching Swap's or Place Piece's counts;
  - a turn needing "Item Used" can't end until an item is used.

## Settled at approval (owner, 2026-10-23)

1. **"Power" → "Tile Power":** every name with a bare "Power" (`LevelWideTilePowers`, `TilePowerReadyTurn`, the Tile
   Power Triggered event and its tag, getter and hook, `HasTurnStartTilePowerReady`, `GetTilePowerTurnsLeft`,
   `FConnectItTilePowerChain`, `EConnectItTilePowerTrigger`...).
2. **The AI never uses items:** a match's AI has everything it requires. Its item budget stays 0.
