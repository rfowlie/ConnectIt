---
schema: recipes
task: Add an Adventure node (a level on a hub map, with its gate and rewards) or a region (a sub-map node), and check that the map can still be finished.
touches:
  - Source/ConnectIt/Public/Adventure/ConnectIt_MapNodeDataAsset.h (identity, Level / SubMap, gate, rewards, concepts)
  - Source/ConnectIt/Public/Adventure/ConnectIt_ActionShardTableDataAsset.h (shards per permanent use; each action's class)
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/ItemActionInventoryDataAsset.h (item caps)
  - Source/ConnectIt/Public/Adventure/ConnectIt_MapNodeComponent.h (the node on a hub map actor)
  - Source/ConnectIt/Private/Framework/Data/ConnectIt_WorldSettings.cpp (hub map validation)
reconciled: 2026-10-29
commit: 81ac444
status: 'current'
---

# Recipe: add an adventure node or region

## Goal

A new node on a hub map that the player can reach, open and win, with rewards that feed the nodes after it. Or a whole
region: a node that opens its own hub map. Decision: [adventure economy](../../_decisions/2026-10-29-adventure-economy-shards-items-seals.md).

## Prerequisites

- The hub map it goes on: [build a hub map](build-a-hub-map.md).
- For a level node: the level and its match preset, built like any match map.
- The game's **action shard table** and **item inventory asset** list every action the node gates on or rewards.
  Each shard table entry needs its **Action Class**.

## Steps

1. **Create a Map Node Data Asset.** Its Node Id is set for you; never edit it, because the save refers to the node by
   it. A duplicated asset gets a new Node Id.
   - Fill in Display Name, Description, Thumbnail and Region.
2. **What it plays:** one of these, never both:
   - a **level node:** Level + Match Preset (empty = the level's default), Action Design, and Item Budget (Open: Place
     Piece + permanent uses + the inventory within the budget; Closed: the level's own loadout, budget normally 0);
   - a **region (sub-map node):** Sub Map = another hub map, which needs one node flagged **Exit**. The node completes
     when that map's Exit is won, and gets its own Rewards then.
3. **The graph:** add the new node to the **Unlocks** of the node(s) that should reveal it, and set its own Unlocks.
   Winning a node reveals all of its unlocks.
4. **Gate** (optional):
   - **Required Actions:** a permanent use of each. Item uses don't count.
   - **Required Seals** (specific) and/or **General Seals:** the player spends them with Break Seal in the node panel.
5. **Rewards:**
   - **Shards** per action (the table turns them into permanent uses);
   - **Item Uses** (capped by the item inventory asset; an action it doesn't list is skipped);
   - **Seals** (a region capstone's) and **General Seals** (optional hard or puzzle levels).
6. **Information:** Teaches / Uses Concepts and Recommended Actions. They are never gates; the validator reads the
   concepts.
7. **Place it:** add a Map Node component with this asset to an actor on the hub map ([build a hub map](build-a-hub-map.md)).
8. **Validate:**
   - Validate Data on the node asset: a level and a sub-map together, or neither.
   - Validate Data on the hub map:
     - hub map checks: Start / Exit, one actor per node, off-map unlocks, unreachable nodes;
     - the **play-through:** a node that never opens and what it still needs (a seal nobody gives before it, or too few
       shards for a required action's first use), and a concept used before it's taught.

## Verify

- `ConnectIt.Adventure.RevealAll 1` in PIE to see it placed and linked.
- Play the path to it from a fresh save: revealed when its unlocker is won, gated until its gate is met or broken.
- Tests for the rules: `ConnectIt.Adventure.*`, especially `.HubProgression` for the validator.

## Gotchas

- **The play-through is a guess about seal order:** it breaks a seal only when nothing else is open, preferring nodes
  whose actions are met. A warning can mean "only if the player spends seals in a particular order", so read it before
  changing data.
- **Sub-maps aren't played through from the parent:** only a sub-map node's own rewards count there, not what its map
  gives. Sub-maps themselves skip the play-through.
- **Unlocks across maps** aren't followed: a node unlocks only nodes on its own map. A region joins the next map as one
  node.
