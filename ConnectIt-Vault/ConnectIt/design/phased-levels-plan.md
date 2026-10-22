---
type: plan
status: in-progress
approved: true
approved-on: 2026-10-21
completed-on:
playtested: false
design: "[phased-levels](phased-levels.md)"
question: "[phased-levels-runtime-change](../_questions/phased-levels-runtime-change.md)"
decision:
---

# Plan: phased levels (runtime layouts, phase events, condition changes)

Design: [phased-levels](phased-levels.md). Discussion, round by round:
[phased-levels-runtime-change](../_questions/phased-levels-runtime-change.md). Written 2026-10-21 in a design-only
session; **approved by the owner 2026-10-21**. Not built -- building happens in its own session. **Reviewed against the
code the same day** (after [one board change at a time](../_decisions/2026-10-21-one-board-change-at-a-time.md)): the
owner's calls -- one Change Layout operation, condition changes and phases as board changes -- are folded in below.

## Context

Settled in the 2026-10-21 discussion:

- **A phase is an event, not a bundle.** The Level director starts a phase and fires **On Phase Changed**; the GameMode
  hub passes it to every director (Level, Players by slot, AI). What changes is up to the scripts.
- **Layout as data, opt-in.** A level's placed tiles stay its starting layout. A phased level asks for a new layout; the
  board state changes; the visuals reconcile to it. Layouts are built in the editor with a **grid layout tool** in a
  sandbox level and saved as data assets.
- **A layout change = remove tiles, then add tiles, as one board change** (review): a **Change Layout** operation does
  both in one `Apply`, so there is one win check and no in-between board (a shrunken board could otherwise meet a lose
  condition such as No Legal Move). **Remove Tiles** and **Add Tiles** also exist on their own, for player actions or
  tile powers later. Tiles in both layouts keep their piece and multiplier. Pieces on added tiles are ordinary
  arrivals: they set off powers and score through the normal chain.
- **Everything a phase changes is a board change** (review), queued and played one at a time like every level change:
  a phase beginning, a side's conditions changing, a layout changing. Each commits through the Mediator with its own
  event, so clients see it in order and the win check runs after it.
- **Only the Level director changes conditions mid-match** (conditions are rules). Score always carries over; replacing a
  side's goals restarts that side's goal progress. Pitfalls are documented, not prevented.
- A layout can store a **camera position**; the camera moves there when the layout's event plays.
- **Changed while writing this plan (owner's question, Claude agreed; confirmed by the owner):** a side's **starting** win / lose
  conditions also move to the Level director. Today they're on the Player / AI directors' Class Defaults
  ([2026-10-21 hub decision](../_decisions/2026-10-21-director-hub-and-level-changes.md)), which would split one level's
  goals between the side directors (start) and the Level director (changes). With both on the Level director, a level's
  whole script is in one place, and side directors become loadout + AI setup, reusable across levels. Classic / versus
  presets get a generic Level director carrying their standard goals.

Each stage builds and passes the tests before the next; the editor must be closed for builds; the owner commits between
stages.

## Stage 1: conditions belong to the Level director

- `UConnectIt_LevelDirector`: **`PlayerWinConditions`, `PlayerLoseConditions`, `AIWinConditions`, `AILoseConditions`**
  (Class Defaults), replacing `WinConditions` / `LoseConditions` on `UConnectIt_PlayerDirector` and
  `UConnectIt_AIDirector`. `ResolveConditionGroups` reads them from the Level director's class at match setup.
- **`SetWinConditions(Side, Conditions, Timing)` / `SetLoseConditions(Side, Conditions, Timing)`** on the Level director,
  through the GameMode's level-change queue (Immediately = once the current board change has played out --
  [one board change at a time](../_decisions/2026-10-21-one-board-change-at-a-time.md); Next Turn Start).
  - A new kind of `FConnectItLevelChange`: {Side, win or lose, condition assets}.
  - **Applied as a board change (review):**
    1. `ModifyRules` re-resolves that side's groups (the rules replicate: `OnMatchRulesChanged`).
    2. The Mediator commits a level operation, **`FConnectItBoardOperation_GoalsChanged`** {Faction, bWinGoals}. Its
       `Apply` resets that side's `GroupsCompleted` to 0 for new win goals and appends a **`GoalsChanged`** event
       {Faction, win / lose, group names}.
    3. The normal `StampWinState` follows: win progress, target score and, if the new goals are already met, the win
       with its Game Won event.
  - Never written to the board directly: only `SetBoardState` writes it.
  - `GoalsChanged` is level-only: no action produces it.
- Validation moves with the lists; a preset whose Level director has no goals for a side warns as today.
- Migration by checklist (owner): move each preset's director goals into its Level director Blueprint; generic Level
  directors for classic and versus. The test directors in `Private/Tests/ConnectIt_MinMaxTestTerms.h` move their goals
  the same way.
- Tests:
  - goals resolved from the Level director match today's;
  - changing a side's goals mid-match resets its progress, leaves score alone, and appends `GoalsChanged`;
  - goals already met when set win at once;
  - the AI's next decision uses the new goals.
- *As built (2026-10-21; 48/48 tests):*
  - **Names:** the operation is **`FConnectItBoardOperation_ChangeGoals`** {Factions, bWinGoals, GroupNames}
    (request type `ConnectIt.Game.State.ChangeGoals`). The event is **`FConnectItBoardEvent_GoalsChanged`** (tag
    `ConnectIt.Event.GoalsChanged`, getter `Get Active Goals Changed Event`).
  - **Rules and GameMode:** the rule set gained `SetConditionGroups` / `GetConditionGroups`. The GameMode gained
    `SetConditions(Side, bWinGoals, Conditions, Timing)`, applied by `ApplyGoalsChange`.
  - **Which factions:** the player side is every non-AI faction (both, in a match with no AI). New AI goals in a match
    with no AI are ignored, with a warning.
  - **Migration aid (not in the plan), now removed:** the Player / AI directors briefly kept their old `WinConditions` /
    `LoseConditions` as "(unused)", so goals already set in director Blueprints still loaded while the owner copied
    them to the Level directors. Deleted 2026-10-22 at the owner's request, after the migration.
  - **Preset validation:** a preset with **no Level Director Class is an error** (nobody would have goals). A Level
    director with no Player Win Conditions is an error; no AI Win Conditions is a warning.
  - **Not unit-tested:** "the AI's next decision uses the new goals" holds by construction (each decision copies the
    GameMode's rules); it's left to PIE. The GameMode's apply path also needs a world, so it's covered in PIE too.
  - **Tests:** `ConnectIt.AI.DirectorConditions` (rewritten for the Level director) and the new
    `ConnectIt.Board.ChangeGoals`. The operation catalog count is now 9.

## Stage 2: the board layout asset

- **`UConnectIt_BoardLayout`** (data asset, `Public/Framework/Data/`): `Cells` -- an **array of structs**, each
  {`Position`, `TileClass` (optional; else the level's default), `FConnectItTileInitialData` shown inline (multiplier,
  active, shiftable, starting piece; later a tile power), optional `VisualVariant` tag}; optional `CameraTransform`.
  Every gameplay value of every cell is visible and editable in the asset; rows are labelled by position in the Details
  panel (`TitleProperty`). The cell stays a plain struct (the board copies it into its own data), not an inline UObject:
  a tile actor can't be instanced inside an asset, and per-cell look goes through the variant tag.
  Validation: duplicate positions, starting pieces for factions the level doesn't have.
- **`DefaultTileClass`** on the level config (`UConnectIt_LevelConfigDataAsset`; it has `PieceActorClass` but no tile
  class today): the class for a cell that names none. Validation: a layout cell with no class on a board with no default.
- Helper: **diff** two layouts / a board and a layout → positions to remove, cells to add (shared positions untouched).
- Tests: diff cases (grow, shrink, move, identical).
- *As built (2026-10-22; 49/49 tests):*
  - **The cell and the diff** are plain structs in `Public/Board/ConnectIt_BoardLayoutCell.h`, not in the asset's
    header, so Stage 3's operations can carry them without depending on `Framework/Data`:
    - `FConnectItBoardLayoutCell` {Position, TileClass (`TSubclassOf<AGridTileBase>`), TileData, VisualVariant};
    - `FConnectItBoardLayoutDiff` {Removed, Added}, with `Between(CurrentPositions, Cells)`.
  - **The asset** is `UConnectIt_BoardLayout` (`Public/Framework/Data/ConnectIt_BoardLayout.h`): `Cells`, `bHasCamera`
    + `CameraTransform`, `FindCell`, `DiffFrom(Board)`, `DiffFrom(OtherLayout)`.
  - **Validation:**
    - No cells is a warning; a repeated position is an error (the diff keeps the first).
    - A starting piece for faction 2 or higher is a warning: every match has 2 factions today, and a layout isn't tied
      to a level.
  - **`DefaultTileClass`** has no validation of its own. A layout isn't tied to a level config, so "a cell with no class
    on a board with no default" can only be checked when a layout is applied: a runtime warning in Stage 5.
  - **Tests:** `ConnectIt.Board.Layout` (grow, shrink, move, identical, from a live board, cell data carried, `FindCell`,
    validation).

## Stage 3: Remove Tiles, Add Tiles and Change Layout operations

- **`FConnectItBoardOperation_RemoveTiles`** {Positions}: removes those tiles from the board state (and any piece on
  them). Event **`TilesRemoved`** {Positions, the faction of each removed piece}. A removed piece is a piece leaving
  (Tile Exit, once tile powers exist). Refused if a position isn't a tile.
- **`FConnectItBoardOperation_AddTiles`** {Cells}: adds tiles with their starting data. Event **`TilesAdded`** {Cells}.
  Starting pieces are arrivals -- reported for scoring (and Enter powers later). Refused if a position is already a tile.
- **`FConnectItBoardOperation_ChangeLayout`** {RemovePositions, AddCells, optional CameraTransform, the layout asset}
  (review): one operation, one board change.
  - Its `Apply` removes, then adds (the same code as the two above), appending `TilesRemoved`, `TilesAdded`, then
    **`LayoutChanged`** {layout, camera transform} for the camera and any "new layout" visual.
  - Arrivals are reported once, so scoring and the win check run on the final board only.
  - `CanApply` = every removal is a tile and every addition isn't, after the removals.
- All three work with today's touched-positions resolve step and with the tile-powers change record, whichever exists
  when built. Their events carry enough to apply themselves to a board (the display-board rule from the tile-powers
  plan). Level-only for now: no action produces them.
- **Audit what assumes a fixed tile set** during a match:
  - The AI search geometry is rebuilt per decision. Since one board change at a time, the AI only decides from a
    settled board, so a layout can't change mid-search.
  - Also check: the influence map, hover subsystems, `StartingPieces` counts used by piece-count conditions, shift
    operations (clients compute a shift's positions from their tile registry; the server's `CanApply` checks them).
- Tests: add / remove on a board; Change Layout on a board (shared tiles keep piece and multiplier; one win check, on
  the final board -- a lose condition met only by the in-between board doesn't fire); added pieces completing a line
  score; removing an occupied tile; refusals.
- *As built (2026-10-22; 50/50 tests):*
  - **The operations:** `FConnectItBoardOperation_RemoveTiles` {Positions}, `_AddTiles` {Cells}, and `_ChangeLayout`
    {RemovePositions, AddCells, LayoutName, bHasCamera, CameraTransform; `SetDiff(Diff)`}. Their request types are
    `ConnectIt.Game.State.RemoveTiles / AddTiles / ChangeLayout`.
    - Change Layout carries the layout's **name**, not the asset: operations stay free of object references, apart
      from the cells' tile class.
    - An empty Change Layout is allowed (a layout that only moves the camera).
    - A position may be removed and added in the same change.
  - **The events:** `FConnectItBoardEvent_TilesRemoved` {Positions, PieceFactions}, `_TilesAdded` {Cells} and
    `_LayoutChanged` {LayoutName, bHasCamera, CameraTransform}, each with a `Get Active ... Event` getter.
    - Change Layout appends Tiles Removed / Tiles Added only when non-empty, and always appends Layout Changed.
  - **Board state:** `FConnectItBoardState::RemoveTileData` (the other tiles keep their order).
    `FConnectItTileInitialData::ToTileData(NumFactions)` / `HasStartingPiece` are now the one conversion, used by board
    setup too.
  - **Starting pieces on added tiles:** they arrive (scored, and they stay, as placed pieces do) and **count as their
    faction's starting pieces** (`StartingPieces`), the baseline piece-count goals measure progress from. A piece
    removed with its tile doesn't lower it.
  - **Audit:**
    - **AI search geometry:** rebuilt each decision from the board's positions, and the AI only decides from a settled
      board. Fine.
    - **Influence map:** iterates `TilePositions` on every board change. Fine.
    - **Scoring, conditions, Level triggers:** read the board by position. Fine.
    - **Hover subsystems and tile actors:** stale until Stage 5 reconciles them (a removed tile's actor stays and can
      still be hovered).
    - **Shift:** the client computes a line from its tile registry (actors), so it is stale until Stage 5. The server's
      `CanApply` refuses positions that aren't tiles.
  - **Tests:** `ConnectIt.Board.TileOperations` (remove / add / change layout, refusals, an added piece scoring, no loss
    from the in-between board, event order). The operation catalog count is now 12.

## Stage 4: Set Layout and phase events on the Level director

- **`SetLayout(Layout, Timing)`** (Level director): queues one **Change Layout** level operation (acting for nobody,
  -1). The diff is taken when the change is **applied**, not when it's asked for, so a layout queued behind other changes
  diffs against the board it will actually change. The camera transform rides in the operation.
- **Phases are gameplay tags** (`ConnectIt.Phase.*` in `DefaultGameplayTags.ini`), picked from a list, so a typo can't
  make two directors disagree and renames go through tag redirects.
- **`BeginPhase(PhaseTag, Timing)` on the Level director only**, as a queued board change (review):
  - It queues the level operation **`FConnectItBoardOperation_BeginPhase`** {PhaseTag}. Its `Apply` writes
    **`CurrentPhase`** onto `FConnectItBoardState` (so it replicates with the board) and appends a **`PhaseChanged`**
    event {Previous, New} (for a phase banner, played in order with everything else).
  - **`OnPhaseChanged(PhaseTag)`** on the director base: the GameMode hub sends it to every director in the usual order
    when it sees `PhaseChanged` in a committed change (as it does for checkpoints).
  - Their reactions (new goals, a layout) queue behind it.
- A phase need not set a layout. **`GetCurrentPhase`** reads the board, so it works on clients and in UI (a Blueprint
  getter on the board state library / GameState).
- **Order at a checkpoint** (recipe guidance, Stage 7): Begin Phase, then Set Win / Lose Conditions, then Set Layout. The
  queue plays them in that order, so the new layout is checked against the new goals.
- Tests: Set Layout queues one Change Layout and produces the expected board; a layout queued behind another change
  diffs against the later board; Begin Phase writes the phase and appends `PhaseChanged`; On Phase Changed reaches
  directors in order, after the commit.

## Stage 5: visuals follow the board's tiles

- **Tile registry at runtime** (plugin `UnrealGridMechanics`, `UGridTileRegistryBase`): it already registers tiles
  spawned later (`HandleActorSpawned`); **removal** is what's new (unregister, hover subsystem included).
- **Reconcile on `TilesRemoved` / `TilesAdded`** (each machine, from the event):
  - Spawn (or take from the actor pool) the tile class -- the cell's `TileClass`, else the level config's
    `DefaultTileClass` -- at the grid position (`UGridDefinition`), and remove the departed ones.
  - Tile actors aren't replicated, so each machine spawns its own, which is correct.
  - **Pieces are replicated actors:** a piece removed with its tile, or arriving on an added one, goes through the
    existing piece add / remove path (the server's), not a local spawn.
  - C++ where it's registry bookkeeping, Blueprint for how it looks.
- **Camera**: listens for `LayoutChanged` and lerps to its camera transform (Blueprint).
- **Owner, editor:** tile appear / disappear visuals; camera listener.

## Stage 6: the grid layout tool

- A sandbox level + an editor utility: lay tiles out, set each tile's data (the same `IConnectIt_TileDataProvider`
  values the board reads), **Save Layout** to a `UConnectIt_BoardLayout`, **Load Layout** to edit one. Camera: save the
  current view as the layout's camera.
- **Who builds what** (review):
  - **Claude:** editor-only C++ helpers, BlueprintCallable, in `WITH_EDITOR` blocks (the project has one runtime
    module):
    - capture a world's placed tiles into a layout's cells;
    - spawn a layout's cells into a world;
    - write a camera transform to a layout.
  - **Owner:** the editor utility widget around them, and saving the asset (`Editor Asset Library`).
- Optional: warn when a layout holds complete lines under a chosen scoring rule.

## Stage 7: vault

Decision notes (phases as events and layouts as data, each phase change a board change; conditions owned by the Level
director -- partly supersedes the 2026-10-21 hub decision); recipe "script a phased level" from the pitfalls list, with
the checkpoint order (Begin Phase, conditions, layout); update `script-a-level` and the condition recipes (conditions now
on the Level director); `code/systems/place-piece-request.md` lists the level-only operations; close the question; log.

## Not in this plan

- Player actions that add / remove tiles, and their range rules.
- A runtime warning for score goals already met when set.
- Saving progress mid-level (a loss restarts the level, as today).

## Verification

- Each stage: build (editor closed); `Automation RunTests ConnectIt` all pass.
- Owner, PIE: a two-phase level -- phase 1 on a 4x1 layout; its checkpoint calls Begin Phase + Set Layout to a 4x4;
  tiles appear / disappear, the camera reframes, pieces on shared tiles stay, new goals apply; a loss in phase 2 ends the
  match; a client sees the same.

## Confirmed before approval

1. ~~Starting conditions move to the Level director~~ -- **decided (owner): yes** (Stage 1).
2. ~~Phase events by name~~ -- **decided (owner): phases are shared generic gameplay tags**, reused by every level:
   `ConnectIt.Phase.1`, `ConnectIt.Phase.2`, ... and `ConnectIt.Phase.Final`. Stage 4 adds them to
   `DefaultGameplayTags.ini` (a handful of numbered ones; more added when a level needs them).
3. ~~A layout cell may name its own tile class~~ -- **decided (owner): yes**; cells are an array of structs with the
   tile class and the tile data inline, all visible and editable in the asset.

Settled in the review (owner, 2026-10-21):

4. A layout change is **one Change Layout operation** (remove, then add, one board change, one win check); Remove Tiles /
   Add Tiles stay as separate operations.
5. **Changing a side's conditions is a board change:** a `GoalsChanged` operation and event, and the normal win check.
6. **Begin Phase is a queued board change:** the phase is written to the board (replicated) with a `PhaseChanged` event;
   directors hear On Phase Changed from the committed change.
