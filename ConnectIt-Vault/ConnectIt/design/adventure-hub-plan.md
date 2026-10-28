---
type: plan
status: in-progress
approved: true
approved-on: 2026-10-23
completed-on:
playtested: false
design: "[adventure-hub](adventure-hub.md)"
question:
decision:
requires: "[hover-component-plan](hover-component-plan.md)"
---

# Plan: the adventure hub (node map, progress, currencies)

Design: [adventure-hub](adventure-hub.md) (rounds 1-5, 2026-10-23); background:
[adventure-modules-and-progression](adventure-modules-and-progression.md). Written 2026-10-23 in a design-only session;
**approved by the owner 2026-10-23**. Not built -- building happens in its own session.

## Context

> **Revised 2026-10-25** by the [player actions and items plan](player-actions-and-items-plan.md) (its Stage 5). The
> settled points below are kept as written; read them in the new vocabulary:
>
> - "skills" are **player actions**;
> - "numbered uses" are **permanent uses**: owned for good, from shards, back every level;
> - "itemed uses" are **item actions** in the player's **inventory**. Each has a cap from the game's item inventory
>   asset, and each node has an **item budget** limiting item uses per match and per turn;
> - there is no "choose items to bring" step: the whole inventory is available, and the budget limits it;
> - grants are kept even on a loss.
>
> Stages 2, 3, 5 and 6 below are rewritten to match; Stage 1's as-built notes are history (renamed since, see its
> last bullet).

Settled with the owner on 2026-10-23:

- **The hub is a 3D level** of **map node actors** placed and dressed by artists, each holding a
  **`MapNodeDataAsset`** (everything about the node, including which nodes it unlocks). Actors start **hidden**; on
  load the game walks the unlock graph from the starting node and reveals each unlocked node, then focuses the camera
  on the last-played node. The camera stays inside the area of revealed nodes. Top-down camera, reusing the owner's
  camera logic (to port).
- **Finishing a node unlocks all its neighbours**, except gated ones. Regions are **biomes**, each its own map; the
  **intro region** is one map that becomes **one node** on the next map once left.
- **Selecting a node never starts it**: the camera centres it and a panel shows its details and a **Start** button.
- **Gates are hand-specified**: required skills and / or **Seals**. Nodes also carry **concept tags** (teaches / uses)
  and **recommended skills** -- information for the player and the validator, never gates.
- **Skills:** Place Piece from the start. Other skills' **numbered uses** (permanent, regenerate each level) come from
  **shards** through a **designer-set table per skill** (normally rising). **Itemed uses** (one-time) are level
  rewards and never count towards requirements. A teaching level gives shards as its designer chooses (default: the
  first shard, not enough for a use).
- **Open vs closed skill design** per node: closed = the level's own loadout (teaching levels, hard puzzle levels);
  open = Place Piece + your numbered uses + any itemed uses you choose to bring.
- **Seals** are spent. **Specific** seals (a tag) open one gate; **general** seals open any. Earned at region capstones
  (specific) and optional hard / puzzle levels (general).
- **No replays for now**: a completed node isn't played again.
- **Wishlist, not in this plan:** secret links, bonus goals that give seals.

Each stage builds and passes the tests before the next; the editor must be closed for builds; the owner commits between
stages.

## Stage 1: adventure data

- **`UConnectIt_MapNodeDataAsset`** (`Public/Adventure/`):
  - identity: a stable **`NodeId`** (a GUID set when the asset is created -- the save refers to nodes by it, so renaming
    or moving the asset doesn't lose progress); display name, description, thumbnail, region (gameplay tag);
  - what it plays: the **level** (world) and **match preset** -- *or* a **sub-map** (another hub map; the node is the
    whole region, complete when that map's **exit node** is);
  - **`Unlocks`**: the nodes completing this one reveals;
  - **gate**: required skills (each: at least one numbered use), required **specific seals** (tags), **general seals**
    (count);
  - information: recommended skills, concept tags (teaches / uses);
  - **skill design**: open / closed;
  - **rewards**: shards per skill, itemed uses per skill, specific seals (tags), general seals (count).
- **Skill shard tables**: per skill, the shard thresholds for the 1st, 2nd, 3rd... numbered use (designer-set,
  normally rising) -- one data asset named in project settings (`ConnectIt_ProjectSettings`).
- **Seal tags**: `ConnectIt.Seal.*` in the tag config.
- Validation on save: a node with neither a level nor a sub-map; a closed node with no loadout in its preset; unlocks
  pointing at itself; a skill's thresholds not rising (warning only).
- Tests: shards → numbered uses through a table; a node's requirement check against a progress value.
- *As built (2026-10-23; 65/65 tests; Throughput 360k nodes/s, unchanged):*
  - **A skill is its action's tag** (`ConnectIt.Action.*`): the same key in the shard table, gates, rewards and the
    progress. The action class a loadout needs comes in Stage 3.
  - **The shard table holds each use's cost** (owner, revised the same day from running totals: a cumulative array
    invites mistakes): 3, 5, 8 gives uses at 3, 8 and 16 shards.
    - `UConnectIt_SkillShardTable` (`Skills`: tag → `FConnectItSkillShardCosts::ShardsPerUse`, each at least 1) and
      `GetNumberedUses(Skill, Shards)`, which pays for each use in turn.
    - Costs that fall are a warning.
    - `Get()` reads Project Settings > ConnectIt > **Skill Shard Table**.
  - **Soft references** (owner): the node's `MatchPreset` and `Unlocks` are `TSoftObjectPtr`, so loading a node loads
    neither its preset nor the graph behind it. Validation loads the preset to check a closed node.
  - **Item uses** (owner's rename from "itemed"): `ItemUses` on the progress and the rewards.
  - **The progress value** `FConnectItAdventureProgress` (`Public/Adventure/ConnectIt_AdventureProgress.h`): the
    save's fields from Stage 2, made here because the requirement check needs it.
    - Completed nodes, broken seals, shards, itemed uses, specific seals, general seals, last-played node.
    - `GetNumberedUses(Skill, Table)` derives uses from the shards.
  - **`UConnectIt_MapNodeDataAsset`**:
    - identity: `NodeId`, display name, description, thumbnail, `Region`;
    - play: `Level` + `MatchPreset`, or `SubMap`; `SkillDesign`;
    - map: `Unlocks`, `Gate` (`RequiredSkills`, `RequiredSeals`, `GeneralSeals`);
    - information: `RecommendedSkills`, `TeachesConcepts` / `UsesConcepts`;
    - `Rewards`: shards, itemed uses, seals, general seals.
  - **`NodeId`**: a GUID set in `PostInitProperties` for a new asset and anew in `PostDuplicate` (a copy is another
    node); read-only in the editor.
  - **`CheckRequirements(Progress, Table)`** → `FConnectItNodeRequirementCheck`: missing skills, missing specific
    seals, general seals short. Seals are skipped once the node's seal is broken; required skills always count, and
    itemed uses never do.
  - **Validation:**
    - errors: no Node Id; neither a level nor a sub-map; both (not in the plan: a node is one or the other); unlocks
      itself; a closed node whose preset's Player director gives no loadout;
    - warnings: a closed node with no preset (its level's own preset isn't checked); an empty Unlocks entry; a table
      that doesn't rise.
  - **Tag roots, under `ConnectIt.Adventure`** (owner): `ConnectIt.Adventure.Seal`, plus `.Region` and `.Concept` for
    the node's region and concepts.
  - **Tests:** new `ConnectIt.Adventure.ShardTable`, `.NodeRequirements`, `.NodeData` (in
    `Tests/ConnectIt_AdventureTests.cpp`).
  - **Renamed 2026-10-25** (player actions plan, Stage 5):
    - `UConnectIt_SkillShardTable` → `UConnectIt_ActionShardTable` (`ConnectIt_ActionShardTable.h/.cpp`; `Actions`
      map; `FConnectItActionShardCosts`), with the Project Settings field **Action Shard Table**;
    - `GetNumberedUses` → `GetPermanentUses`;
    - `RequiredSkills` / `RecommendedSkills` / `MissingSkills` → `...Actions`;
    - `EConnectItSkillDesign` / `SkillDesign` → `EConnectItActionDesign` / `ActionDesign`;
    - keys are `ConnectIt.PlayerAction.*` tags.
    - The node gained an **`ItemBudget`** (the plugin's `FItemBudget`; default none; a closed node with one is a
      warning).

## Stage 2: progress and the save

- **`UConnectIt_AdventureSaveGame`** (`USaveGame`), holding the progress value (`FConnectItAdventureProgress`):
  - completed node ids;
  - **broken seals** (gated node ids opened);
  - shards per player action (permanent uses are **derived** from shards and the action shard table, not stored);
  - the **inventory**: item actions held, by player action tag;
  - seals held (specific tags, general count);
  - the last-played node id.
- **`UConnectIt_AdventureSubsystem`** (game instance):
  - load / save (one slot to start);
  - **queries:** a node's state (hidden / available / gated, with what's missing / completed), permanent uses of a
    player action, the inventory, seals held;
  - **break a seal:** spend the seals a gated node needs; it becomes available for good;
  - **record a result:** on a win, mark complete and grant rewards (shards, item actions up to their caps, seals).
    Win or lose, write back the item uses spent and any items granted during the match. Then save.
- **Tag ↔ class (to settle here):** the save and rewards key player actions by tag, the plugin's inventory and
  permanent list by class. Either the action shard table's entries also name their action class, or a lookup finds the
  class for a tag.
- **Tests:**
  - a node's state through complete / gate / break;
  - rewards applied once;
  - derived uses grow with shards;
  - item uses don't satisfy a required action;
  - granted items respect their caps.

## Stage 3: starting a node and coming back

- **Start** from the hub: the subsystem builds the match settings (`UConnectIt_MatchSetupSubsystem` -- level,
  preset) and remembers the node being played.
- **The player's set-up:**
  - **Closed:** the preset's Player director gives the loadout, as today.
  - **Open:** at match start the player's PlayerState is seeded with Place Piece plus each player action with
    permanent uses from shards (`InitialiseActionState(Loadout, PermanentActions)`: match uses = the permanent uses,
    turn uses from the action's set-up).
  - **Either way:** the inventory is seeded from the save (`InitialiseInventory`), and the item budget from the node
    (`SetItemBudget(Node->ItemBudget)`).
- **Coming back:** when the match ends, the result (won / lost) and the PlayerState's inventory go to the
  subsystem, which records them, and the hub map loads again, focusing the node just played. A loss changes nothing
  but the inventory (items spent, items granted).
- **Tests:**
  - an open set-up built from a progress value;
  - the inventory read back after item uses and a grant;
  - a loss keeps grants and removes spent items.

## Stage 4: the hub level

- *Revised 2026-10-28 ([editor-notes-2026-10-28](editor-notes-2026-10-28.md), item 3): components, not a base actor
  (re-approved by the owner 2026-10-28).*
- **`UConnectIt_MapNodeComponent`** (replaces `AConnectIt_MapNodeActor`): added to **any** actor an artist dresses;
  holds its `MapNodeDataAsset`; hides its owner by default; a visual **state** (available, gated, completed, the
  "you were here" focus) set by the hub and exposed to Blueprint.
- **Hover / select** through the **grid hover component** ([hover-component-plan](hover-component-plan.md)) added to
  the node actor with a `Hover.MapNode` category; the hub's listener (the hub manager, or a component on the player
  controller) handles node hovers and clicks -- nodes don't react on their own.
- **Scale on hover:** a small **transform-lerp component** (start / end transform, time, forward / reverse) in
  `UnrealGameMechanics/Animation/`, beside the existing `UActorLerpComponent` (which only moves locations along
  waypoints); the listener plays it on hover / unhover.
- **Lines between nodes:** a **spline mesh** actor per unlock link (node to node, a repeating mesh or ribbon
  material), revealed with its destination node by the reveal walk.
- **The hub manager** (the hub map's GameMode or a world subsystem): on load, finds every node component in the map
  (data asset → actor), walks the graph from the map's **starting node** -- reveal the start; for each completed node,
  reveal its unlocks; recurse -- sets each revealed actor's state, then focuses the last-played node.
- **Camera** (owner's ported logic): top-down, pan with drag / wheel / mouse at the screen edge, **bounded** to the
  revealed nodes' area plus a margin; **focus** a node on selection.
- **Selection**: select → focus + open the node panel (stage 6); Start and Break Seal live in the panel only.
- **Sub-maps**: a node whose data asset names a sub-map loads that hub map on Start; leaving a sub-map (its exit node
  completed) returns to the parent map with that node complete.
- Debug: a console command / setting to **reveal every node** (the owner's worst-case performance check, and for
  layout work).
- Tests: the reveal walk on a small graph (start only; after completions; a gated node revealed but not playable).

## Stage 5: the map validator

An editor utility (map check on the hub map, plus a button) that walks the data assets from the starting node:

- every data asset reachable from the start has exactly one node actor in the hub map; no actor without a data asset;
- unreachable nodes;
- **gates**: each gate's seals can be earned on paths that don't pass through it (specific seals before their gate;
  general seals counted along the way); each required action can reach its first permanent use from shards
  available before the gate;
- **concept warnings**: a node that *uses* a concept with no node that *teaches* it on any path to it;
- a sub-map with no exit node.

Warnings and errors only -- it never changes data.

## Stage 6: UI and art (owner, editor)

- Node panel: name, description, region, rewards, required (with what's missing) and recommended actions, concepts,
  gate and its seal cost, the **item budget**, **Break Seal** / **Start**.
- **Breaking a seal** (owner, 2026-10-23): selecting a gated node opens its panel with a **Break Seal** button,
  **interactable only when the player can afford it** (the subsystem's query says so, and why not). Clicking it breaks
  the seal through the subsystem (spends the seals, records it, saves), plays a **break visual** on the node actor, and
  the node turns available.
- Progress display: shards toward the next permanent use per action, the inventory, seals held.
- Node actor visuals per state; region dressing; the camera port.
- The intro region map and its node on the world map.

## Stage 7: vault

Decision notes (the hub as a 3D level with revealed node actors; shards, permanent and item actions, and Seals);
`code/` recipe "add an
adventure node / region"; update [adventure-hub](adventure-hub.md) and
[adventure-modules-and-progression](adventure-modules-and-progression.md); log.

## Not in this plan

- Secret links; bonus goals that give seals (both wishlist in `_tasks/active.md`).
- Replays / challenge versions of completed levels.
- The content itself: the intro region's levels (the Level 1 draft), other regions.
- Several save slots.

## Verification

- Each stage: build (editor closed); `Automation RunTests ConnectIt` all pass.
- Owner, PIE: a three-node test map -- start node available; complete it, its two neighbours appear, one gated by a
  seal; earn the seal elsewhere, break it, play it; quit and relaunch, progress kept; the camera stays inside the
  revealed area; selecting never starts a level.

## Settled before approval (owner, 2026-10-23)

1. Nodes are identified in the save by a GUID on the data asset (renames / moves keep progress).
2. Seals are spent with the **Break Seal** button in the selected gated node's panel -- enabled only when affordable;
   clicking breaks the seal, shows a visual and saves; the node stays open. Never automatic on Start.
3. On a loss, only the itemed uses actually spent are lost; nothing else changes. *(2026-10-25: items spent are
   lost, items granted are kept -- the inventory is the save's.)*
4. Numbered uses are derived from shards (only shards are saved) -- less data, more error-proof. *(Now "permanent
   uses".)*
