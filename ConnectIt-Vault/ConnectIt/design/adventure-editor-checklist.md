# Adventure: the owner's editor checklist

Checklist (2026-10-25) of the **in-editor work** for the start of Adventure mode, gathered from every plan's "owner,
editor" items. Tick boxes as you go. Each group says when it can start; links point at the plan with the details.
Order follows the [roadmap](adventure-mode-roadmap.md): stabilise first, then the hub, then the intro region.

## 1. Foundations -- can start now

Visuals on the display board ([tile-powers-plan](tile-powers-plan.md), Stage 2):

- [ ] Move the visual Blueprints that read the current board (tiles, score UI, `CI_PieceVisualHandler`) to
      `GetDisplayBoard()` / `OnDisplayBoardChanged`.
- [ ] Call `Shift Tiles` from the Board Shifted handler and animate the tile actors it returns.

Busy board ([one-board-change-at-a-time-plan](one-board-change-at-a-time-plan.md)):

- [ ] Action buttons / input read `Is Board Change Resolving` and its event -- greyed out while the board resolves.

Tile catalog and builder tiles ([tile-powers-plan](tile-powers-plan.md), Stage 3):

- [ ] A builder tile Blueprint with a blockout look; set it as Builder Tile Class (project settings).
- [ ] A tile catalog (Default Tile Class + entries); set it in each map's World Settings and as the Default Tile Catalog.
- [ ] Replace placed tiles with builder tiles in existing maps; capture layouts again.
- [ ] Clear `DefaultTileClass` on level configs once they have a catalog.

Grid layout tool ([phased-levels-plan](phased-levels-plan.md) Stage 6, [tile-powers-plan](tile-powers-plan.md)):

- [ ] The editor utility widget around the tool's helpers: Save Layout (Editor Asset Library), Load, **Preview /
      Clear**, save the camera view.
- [ ] A sandbox level for building layouts.

Phased levels ([phased-levels-plan](phased-levels-plan.md), Stage 5):

- [ ] Tile appear / disappear visuals (Tiles Added / Removed).
- [ ] A camera listener on Layout Changed that moves to the layout's camera transform (until the camera plan replaces
      it).

## 2. Tiles for the intro region -- can start now

First tile powers ([first-tile-powers-plan](first-tile-powers-plan.md)):

- [ ] Catalog entries for **Push, Repel, Vacuum, Chained**: their tile classes and hook animations.

Locked and chained tiles ([recipe: lock a tile or chain tiles](../code/recipes/lock-a-tile-or-chain-tiles.md)):

- [ ] A **locked** look.
- [ ] A **chained** look (chains) that snaps on Tile Lock Changed.
- [ ] A **"no partner"** icon, and the **partner's** look (from `GetChainedTilesFor`).
- [ ] The **refused-click message** from `GetPlacementBlocker` ("This tile is locked" / "already has a piece").

## 3. Actions and items -- after the player-actions plan reaches its Stage 6

([player-actions-and-items-plan](player-actions-and-items-plan.md))

- [ ] Rebuild the **5 loadouts** (`Content/_ConnectItNetworked/Game/Data/ActionLoadout/`) with the one permanent list.
- [ ] Turn-end trees that need an **Item Used** leaf.
- [ ] Check the action Blueprints load through the class redirect.
- [ ] An **inventory button** in the action UI.

## 4. Level outcomes -- after the level-outcomes plan

([level-outcomes-plan](level-outcomes-plan.md))

- [ ] An AI director Blueprint with the **rotating test opponent** strategy, for testing boards.
- [ ] **Max steps** on loadouts that want longer shifts, and the shift UI's **distance picking**.
- [ ] A **Turn Skipped** cue, and a draw on the results screen.
- [ ] Set each level's **Stuck Board outcome** on its Level director (default Lose).

## 5. The adventure hub -- after hub Stages 2-5 (and the camera plan for 5c)

([adventure-hub-plan](adventure-hub-plan.md), Stage 6)

5a. Data:

- [ ] **Seal** tags (`ConnectIt.Seal.*`) and the **shard tables** data asset (named in project settings) -- numbers
      tuned by you.
- [ ] A **`MapNodeDataAsset`** per intro node (level + preset, unlocks, gate, rewards, open / closed, concept tags,
      difficulty 1-100).

5b. The hub map:

- [ ] The intro region hub map with **node actors placed** (hidden by default), each pointing at its data asset.
- [ ] Node actor visuals per state (available, gated, completed, focus) and a **break-seal** visual.
- [ ] A small world map with the intro region as one node, two next nodes and one Seal gate.

5c. Camera ([top-down-camera-plan](top-down-camera-plan.md), Stage 4):

- [ ] Two camera profiles: **hub** (edge scroll, drag, wheel zoom, keyboard; bounds from revealed nodes) and
      **match** (wheel zoom, drag, keyboard; bounds from the board).

5d. Hub UI:

- [ ] The **node panel**: name, description, region, rewards, required (with what's missing) and recommended actions,
      concepts, gate and seal cost, item budget, **Break Seal** (enabled only when affordable) / **Start**.
- [ ] The **progress display**: shards toward the next use per action, the inventory, seals held.

## 6. Intro region levels -- once 1-5 are in place

Per level (the [level template](adventure-content-definitions.md)): purpose, concepts, actions, goals, stuck outcome,
opponent, board, rewards, difficulty.

- [ ] A map + its World Settings (board, preset, tile catalog, camera profile).
- [ ] A match preset naming its Level / Player / AI director classes.
- [ ] A Level director Blueprint (win / lose conditions for both sides, triggers, phases).
- [ ] The level's loadout (closed levels) and AI director (if there's an opponent).
- [ ] Its node data asset and node actor on the hub map.
- [ ] A PIE run through it.

## 7. PIE checks owed -- any time, the sooner the better

None of the complete plans has been played. Each plan's **Verification** lists its checks:

- [ ] [AI director pipeline](ai-director-pipeline.md)
- [ ] [Conditions and legal moves](conditions-and-legal-moves.md)
- [ ] [Three directors](three-directors-plan.md)
- [ ] [Level world settings](level-world-settings.md)
- [ ] [One board change at a time](one-board-change-at-a-time-plan.md)
- [ ] [Phased levels](phased-levels-plan.md)
- [ ] [Tile powers](tile-powers-plan.md)
- [ ] [Tile lock and chained tiles](tile-lock-and-chained-tiles-plan.md)
- [ ] [First tile powers](first-tile-powers-plan.md)

Set a plan's `playtested: true` once its checks pass.

## Not on this list yet (later)

The teaching layer (prompts, highlights), the level screens (card, goals, results, rewards reveal), player settings and
save -- all `_tasks/active.md` rows to plan later.
