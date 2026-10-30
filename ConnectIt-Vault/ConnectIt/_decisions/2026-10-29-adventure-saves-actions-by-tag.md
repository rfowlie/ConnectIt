---
Date: 2026-10-29
status: Active
superseded by:
tags:
  - adventure
  - save
  - actions
summary: 'The Adventure save and rewards key player actions by tag, never by class (save stability); the action shard table names each entry''s class, and item actions find theirs through the item inventory asset'
---
## Decision

**The Adventure save, node rewards and shard table keep player actions by tag (`ConnectIt.PlayerAction.*`).** Where
the match needs the class (the turn-based plugin's permanent list and inventory), it is looked up:

- **Permanent actions:** each action shard table entry names its **Action Class**. Validation requires it, and requires
  that its tag is the entry's key.
- **Item actions:** the item inventory asset already lists every item action by class; the one with the tag is found
  there.
- `FConnectItAdventureProgress::FindActionClass(Tag, Table, Items)` does both. The inventory is written back to the save
  by each class's tag (`GetTagForClass`).

## Why

- **Save stability (owner):** a tag survives renaming or moving an action Blueprint. A class path in a save would lose
  the player's shards and items for that action unless a path redirect were kept forever. This is the reason nodes are
  saved by GUID, too.
- **No new list to keep in sync:** both class lists already exist (the shard table's entries, the item caps).

Rejected:
- a separate tag → class asset (one more list to maintain);
- keying the save and rewards by class (fragile saves, and it reverses Stage 1's "an action is its tag").

## What Would Change It

- An action that is neither in the shard table nor an item but still needs a class from the save. It would need an
  entry in one of the two.

Plan: [adventure-hub-plan](../design/adventure-hub-plan.md), Stage 2. Related:
[action config keyed by class, not tag](2026-09-18-action-config-keyed-by-class-not-tag.md) (inside a match, classes
stay the key).
