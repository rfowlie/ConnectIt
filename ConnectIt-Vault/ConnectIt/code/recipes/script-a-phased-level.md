---
schema: recipes
task: Script a phased level -- a level that, when something happens (a checkpoint, a trigger), moves to its next phase and changes any of its board layout, goals, scoring rule and loadouts mid-match -- with board layout assets, the layout tool and the Level director.
touches:
  - Source/ConnectIt/Public/AI/ConnectIt_LevelDirector.h (the Level director Blueprint: Begin Phase, Set Win / Lose Conditions, Set Layout, On Phase Changed)
  - Source/ConnectIt/Public/Framework/Data/ConnectIt_BoardLayoutDataAsset.h (UConnectIt_BoardLayoutDataAsset data assets, one per layout, made with the layout tool in a sandbox level)
  - Source/ConnectIt/Public/Board/Powers/ConnectIt_TileCatalogDataAsset.h (the map's tile catalog, in World Settings: which tile actor shows each power tag; its Default Tile Class for none)
  - Source/ConnectIt/Public/Board/Actor/ConnectIt_BuilderTile.h (builder tiles: what the layout tool places and captures)
  - Source/ConnectIt/Public/Board/ConnectIt_TileDataProvider.h (the catalog's tile Blueprints: Apply Layout Cell)
  - Source/ConnectIt/Public/Framework/Library/ConnectIt_BoardEventLibrary.h (the visual handlers for Tiles Removed / Tiles Added / Layout Changed / Phase Changed, once per project)
reconciled: 2026-10-23
commit: 54c1284
---

# Recipe: script a phased level

## Goal

One level in **phases**: phase 1 on a 4x1 strip ("score 4"), then at the checkpoint the board grows to 4x4, the goal becomes
"score 12", and the AI gets a new move. A loss in any phase ends the match (restart the level). A phase is **an event, not a
bundle**: the Level director begins it, every director hears **On Phase Changed**, and each changes what it owns. Each change
(the phase, a side's goals, the layout) is its own board change, played in order with its own visuals and win check.

Design: [phased-levels](../../design/phased-levels.md); decision:
[phases as events, layouts as data](../../_decisions/2026-10-22-phases-as-events-layouts-as-data.md).

## Prerequisites

- A level scripted with a Level director ([[script-a-level]]): both sides' starting goals are its Player / AI Win / Lose
  Conditions ([[add-a-win-condition]]). The starting layout is the tiles placed in the map, as for any level.
- A **tile catalog** in the map's World Settings (or the project default): the tile actor for each power tag, and its
  **Default Tile Class** for cells with none. (Without any catalog, the level config's old Default Tile Class is used.)
- **Once per project** (Blueprint):
  - **The catalog's tile Blueprints:** implement the tile data interface's **Apply Layout Cell** (take the starting values
    and the Visual Variant's look).
  - **A builder tile Blueprint** (`ConnectIt_BuilderTile`), set as Project Settings > Game > ConnectIt > **Builder Tile
    Class**: the layout tool places these.
  - **Visual handlers** for the board events, on every machine:
    - **Tiles Removed:** Unregister Tiles At; animate out; Destroy Actor; Despawn Piece At where Piece Factions isn't -1.
    - **Tiles Added:** Spawn Layout Tiles; appear; Spawn Piece At for cells with a starting piece.
    - **Layout Changed:** the camera moves to its transform.
    - **Phase Changed** (optional): a banner.
  - **The layout tool:** an editor utility widget over `Capture Layout From Level` / `Load Layout Into Level` / `Set Layout
    Camera` / `Preview Tile Catalog` / `Clear Tile Catalog Preview`, plus `Save Loaded Asset`; and a **sandbox map** whose
    World Settings name the board's level config. A captured cell keeps its builder tile's **Is Locked** and **Chain
    Partner** (as a position in the layout; loading it back restores the picker) -- [[lock-a-tile-or-chain-tiles]].

## Steps

1. **Make each phase's layout** (skip for phases that keep the board):
   1. Open the sandbox map.
   2. Lay **builder tiles** out and set each one's data (multiplier, active, shiftable, starting piece, power tag).
      **Preview Tile Catalog** to see it as the catalog's tiles (never saved; each shows in the outliner as
      "<builder> (preview)"). While previewing, **moving a preview moves its builder**: when the drag ends it snaps to
      the grid cell under it and the hidden builder goes there too. Deleting a preview leaves its builder; saving the
      map clears the preview first (press Preview again after). **Clear** before editing builder data.
   3. In the tool: pick or create a `ConnectIt Board Layout` asset (e.g. `CIBL_Tutorial3_Phase2`).
   4. **Capture**.
   5. Frame the view and **Set Camera** (optional).
   6. **Save**.

   To edit one later, **Load** it, change it, then Capture and Save again. Every cell's values are also editable in the asset
   itself.
2. **Give the phases goals that chain.** Phase 1's goal is a **checkpoint**: list it first in the Level director's Player Win
   Conditions, followed by a placeholder last goal that the phase change replaces. Or react to a trigger instead of a
   checkpoint.
3. **Begin the next phase** -- in the Level director, **On Win Checkpoint Reached** (Goal = phase 1's goal), or **On Trigger
   Fired**. Call, **in this order** (they play in the order asked):
   1. **Begin Phase (`ConnectIt.Phase.2`)** -- the phase is recorded on the board, Phase Changed plays, and every director
      hears **On Phase Changed (Phase)**.
   2. **Set Win Conditions (Player, [phase 2 goals])** (and the AI's, and Set Lose Conditions, as the phase needs) -- that
      side's goal progress restarts at 0.
   3. **Set Layout (`CIBL_..._Phase2`)** -- one board change: tiles not in the layout go (with their pieces), new tiles come
      (with their starting values), shared tiles keep their piece and multiplier, the camera moves.
   Timing **Immediately** (the default) = once the move that reached the checkpoint has played out.
4. **Other sides react in their own directors**: **On Phase Changed** in the AI director (**Set AI Loadout**, new strategies) or
   a Player director (**Set Player Loadout**). The scoring rule: the Level director's **Set Scoring Rule**.
5. **The last phase**: its goal is the side's last Win Condition; completing it wins. Phase tags: `ConnectIt.Phase.1` to `.4`
   and `.Final` (add numbers in Project Settings > Gameplay Tags as levels need them).

## Verify

- Output Log, filter `ConnectIt_`:
  - `phase 'ConnectIt.Phase.2' begins`;
  - `player win goals replaced (N)`;
  - `level operation 'ConnectIt.BoardOperation.ChangeLayout' '<layout>': -A tile(s), +B tile(s)`.
  - Each is followed by its own board change and release (one board change at a time).
- In PIE: the banner (if any), then the tiles go / come, and the camera reframes. Pieces on shared tiles stay. The UI's
  progress shows the new goal from its start. **Get Current Phase** (GameState) reads the new phase.
- A 2-player listen server: the client sees the same sequence.

## Pitfalls

- **Everything you don't change carries over**: pieces, multipliers, score, loadouts, triggers, the scoring rule -- and goal
  progress, unless the goal list is replaced.
- **Score carries over**: a score goal set in a later phase may already be met -- it then **completes at once** (Goals Changed,
  then the checkpoint or win in the same change). Set the target above the current score, or use it on purpose.
- **Order matters at a checkpoint**: set the new goals **before** the new layout. The layout's change is win-checked against
  whatever goals are in force when it plays, so the old goals would judge the new board.
- **A new layout can score the moment it appears**: starting pieces are ordinary arrivals, so a layout holding a complete line
  under the scoring rule in force scores (and stays). Use it on purpose (instant pressure) or avoid it.
- **Tiles in both layouts keep their piece and multiplier.** Clear them explicitly (Remove Piece, Destroy Tile Multiplier) if
  the phase should start clean.
- **Starting pieces on added tiles count as starting pieces** (piece-count goals measure progress from them). Pieces removed
  with their tiles don't lower that count.
- **A layout cell whose power tag has no tile class in the catalog** (or no catalog and no Default Tile Class) is on the
  board but not shown (warning: `no tile actor for (X,Y)`). Layout validation flags tags the project default catalog
  doesn't define.
- **Layouts captured before the tile catalog** named a Tile Class per cell; that field is gone. Capture them again from
  builder tiles.
- **The AI can't foresee a phase change**: it plays the current phase until the change happens, and never decides
  mid-change (it waits for the board to settle).
- **Get Current Phase runs ahead of the visuals**: it reads the committed board. For a banner timed with the board, use the
  Phase Changed event.
- **The Level director is server-only**: clients see phases, goals and layouts through the board events, the replicated board
  and the replicated rules.

## See also

- Recipes: [[script-a-level]], [[add-a-win-condition]], [[direct-the-ai-during-a-match]]
- Flow: [[place-piece-request]] (how each change is carried out and played, one at a time)
- Design: [phased-levels](../../design/phased-levels.md), [phased-levels-plan](../../design/phased-levels-plan.md)
