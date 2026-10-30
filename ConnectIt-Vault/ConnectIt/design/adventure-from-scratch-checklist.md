# Adventure mode from scratch: the editor checklist

A step-by-step checklist of the **editor work** to get Adventure mode running, written as if **no editor work has been
done yet**. Follow it in order: each phase ends with something you can run, so a problem shows up in the phase that
caused it. Tick boxes as you go.

**Written 2026-10-29. How it was derived:** from the C++ headers, the recipes in `code/recipes/`, the plans in this folder
and the project's config -- *not* by opening the existing Blueprints, so a class name here is the **C++ parent** to
reparent or subclass, not a name you already have. Where a step needs code that isn't built yet, it says so and points at
the plan.

The existing, less structured list of what is outstanding in the current project is
[adventure-editor-checklist](adventure-editor-checklist.md); this note is the full path.

## What is built and what isn't (2026-10-29)

| Needed for | State |
| --- | --- |
| Matches: rules, directors, conditions, loadouts, AI, board events, display board | built |
| Tiles: catalog, builder tiles, layouts, locked / chained tiles, the first powers (Push, Repel, Vacuum, Chained) | built |
| Phased levels (Begin Phase, Set Layout) | built |
| The board visualizer (one per map, a function per event, completion handles) | built |
| Player actions and items (permanent actions, item inventory and budget) | built |
| The Adventure data, save and subsystem; the hub map (hub game mode, hub component, node components) | built |
| **Action cursors** (cursors owned by actions, tile `CursorState`) | **in progress** ([action-cursors-plan](action-cursors-plan.md)) |
| **Top-down camera** (rig pawn, edge scroll / drag / zoom, framing, focus) | **not built** ([top-down-camera-plan](top-down-camera-plan.md)) |
| **Level outcomes** (No Moves, Stuck Player, Stuck Board, Turns condition, test opponent, shift distance) | **not built** ([level-outcomes-plan](level-outcomes-plan.md)) |
| **Actor pool async + pre-warming + pooled tiles** | **not built** ([actor-pool-plan](actor-pool-plan.md)) |
| The teaching layer, level screens, player settings and save | **not designed yet** (`_tasks/active.md`) |

So the **hub works but has no camera of its own yet** (Phase 6 says what to do until the camera plan is built).

---

## Phase 0 -- Project baseline (once)

- [ ] **Plugins:** open the project; confirm these are enabled: `GameplayAbilities`, `UnrealGameMechanics`,
      `UnrealGameIntelligence`, `UnrealGridMechanics`, `UnrealTurnBasedMechanics`, `UnrealCodingUtils`,
      `UnrealSkinMechanics`, and the engine's Enhanced Input. (`UnrealUnsorted` is deliberately **not** enabled.)
- [ ] **Gameplay tags:** the native tags (actions, events, tile powers) need nothing. The **authored families** are already
      in `Config/DefaultGameplayTags.ini` as parents -- you add the children you use: `ConnectIt.Adventure.Region.<Name>`,
      `ConnectIt.Adventure.Seal.<Name>`, `ConnectIt.Adventure.Concept.*` (the list in
      [adventure-content-definitions](adventure-content-definitions.md)), `ConnectIt.Phase.1..4 / Final` (already there).
- [ ] **Content folders:** make `Levels/<LevelName>/` (a level's map and everything only it uses) and `Shared/<Type>/`
      (anything two levels use), with the prefixes you use (`CILC_` level config, `CIMP_` match preset, `CIWC_` condition,
      `CIAL_` loadout) -- see [content-folders](content-folders.md). Move assets only in the Content Browser, then **Fix Up
      Redirectors**.
- [ ] **Cursor trace:** decide the trace channel the player controller's cursor uses (default: Visibility). Tiles must
      **block** it; pieces must **ignore** it (the hovered tile gives the piece).

**Done when:** the project opens with no missing-plugin or tag errors.

## Phase 1 -- The actors (Blueprints)

Make these once; every level reuses them.

- [ ] **Tile Blueprint** -- parent `ConnectIt_TileBase`. A mesh with collision that **blocks the cursor trace**. (It already
      has a hover component in C++; do not add your own *On Begin Cursor Over* broadcast.) Leave the state looks and hooks
      for later (Phases 4-5).
- [ ] **Piece Blueprint** -- parent `ConnectIt_PieceBase`. A mesh; a look per faction (**On Faction Visual Update**); the
      pool's activate / deactivate if it has visuals; collision that **ignores the cursor**.
- [ ] **Builder tile Blueprint** -- parent `ConnectIt_BuilderTile`, with a blockout look. Set it as **Project Settings >
      Game > ConnectIt > Builder Tile Class**. (Builders are what you place in a map; they become the catalog's tiles when
      the match starts.)
- [ ] **Board visualizer Blueprint** -- parent `ConnectIt_BoardVisualizer`. Optional at first: with none set, the C++ base
      just calls the actors' own hooks. Make it when you want effects ([add-visuals-for-a-board-event](../code/recipes/add-visuals-for-a-board-event.md)).

**Done when:** the Blueprints compile and open with no warnings.

## Phase 2 -- The board: catalog, config, a test map

- [ ] **Tile catalog** -- data asset `ConnectIt Tile Catalog`. Set **Default Tile Class** = your tile Blueprint. (Entries for
      powers come in Phase 5.) Set it as **Project Settings > ConnectIt > Default Tile Catalog**.
- [ ] **Level config** -- data asset `ConnectIt Level Config` (a board; many maps can share it). Set **Piece Actor Class**
      (your piece Blueprint), **Piece Pool Initial Size** (the default is 32), **Tile Registry** and **Piece Registry**
      (pick the ConnectIt registry classes), **Grid Definition** (cell size -- must match your tile spacing), and
      **Visualizer Class** if you made one.
- [ ] **A test map** (`Levels/TestBoard/`): in **World Settings** set the **Level Config**, a **Tile Catalog** (or leave the
      project default), a **Default Match Preset** (Phase 3), and the **Game Mode override** (Phase 3). Lay out a grid of
      **builder tiles** at the cell spacing; give each its starting values (multiplier, active, shiftable, starting piece,
      tile power tag) as needed.
- [ ] A **PlayerStart**, and a **camera** to see the board: a placed camera actor / the default pawn's view is enough for
      now (the camera plan comes later).

**Done when:** *Validate Data* on the map shows no errors (match maps warn if the preset is missing).

## Phase 3 -- Rules and match setup

- [ ] **Game framework Blueprints** -- children of `ConnectIt_GameMode`, `ConnectIt_GameState`, `ConnectIt_PlayerState`,
      `ConnectIt_PlayerController` and `ConnectIt_HUD`. In the **GameMode**: **Match Type = Adventure**, **AI Controller
      Class** = a Blueprint child of `ConnectIt_AIController`, **Num Factions = 2**, **Board Change Timeout Seconds** (the
      default is fine). Point the GameMode's default classes at the others. Set this GameMode in the test map's World
      Settings.
- [ ] **Input:** an **Input Mapping Context** and the **Input Actions** the actions bind (select, cancel, end turn, and
      one per action), with the input tag bindings set on each action (below). The controller adds the mapping context.
- [ ] **Player action Blueprints** -- children of `ConnectIt_PlacePieceAction`, `ConnectIt_SwapPieceAction`,
      `ConnectIt_BoardShiftAction` (the actions Adventure ships). On each: display name and icon, input bindings,
      **Requires Selection**, and its tile-state tags (`Tag Action Grid State`, `Tag Action Mouse Hover`...). *(When the
      action cursors plan lands these gain cursor classes -- see Phase 4.)*
- [ ] **Loadouts** -- `Action Loadout` data assets: the **permanent action list** (Place Piece: unlimited match uses, 1 per
      turn; Swap: its match uses and per-turn cap), the **item inventory** and **item budget** (0 for closed levels), and the
      **turn-end requirements** tree (e.g. "used Place once"). One for the player, one for the AI.
- [ ] **Conditions** -- `Match Condition` data assets for the goals ([add-a-win-condition](../code/recipes/add-a-win-condition.md)):
      e.g. *Score Threshold* for the win.
- [ ] **Directors** (Blueprints; Class Defaults hold their setup):
      - **Level director** (`ConnectIt_LevelDirector`): the **Player / AI Win and Lose Conditions** (both sides' goals live
        here), triggers, level changes ([script-a-level](../code/recipes/script-a-level.md)).
      - **Player director** (`ConnectIt_PlayerDirector`): the player's **Starting Loadout**.
      - **AI director** (`ConnectIt_AIDirector`): the AI's **Starting Loadout** and strategy -- the MinMax (Classic)
        default is fine to start ([set-up-a-custom-ai-opponent](../code/recipes/set-up-a-custom-ai-opponent.md)).
- [ ] **Match preset** -- `ConnectIt Match Preset` data asset: the **scoring rule** (Lines, length 4), and the three
      **director classes** (Level and Player required). Name it as the map's **Default Match Preset**.

**Done when:** the preset validates with no errors.

## Phase 4 -- A playable match (PIE)

- [ ] **Single player vs the AI:** play to a win. Place a piece, score a line, watch the score; the AI answers; the match
      ends.
- [ ] **Tile and piece looks:** the tile Blueprint reacts to the tags the actions send (`SendGameplayTag`) to show hover /
      valid / invalid. *(The action cursors plan changes this to `OnCursorStateChanged` and pooled cursors; until it is
      built, use the tag path.)*
- [ ] **Two players** (PIE, 2 clients): both see the same board and events; a move can't be sent while the board is busy.
- [ ] **A swap and a shift** that score.
- [ ] Set `playtested: true` on the plans this exercised (see [adventure-editor-checklist](adventure-editor-checklist.md) part C).

**Done when:** a full match plays on both machines with no errors in the log.

## Phase 5 -- Tile powers and tile features (needed for the lock / chain teaching level)

- [ ] **Tile classes** for each power -- Blueprints deriving from `ConnectIt_TileBase`: Push, Repel, Vacuum, Chained.
- [ ] **Catalog entries** (the tile catalog is an **array**): one per power -- the tile class and the power's own values
      (only the ones it uses show; the trigger mask is exposed). The **entry's effect decides the tag**.
- [ ] **Builder tiles** with the **Tile Power Tag** set; for Chained, the **Chain Partner Tile** (a partner is optional).
- [ ] **Locked** tiles: tick **Is Locked** on a builder; a locked tile refuses placement ([lock-a-tile-or-chain-tiles](../code/recipes/lock-a-tile-or-chain-tiles.md)).
- [ ] **Hooks / visuals** on the tile classes and the visualizer: power fired, lock changed, chain partner changed,
      multiplier changed, piece enter / exit ([add-a-tile-power](../code/recipes/add-a-tile-power.md)). Each animation holds the
      event's completion handle until it ends.
- [ ] **Layouts for a phased level:** the layout tool (editor utility widget + a sandbox map): lay builders out, **Capture**,
      **Save** a `Board Layout` asset per phase ([script-a-phased-level](../code/recipes/script-a-phased-level.md)).

**Done when:** a map with a Push, Repel, Vacuum and Chained tile plays: a power fires and the chain plays event by event.

## Phase 6 -- The Adventure hub

- [ ] **Action shard table** -- data asset `Action Shard Table`; per action, the shards for the 1st, 2nd... permanent use
      (**no defaults -- you tune them**). Set it as **Project Settings > ConnectIt > Action Shard Table**.
- [ ] **Authored tags:** add the `ConnectIt.Adventure.Region.*` and `.Seal.*` children you need.
- [ ] **Map node data assets** (`Map Node`), one per node of the intro region: display name, region, the **Level** (a soft
      world) and **Match Preset** (or a **Sub Map** for a region node), **Action Design** (open / closed), **Item Budget**,
      **Unlocks** (the nodes finishing it reveals), the **gate** (required actions, required specific seals, general
      seals), recommended actions, concept tags (teaches / uses), **difficulty (1-100)**, and the **rewards** (shards,
      item uses, seals). The levels they play can be stubs at first.
- [ ] **The hub game mode** -- Blueprint child of `ConnectIt_GameMode_AdventureHub`. On its **Adventure Hub** component:
      **Link Actor Class** (a Blueprint child of `ConnectIt_HubLinkActor`: a spline mesh between two nodes) and **Framing
      Margin**.
- [ ] **The hub map** (`Levels/Hub_Intro/`): World Settings **Game Mode = the hub game mode**; place a **node actor** per node
      (any actor you dress -- a mesh) with a **Map Node component** (set its **Node Data**) and a **Grid Hover component**
      (category MapNode). Tick **Is Start** on exactly one (validation errors on none or on two); in a sub-map tick **Is
      Exit** on one.
- [ ] **Node visuals:** react to each node component's **On State Changed** (hidden / available / gated / completed, plus
      the focus flag); a **break-seal** visual; scale on hover if you want it.
- [ ] **A camera for the hub.** *The camera plan is not built:* until it is, use a placed top-down camera that frames the
      map, and the hub component's **Focus On Node** / `GetRevealedBounds` to position it by hand. When the plan lands you
      make the two camera profiles (hub, match) and a rig pawn.
- [ ] **The node panel** (a widget): listen to the hub component's **On Node Selected**; show the node's name, description,
      region, rewards, requirements (what's missing), recommended actions, concepts and gate; a **Break Seal** button
      (enabled only when **Can Break Seal**; calls **Break Seal**) and a **Start** button (**Start Node**). Selecting a node
      never starts it.
- [ ] **Progress display:** shards toward the next use per action, items, seals held (from the Adventure subsystem).
- [ ] **Coming back:** the results screen's button calls **Return To Hub**; the match end records the result.
- [ ] A **new game / continue** entry in the main menu (the save loads at the hub; a fuller version is a plan-later task).

**Done when:** a fresh save shows only the start node; finishing its level reveals the next; a gated node shows what it
needs, and breaking its seal opens it; quit and relaunch keeps progress.

## Phase 7 -- The first adventure levels (the intro region)

Per level (the [level template](adventure-content-definitions.md)): purpose, concepts, actions (closed: the exact kit / open:
required and recommended), goals, opponent, board, rewards, difficulty.

- [ ] **A map** (`Levels/<Name>/`) with its World Settings (Level Config, Tile Catalog, Default Match Preset, the match
      Game Mode).
- [ ] **A match preset** and **director Blueprints** (Level, Player, AI) for it; a loadout and AI if there's an opponent.
- [ ] **Its node data asset** and its **node actor on the hub map**.
- [ ] **A PIE run** through it from the hub and back.

The intro region's draft list (placing pieces as a phased level, scoring, double lines, opponents, blocking, swap, shift, a
capstone) is in [adventure-content-definitions](adventure-content-definitions.md); the first level's sequence is in
[adventure-modules-and-progression](adventure-modules-and-progression.md).

## Not yet possible (waiting on code)

- **Pooled cursors owned by actions** -- the action cursors plan.
- **The camera rig** (edge scroll, drag, zoom, auto-focus on a move) -- the top-down camera plan.
- **No Moves / Stuck Player / Stuck Board, a Turns condition, the rotating test opponent, longer shifts** -- the level
  outcomes plan. (Until then a puzzle level can't lose by running out of moves.)
- **Async pooled spawning, pre-warming, pooled tiles** -- the actor pool plan.
- **Teaching prompts and highlights, level screens, player settings and save** -- `_tasks/active.md` plan-later rows.

## Keeping this note honest

Update the status table when a plan lands, and move the matching "not yet possible" line into the right phase.
