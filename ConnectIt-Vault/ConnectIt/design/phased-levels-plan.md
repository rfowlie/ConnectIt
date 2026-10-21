# Plan: phased levels (runtime layouts, phase events, condition changes)

Design: [phased-levels](phased-levels.md). Discussion, round by round:
[phased-levels-runtime-change](../_questions/phased-levels-runtime-change.md). Written 2026-10-21 in a design-only
session; **approved by the owner 2026-10-21**. Not built -- building happens in its own session.

## Context

Settled in the 2026-10-21 discussion:

- **A phase is an event, not a bundle.** The Level director starts a phase and fires **On Phase Changed**; the GameMode
  hub passes it to every director (Level, Players by slot, AI). What changes is up to the scripts.
- **Layout as data, opt-in.** A level's placed tiles stay its starting layout. A phased level asks for a new layout; the
  board state changes; the visuals reconcile to it. Layouts are built in the editor with a **grid layout tool** in a
  sandbox level and saved as data assets.
- **A layout change = Remove Tiles, then Add Tiles** -- two ordinary board operations (also usable later by player
  actions or tile powers). Tiles in both layouts keep their piece and multiplier. Pieces on added tiles are ordinary
  arrivals: they set off powers and score through the normal chain.
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
- **`SetWinConditions(Side, Conditions, Timing)` / `SetLoseConditions(Side, Conditions, Timing)`** on the Level director:
  through the GameMode's existing level-change queue (Immediately = once the current board change has played out --
  [one board change at a time](../_decisions/2026-10-21-one-board-change-at-a-time.md); Next Turn Start), re-resolve
  that side's groups in the match rules, reset that side's `GroupsCompleted` to 0 when its win goals change, replicate
  the rules (`OnMatchRulesChanged`), restamp win progress / target score on the board.
- Validation moves with the lists; a preset whose Level director has no goals for a side warns as today.
- Migration by checklist (owner): move each preset's director goals into its Level director Blueprint; generic Level
  directors for classic and versus.
- Tests: goals resolved from the Level director match today's; changing a side's goals mid-match resets its progress
  and leaves score alone; the AI's next decision uses the new goals.

## Stage 2: the board layout asset

- **`UConnectIt_BoardLayout`** (data asset, `Public/Framework/Data/`): `Cells` -- an **array of structs**, each
  {`Position`, `TileClass` (optional; else the level's default), `FConnectItTileInitialData` shown inline (multiplier,
  active, shiftable, starting piece; later a tile power), optional `VisualVariant` tag}; optional `CameraTransform`.
  Every gameplay value of every cell is visible and editable in the asset; rows are labelled by position in the Details
  panel (`TitleProperty`). The cell stays a plain struct (the board copies it into its own data), not an inline UObject:
  a tile actor can't be instanced inside an asset, and per-cell look goes through the variant tag.
  Validation: duplicate positions, starting pieces for factions the level doesn't have.
- Helper: **diff** two layouts / a board and a layout → positions to remove, cells to add (shared positions untouched).
- Tests: diff cases (grow, shrink, move, identical).

## Stage 3: Remove Tiles and Add Tiles operations

- **`FConnectItBoardOperation_RemoveTiles`** {Positions}: removes those tiles from the board state (and any piece on
  them). Event **`TilesRemoved`** {Positions, the faction of each removed piece}. A removed piece is a piece leaving
  (Tile Exit, once tile powers exist). Refused if a position isn't a tile.
- **`FConnectItBoardOperation_AddTiles`** {Cells}: adds tiles with their starting data. Event **`TilesAdded`** {Cells}.
  Starting pieces are arrivals -- reported for scoring (and Enter powers later). Refused if a position is already a tile.
- Both work with today's touched-positions resolve step and with the tile-powers change record, whichever exists when
  built. Both events carry enough to apply themselves to a board (the display-board rule from the tile-powers plan).
- **Audit what assumes a fixed tile set** during a match: the AI search geometry (rebuilt per decision -- likely fine),
  the influence map, hover subsystems, `StartingPieces` counts used by piece-count conditions, shift operations.
- Tests: add / remove on a board; added pieces completing a line score; removing an occupied tile; refusals.

## Stage 4: Set Layout and phase events on the Level director

- **`SetLayout(Layout, Timing)`**: diffs the current board with Layout and queues Remove Tiles then Add Tiles as level
  changes (acting for nobody, -1), plus the camera target if the layout has one.
- **`BeginPhase(PhaseTag)`** and **`OnPhaseChanged(PhaseTag)`** on the director base: phases are **gameplay tags**
  (`ConnectIt.Phase.*` in `DefaultGameplayTags.ini`), picked from a list, so a typo can't make two directors disagree
  and renames go through tag redirects. The Level director records the current phase and the GameMode hub sends On Phase
  Changed to every director in the usual order. A phase need not set a layout. `GetCurrentPhase` for UI.
- Tests: Set Layout produces the expected two operations and board; phase event order across directors.

## Stage 5: visuals follow the board's tiles

- **Tile registry at runtime** (plugin `UnrealGridMechanics`, `UGridTileRegistryBase`): add / remove a tile actor at a
  position.
- **Reconcile on `TilesRemoved` / `TilesAdded`**: spawn (or take from the actor pool) the tile class -- the cell's
  `TileClass`, else the level config's default -- at the grid position (`UGridDefinition`), and remove the departed
  ones; piece visuals for pieces on added tiles, and removed with their tiles. In C++ where it's registry bookkeeping,
  Blueprint for how it looks.
- **Camera**: listens for the layout event and lerps to the layout's camera transform (Blueprint).
- **Owner, editor:** tile appear / disappear visuals; camera listener.

## Stage 6: the grid layout tool

- A sandbox level + an editor utility (Blueprint editor utility widget, or a small editor-only actor): lay tiles out,
  set each tile's data (the same `IConnectIt_TileDataProvider` values the board reads), **Save Layout** to a
  `UConnectIt_BoardLayout`, **Load Layout** to edit one. Camera: save the current view as the layout's camera.
- Optional: warn when a layout holds complete lines under a chosen scoring rule.

## Stage 7: vault

Decision notes (phases as events and layouts as data; conditions owned by the Level director -- partly supersedes the
2026-10-21 hub decision); recipe "script a phased level" from the pitfalls list; update `script-a-level` and the
condition recipes (conditions now on the Level director); close the question; log.

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
