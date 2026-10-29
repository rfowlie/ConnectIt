---
type: plan
status: in-progress
approved: true
approved_on: 2026-10-23
completed_on:
started:             2026-10-24          # YYYY-MM-DD, when the first stage was built (blank until then)
stages: 7              # how many stages the plan has
current_stage: "6: content (owner, editor); Stages 1-5 and 7 built"   # the first stage not yet built ("complete" / "not started")
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
- **Design lean (owner, 2026-10-24): the core actions won't be item actions.** Place, Swap, Shift and the like are
  very likely to stay permanent, and item actions to be their own set. Not a rule; the plugin allows either. It keeps
  Adventure simpler: a permanent action and an item action rarely share a class.
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
- *As built (2026-10-24; 65/65 tests):*
  - The real count was 29 source files (the 48 included generated files). The header and `.cpp` were moved with
    `git mv`; includes and log prefixes were updated.
  - **The class redirect lives in the plugin's own config** (`Plugins/UnrealTurnBasedMechanics/Config/DefaultUnrealTurnBasedMechanics.ini`),
    so it travels with the plugin. The plugin's older function redirect (`ConstructActionInput`) now points at the new
    class.
  - The class gained a comment in the new vocabulary ("A Player Action: ..."), and an example comment lost the word
    "ability".
  - **Also fixed:** the adaptive unity build put the Adventure tests in one blob with the MinMax tests, and both
    `using namespace` blocks brought in a `TestFlags`. The Adventure tests now qualify their names instead.
  - **Still owner:** check in the editor that the action Blueprints load through the redirect (Stage 6).

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
- *As built (2026-10-24; 67/67 tests; Throughput 323k-340k nodes/s over three runs, the low edge of the usual range,
  with nothing in the search path changed):*
  - **`FPermanentActionConfig`:** `ActionClass`, `MatchUses` (-1), `TurnUses` (1, at least 1), `CooldownTurns`.
    - The runtime state holds `MatchUsesRemaining` (-1 = unlimited), `MaxUsesThisTurn`, `UsesThisTurn`,
      `CooldownTurnsRemaining`, and its `CooldownTurns`.
    - The cooldown length moved onto the state, so a use needs no config lookup. That is what makes a game-built
      list work like the loadout's.
    - The numbered structs, `NumberedActions` and `GrantActionUses` are gone.
  - **PlayerState:**
    - `InitialiseActionState(Loadout)` and `InitialiseActionState(Loadout, PermanentActions)`: a later duplicate is
      ignored, and turn uses below 1 are raised to 1;
    - `GetMatchUsesRemaining` (was `GetNumberedActionUsesRemaining`);
    - `CanUseAction` / `ConsumeActionUse` work from the runtime entry only;
    - `HasActionConfig` = any action seeded.
  - **The action:** `GetMatchUsesRemaining` (was `GetUsesRemaining`); `GetNumberedRuntimeState` is gone.
  - **The actions component builds instances from the PlayerState** (not in the plan, needed for a game-built list):
    `SyncRuntimeActions` builds one per action in the replicated state, falling back to the loadout's list until it
    arrives. It runs on initialise and on every action-state update, and only adds instances.
  - **Redirects** (plugin config):
    - functions `GetUsesRemaining` → `GetMatchUsesRemaining` (action) and `GetNumberedActionUsesRemaining` →
      `GetMatchUsesRemaining` (PlayerState);
    - the property `MaxUsesPerTurn` → `TurnUses` (an old 0 = "unlimited" is now invalid: validation errors, seeding
      raises it to 1).
  - **Loadout validation:**
    - errors: no class; a duplicate; turn uses below 1; a turn-end leaf needing more uses than the action's turn
      uses;
    - warning: match uses 0.
  - **ConnectIt:** the AI strategy and controller and the GameMode read the one list; comments and logs updated.
  - **Tests:** new `ConnectIt.PlayerActions.PermanentUses` and `.Loadout` (`Tests/ConnectIt_PlayerActionTests.cpp`),
    spawning the PlayerState in a throwaway game world.
  - **Breaks in content (owner, Stage 6):**
    - `CIW_ActionElement` and `CIW_ActionMenu` read the numbered state (`GetNumberedActionState`,
      `FNumberedActionRuntimeEntry`) and fail to compile; switch them to `GetPermanentActionState` /
      `GetMatchUsesRemaining`;
    - the loadouts' `NumberedActions` entries are dropped on load.

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
- *As built (2026-10-24; 69/69 tests; Throughput 358k nodes/s):*
  - **Structs** (`Action/ActionConfig.h`):
    - `FItemActionEntry`: `ActionClass`, `Count` (the cap is the game's, below);
    - `FItemBudget`: `MatchUses` (-1 = unlimited), `TurnUses` (0 = no items; always finite); the default is no
      items.
  - **PlayerState** (replicated: `Inventory`, `ItemBudget`, `ItemUsesThisMatch`, `ItemUsesThisTurn`):
    - `GetInventory`, `GetItemCount`, `GetItemBudget`, `GetItemUsesThisTurn` / `ThisMatch`, `HasItemBudgetLeft`,
      `CanUseItem`;
    - server: `InitialiseInventory(Entries)`, `SetItemBudget`, `ConsumeItemUse`, `GrantItems`;
    - `OnInventoryChanged`.
    - An inventory change also bumps the action-state revision, so the actions component's post-request wait works
      for item uses. The turn reset clears item uses this turn.
  - **Revised after review (owner, 2026-10-24): caps are the game's static tuning.**
    - A new plugin data asset, **`UItemActionInventoryDataAsset`** (`MaxHeld`: item action → cap, at least 1), is
      named in a new plugin project setting (`UTurnBasedMechanicsSettings`: Project Settings > Plugins > Turn Based
      Mechanics > **Item Inventory**). The plugin gains a `DeveloperSettings` dependency.
    - **Every item action has a cap:** one the asset doesn't list can't be held. Seeding drops it, a grant adds
      nothing, and both log a warning.
    - **Grants can add an action not yet held,** clamped to its cap. Only held items are kept: seeding drops entries
      holding none, and spending the last one removes its entry.
    - This replaces a per-entry `MaxHeld` and "grants only to an existing entry" (which needed every possible item
      seeded at 0).
  - **The request:** `FTurnActionRequest::bIsItemUse` (in the outcome match too).
  - **The action:** `IsItemUse()` / `SetItemUse()`.
    - Set by `ActivateItemAction` and cleared when the action finishes (complete or cancel), so a refused request
      retried stays an item use.
    - While set, `CanActivate` asks `CanUseItem` and `RequestBoardChange` marks the request.
  - **The component:** `ActivateItemAction(Class)`, and `SyncRuntimeActions` also builds instances for the
    inventory's item actions.
  - **Turn end:** the tree is evaluated against `FTurnEndUses` (an action's permanent uses this turn, plus item uses
    this turn). New leaf **Item Used** (`UTurnEndRequirementItemUsed`, `RequiredItemUses`). An item use doesn't count
    towards an Action Used leaf.
  - **The server gate is Stage 4:** until the Mediator handles `bIsItemUse`, an item request is checked like a
    permanent one.
  - **Tests:** new `ConnectIt.PlayerActions.Items` and `.ItemUsedTurnEnd`.
    - `ActivateItemAction` needs a controller and input, so it's for PIE (after Stage 4).

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
- *As built (2026-10-24; 69/69 tests; Throughput 344k-366k nodes/s):*
  - **Found on the way: the action Blueprints named themselves with the operation's tag.** `CI_Action_PlacePiece` /
    `_SwapPiece` / `_BoardShift` returned `ConnectIt.Game.State.PlacePiece` / `.SwapPieces` / `.Shift` as their
    action tag, and the `ConnectIt.Action.*` tags were used nowhere. So:
    - the three old tags **redirect to the new player action tags** (`ConnectIt.PlayerAction.PlacePiece`,
      `.SwapPiece`, `.Shift`, native), and the Blueprints return them unedited;
    - the C++ actions now return them too (`GetActionTag_Implementation`);
    - the operation tags are native `ConnectIt.BoardOperation.*`, and no asset used them, so they need no redirect.
    - `ConnectIt.Action.PlacePiece` / `.SwapPiece` redirect too; the unused `ConnectIt.Action.*` list entries are
      gone.
  - **Native tag symbols:**
    - `ConnectIt_Game_*` → `ConnectIt_BoardOperation_*`, plus `ConnectIt_PlayerAction_*`;
    - the unused `ConnectIt.Game.Pre` / `.Post`, `.Game.State.SelectTile` / `.UpdateBoard` are removed.
  - **Operation-tag names:** `GetRequestType` → `GetOperationTag`, `GetAllRequestTypes` → `GetAllOperationTags`; the
    AI's `AvailableOperationTags`, `Own/OpponentOperationTags`, `GetSupportedOperationTags`, `LoadoutGrantsOperation`,
    `GetModelledOperationTags` (property and function redirects; `CI_AIStrategy_PlaceRandom` uses two).
    - The plugin's generic `ProducesRequestType` stays (a plugin request has a type).
    - Comments say "operation tag".
  - **The No Legal Move condition's `Move`** takes `ConnectIt.BoardOperation` tags.
  - **The request gate:** an item use (`bIsItemUse`) looks the action up among held items (new plugin
    `FindItemActionClassByTag`), checks `CanUseItem` and spends with `ConsumeItemUse`. A player with only items can
    send item requests.
  - **Tile Power renames,** with redirects for every Blueprint-visible one:
    - the event `FConnectItBoardEvent_TilePowerTriggered` (its `TilePowerName`), tag
      `ConnectIt.Event.TilePowerTriggered`, getter `GetActiveTilePowerTriggeredEvent`, hook `OnTilePowerTriggered`;
    - `LevelWideTilePowers` (rule set and Level director), `TilePowerReadyTurn(s)`, `GetTilePowerTurnsLeft`;
    - `EConnectItTilePowerTrigger` (and the bitmask path), `HasTurnStartTilePowerReady`;
    - the catalog entry's `TilePower`, `MakeTilePowers`;
    - `FConnectItTilePowerChain` in `ConnectIt_TilePowerChain.h/.cpp` (moved with `git mv`), `RunSyncTilePowers`,
      `RunTurnStartTilePowers`;
    - the test `ConnectIt.Board.TurnStartTilePowers`.
    - **Left alone:** function-local variables (`Power`, `Powers`, `PowerTag`...) and test helpers (`MakeTestPower`,
      `SetPower`).
  - **A Stage 3 bug fixed:** `AreTurnEndRequirementsMet` built its `FTurnEndUses` around a temporary lambda, which
    the `TFunctionRef` outlived. The Item Used test crashed once the build layout changed. It now uses a named lambda,
    and `FTurnEndUses`' comment warns.

## Stage 5: Adventure conforms

- **Renames:**
  - `UConnectIt_SkillShardTable` → **`UConnectIt_ActionShardTableDataAsset`** (shard costs only: item caps are the plugin's
    item inventory asset, after the Stage 3 revision);
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
- **Tests:** the Stage 1 Adventure tests renamed.
- *As built (2026-10-25; 69/69 tests; Throughput 370k nodes/s):*
  - **Renames** (in the 8 Adventure files; no asset used them, so no redirects):
    - `UConnectIt_ActionShardTableDataAsset` (moved with `git mv`), `FConnectItActionShardCosts`, its `Actions` map;
    - `GetPermanentUses`; the project setting `ActionShardTable` ("Action Shard Table");
    - `RequiredActions` / `RecommendedActions` / `MissingActions`;
    - **"skill design" → `EConnectItActionDesign` / `ActionDesign`** (not in the plan's list; it was the last
      "skill");
    - comments and test names ("skill" → "action", "numbered" → "permanent"); the concept tag's description.
  - **The node's item budget is the plugin's `FItemBudget`** (`ItemBudget`; default none) rather than two ints.
    - A closed node whose budget allows items is a warning.
    - The open / closed descriptions now say the inventory and budget.
  - **Rewards keep `ItemUses`**, keyed by player action tag: now item actions into the inventory, up to their caps.
  - **The adventure-hub plan revised:**
    - a revision note in its Context;
    - Stage 1's renames recorded;
    - Stages 2, 3, 5 and 6 rewritten (the save holds the inventory; open set-ups seeded through
      `InitialiseActionState(Loadout, PermanentActions)`; inventory and budget at match start; spent items lost and
      grants kept, win or lose; the node panel shows the budget);
    - the settled points annotated.
    - **A new point to settle in its Stage 2:** tag ↔ class (the save keys player actions by tag; the plugin's
      inventory and permanent list by class).
  - **Tests:** renamed, plus a closed node's item-budget warning (in `.NodeData`).

## Stage 6: content (owner, editor)

- **Rebuild the 5 loadouts** (`Content/_ConnectItNetworked/Game/Data/ActionLoadout/`) with the one permanent list.
- **Turn-end trees** that need an Item Used leaf.
- **Check that the action Blueprints load** through the class redirect.
- **An inventory button** in the action UI, for when items are first used.

## Stage 7: vault

**Done 2026-10-29** ([log](../_logs/2026-10-29-1040.md)). Stage 6 (owner, editor) is still open, so the plan stays in
progress.

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

## Settled after Stage 3 (owner, 2026-10-24)

3. **Item caps live in a game-static data asset** (`UItemActionInventoryDataAsset`, named in the plugin's project
   settings), never passed in or kept per entry. A grant can add an action the player doesn't hold yet.
4. **An item action the asset doesn't list can't be held** (for clarity: every item action has a cap).
5. **Only held items are kept** in the inventory (no zero-count entries).
6. **Design lean:** the core actions (place, swap, shift...) are very likely not to be item actions.
