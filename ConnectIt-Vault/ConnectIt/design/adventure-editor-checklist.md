# Adventure: the owner's editor checklist

Checklist of the **in-editor work** on the way to Adventure mode, gathered from every plan's "owner, editor" items.
**Rewritten 2026-10-29** against what is built now (the first version, 2026-10-25, predated most of this). Tick boxes as
you go; links point at the plan with the details.

How it is sorted: **A** can be done now and will not be redone; **B** is waiting on a plan that will change how the work
is done -- doing it now means doing it twice; **C** is the play-testing every finished plan still owes.

## A. Do now

**A1. Resave renamed assets** ([data-asset-naming-plan](data-asset-naming-plan.md))
- [ ] Open and resave, so the class redirects can be dropped later: every map (World Settings), match preset, tile
      catalog, board layout, level catalog, shard table and match-condition asset. Fix anything that fails to load.

**A2. The tile catalog** ([tile-catalog-effects-plan](tile-catalog-effects-plan.md))
- [ ] Entries are now an **array** (old map entries were dropped -- only `CIDTC_Test` had any): add an entry for each of
      **Push, Repel, Vacuum, Chained** -- tile class plus the power's values (only the relevant ones show; the trigger mask
      is still exposed).
- [ ] Set a **Default Tile Class**; set the catalog in each map's **World Settings** and as the **Default Tile Catalog**
      (Project Settings > ConnectIt).
- [ ] Each entry's tile class must derive from `ConnectIt_TileBase` (your push tile is a child of `CI_GridTileBase` --
      check that chain, or the catalog will not accept it).

**A3. Builder tiles and layouts** ([tile-powers-plan](tile-powers-plan.md) Stage 3)
- [ ] A **builder tile Blueprint** with a blockout look; set it as **Builder Tile Class** in project settings.
- [ ] Replace placed tiles with builder tiles in existing maps; **capture layouts again**.
- [ ] Clear `DefaultTileClass` on the level configs once their maps have a catalog.
- [ ] Preview / Clear buttons on the layout tool widget; a sandbox level for building layouts.

**A4. Hover clean-up** ([hover-component-plan](hover-component-plan.md) Stage 4)
- [ ] `CI_GridTileBase`: remove its manual *On Grid Tile Begin Cursor Over* broadcast (C++ raises hover now) and any look
      driven directly by cursor-over. Keep the **tag-driven** state looks for now -- they change with the cursor plan (B2).
- [ ] Confirm **pieces do not block the cursor** (the hovered tile gives the piece).
- [ ] **Retest the push tile's hover** (benched earlier).

**A5. Loadouts and actions** ([player-actions-and-items-plan](player-actions-and-items-plan.md) Stage 6)
- [ ] **Rebuild the 5 loadouts** (`Content/_ConnectItNetworked/Game/Data/ActionLoadout/`) with the one permanent list.
- [ ] Turn-end trees that need an **Item Used** leaf.
- [ ] Open every action Blueprint -- they must load through the class redirect.
- [ ] *(Later, with the UI)* an inventory button in the action UI.

**A6. Start the hub's data** ([adventure-hub-plan](adventure-hub-plan.md) Stages 1-2 are built)
- [ ] A **shard table** data asset (no default numbers -- you tune them), named in project settings.
- [ ] A few **`MapNodeDataAsset`s** to exercise it (display name, region, `Unlocks`, gate, rewards, open / closed);
      the levels they play can be stubs for now.

**A7. Check the editor features that were built** (each plan's Verification)
- [ ] [World Settings validation](world-settings-validation-plan.md): Validate Data on the main menu / hub map is quiet;
      a match map missing its preset warns.
- [ ] [Layout preview sync](layout-preview-sync-plan.md): Preview → drag a preview two cells → Clear → its builder is at
      the new cell; save while previewing → no preview tiles in the saved map.
- [ ] A catalog entry shows only Push's own values for Push, Vacuum's for Vacuum.

## B. Wait -- a plan will change how this is done

**B1. The board visualizer** ([board-visualizer-plan](board-visualizer-plan.md); Stage 1 built, Stage 6 is your part)
Replaces `CI_PieceVisualHandler` and gives tile hooks a completion handle. **Do not rewire the old handler** (to the
display board, Shift Tiles, the Tiles Added / Removed handlers, the layout camera listener) -- that work becomes a
`BP_BoardVisualizer`. The cost of waiting: until then, chains and power events do not animate properly; ordinary levels
without powers are fine.
- [ ] *(then)* `BP_BoardVisualizer`; set it on the level configs; move piece visuals into piece hooks; tile hooks use the
      handle; delete the old handler.

**B2. Cursors and tile states** ([action-cursors-plan](action-cursors-plan.md), not started)
- [ ] *(then)* cursor Blueprints (hover variants, a marker); tile Blueprints implement **`OnCursorStateChanged`**
      (lands together with the actions switching over).

**B3. Locked / chained tile looks** ([tile-lock-and-chained-tiles-plan](tile-lock-and-chained-tiles-plan.md); recipe
[lock-a-tile-or-chain-tiles](../code/recipes/lock-a-tile-or-chain-tiles.md))
- [ ] The art can be made now (a locked look, chains, a "no partner" icon, the partner's look); **wire it after B1**,
      since the hooks it uses change.
- [ ] The refused-click message from `GetPlacementBlocker` (UI).

**B4. The hub map** ([adventure-hub-map-plan](adventure-hub-map-plan.md), [top-down-camera-plan](top-down-camera-plan.md),
hub plan Stage 6)
- [ ] The intro region's map with node actors (a Start flag on one), node visuals per state, a break-seal visual, link
      actors; a small world map with the intro region as one node.
- [ ] Two camera profiles (hub, match); the node panel (Break Seal / Start) and the progress display.

**B5. Level outcomes** ([level-outcomes-plan](level-outcomes-plan.md), not started)
- [ ] A rotating test-opponent AI director; max steps on loadouts that want longer shifts and the shift distance UI; a
      Turn Skipped cue and a draw result; each level's Stuck Board outcome.

## C. PIE checks owed

**Every finished plan is still `playtested: false`.** Rather than one pass per plan, one short smoke run covers most of
them -- then set `playtested: true` on the plans it exercised:

- [ ] **A classic match vs the AI:** place, score, win; a swap and a shift that score. *(directors, conditions, rules,
      legal moves, the AI pipeline, level world settings)*
- [ ] **A 2-player PIE** (a client): place / swap / shift animate on both; a move can't be sent while the board is busy.
      *(one board change at a time, the display board)*
- [ ] **A level with a Push, a Repel, a Vacuum and a Chained tile** (builders → catalog): the chain plays event by event;
      a locked tile refuses a placement; a chained tile opens when its partner gets a piece. *(tile powers, first tile
      powers, lock and chained tiles)*
- [ ] **A phased level** (Change Layout / Begin Phase): tiles appear and disappear, the board reframes. *(phased levels)*
- [ ] **Loadouts:** Swap runs out after its match uses; an item respects the item budget. *(player actions and items)*

## Not on this list yet

The teaching layer, the level screens (card, goals, results, rewards reveal), player settings and save -- the plan-later
rows in `_tasks/active.md`.
