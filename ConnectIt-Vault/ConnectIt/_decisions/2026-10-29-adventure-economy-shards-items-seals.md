---
Date: 2026-10-29
status: Active
superseded by:
tags:
  - adventure
  - design
  - save
summary: 'Adventure economy: permanent uses come only from shards through a designer-set table per action (only shards saved); item actions are one-time inventory, never count for gates; gates are hand-set (required actions and/or specific / general Seals, spent with Break Seal, never on Start); no replays; the map validator plays the graph through to warn about unreachable gates and untaught concepts'
---
## Decision

**The Adventure hub runs on three currencies and hand-set gates, and the save holds only what can't be derived.**

- **Shards → permanent uses:**
  - nodes reward shards per player action;
  - the **action shard table** (designer-set, normally rising costs) turns them into **permanent uses**, back every
    level;
  - only shards are saved, and uses are derived from them;
  - Place Piece is everyone's from the start.
- **Item actions:**
  - one-time uses held in the **inventory** (capped per action by the item inventory asset);
  - granted by rewards, and spent in a level within the node's **item budget**;
  - they **never count** towards a gate;
  - items spent are lost even on a loss, and items granted are kept.
- **Gates are hand-set** on the node: **required actions** (a permanent use of each), and/or seals.
  - **Specific seals** (a tag) open the gate that names them; **general seals** count towards any gate.
  - Seals are **spent** with **Break Seal** in the node's panel, never automatically on Start.
  - A broken seal stays broken; required actions still count after it.
- **Concepts** (teaches / uses) and **recommended actions** are information for the player and the validator, never
  gates.
- **No replays for now:** a completed node isn't played again, and its rewards are granted once (`CompleteNode`).
- **Map validation plays the map through:** from a fresh save, it wins everything open and breaks a seal only when
  nothing is open. It warns about a node that never opens (what it still needs) and a concept used before any node
  teaches it.
  - These are warnings, not errors, because the seal order is the validator's guess.
  - Not run on a sub-map: the player arrives there with the parent map's progress.

## Why

- **Derive, don't store:** the save holds shards, not uses. Retuning the table can't leave a save inconsistent.
- **Items outside gates:** gates check what the player owns for good. Otherwise a gate could be passed once with a
  spent item and then leave the player stuck.
- **Seals spent by hand:** the player chooses which gate a general seal opens. An automatic spend on Start could waste
  a seal on the wrong gate.
- **A play-through rather than per-gate checks:** it uses the same rules as the game (`RunRevealWalk`, `GetNodeState`,
  `CanBreakSeal`). It catches gates that deadlock each other, and it has no separate model that could drift from the
  rules.

## Links

- Plan: [adventure-hub-plan](../design/adventure-hub-plan.md); design: [adventure-hub](../design/adventure-hub.md)
  (rounds 2-5).
- Related: [adventure saves actions by tag](2026-10-29-adventure-saves-actions-by-tag.md),
  [hub map nodes are components](2026-10-29-hub-map-nodes-are-components.md),
  [player actions, tile powers, board operations](2026-10-23-player-actions-tile-powers-board-operations.md).
- Recipe: [add an adventure node or region](../code/recipes/add-an-adventure-node-or-region.md).
