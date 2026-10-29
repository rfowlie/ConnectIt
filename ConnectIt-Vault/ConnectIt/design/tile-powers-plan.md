---
type: plan
status: complete
approved: true
approved_on: 2026-10-22
completed_on: 2026-10-22
started:             2026-10-22          # YYYY-MM-DD, when the first stage was built (blank until then)
stages: 6              # how many stages the plan has
current_stage: complete   # the first stage not yet built ("complete" / "not started")
playtested: false
design: "[tile-power-triggers](tile-power-triggers.md)"
question: "[tile-powers-in-the-board-pipeline](../_questions/tile-powers-in-the-board-pipeline.md)"
decision: "[tile powers in the after-move chain](../_decisions/2026-10-22-tile-powers-in-the-after-move-chain.md); [tile catalog and builder tiles](../_decisions/2026-10-22-tile-catalog-and-builder-tiles.md)"
---

# Plan: tile powers (chain reactions in the after-move step)

Design: [tile-power-triggers](tile-power-triggers.md). Discussion and every decision, round by round:
[tile-powers-in-the-board-pipeline](../_questions/tile-powers-in-the-board-pipeline.md). Power sketches:
[tile-powers](tile-powers.md). Written 2026-10-21; reviewed against the code the same day (round 10 in the question:
Shift, per-arrival survival, the search's turn number, the display board's limits, the scoring-rule interface, the
gate, the guard, Crumble); walked through with the owner and **approved 2026-10-22**. Stage 3 **revised 2026-10-22** at the owner's direction
after its first build (the tile catalog and builder tiles). **Complete 2026-10-22:** Stages 1-4 and 6 built; Stage 5 (the
first real powers) moved to its own plan, to follow the tile lock plan.

## Context

A tile power reacts to a board change and makes another board change; chains of them resolve inside one action and are
played back one event at a time. Settled in the 2026-10-21 discussion:

- Powers run in the rules' after-move step, `FConnectItRuleSet::ResolveBoardChange`, so every path gets them: the
  Mediator (player actions and Level director operations), `Simulate Operation`, and the AI's search.
- **Two phases.** *Settle* (pre scoring): Piece Enter / Piece Exit / Piece Changed / Tile Changed powers fire, in that order and tile order, until
  nothing more fires. *Score*: find every line made by pieces that arrived since the last pass, apply them all at once.
  If anything scored, the Scoring powers on those lines' tiles fire, and what scoring and they changed goes back through
  settle. **The chain ends at the first scoring pass with no lines.** Win checked once, after the chain. No post-scoring
  reactions (a slot is reserved at the end). Director / checkpoint / win reactions are new board requests afterwards
  (the existing Level director path).
- **One power per tile, one trigger per power**, plus an optional gate (a minimum multiplier) and a cooldown in turns
  (all turns; 0 = every time). Level-wide rules (Crumble) are powers that apply to every tile, on top of a tile's own.
- Each power on a tile fires at most once per action -- that alone ends every chain; a cap stays only as a bug guard.
- A capture is a **Piece Changed** (a pre-scoring trigger of its own, owner 2026-10-22), not an Exit + Enter. A piece cleared
  by scoring sets off Scoring powers, never Exit.
  **A shift is neither** (review): Shift moves whole tile data, so a tile's power, cooldown and piece travel together
  and no piece changes tile -- shifted positions are only scored.
- **A power's effect is a board operation**, the same kind a player action sends, so powers and actions can share
  them. A power acts for the faction whose change set it off.
- Random choices come from a seed on the board. A power with no effect still shows and starts its cooldown.
- **Visuals read a display board** that starts at the board before the action and is updated by each event as it plays.
  Every event carries everything needed to apply it; a test checks before + events = after. Events record their cause.

Each stage builds and passes the tests before the next; the editor must be closed for builds; the owner commits between
stages; nothing is committed by me.

## Stage 1: the change record and the scoring pass (no powers yet)

- **`FConnectItBoardChanges`** replaces `FConnectItTouchedPositions` (`ConnectIt_BoardOperation.h`): what a step
  changed, inline-allocated -- `Entered` and `Exited` (position + the piece's faction; `Entered` also
  `bSurvivesScoring`, from the operation that made it), `Moved` (positions a shift gave a new occupant: scored, but no
  Enter / Exit), `PieceChanged` (position + new and previous owner; scored like an arrival), `TileChanged` (position +
  what: multiplier / active / power), plus the faction of the change.
  `FConnectItBoardOperation::Apply` fills it; each operation reports what it really does:
  - Place / Force Place: enter. Swap: exit + enter on both tiles. Shift: moved. Capture: piece changed. Remove: exit.
    Toggle / Destroy Multiplier: tile changed.
  - **Tiles appearing or disappearing set off nothing** (owner, 2026-10-22): **Remove Tiles** destroys the pieces on
    them with no Piece Exit; **Add Tiles** brings its starting pieces in with no Piece Enter -- but they **are scored**,
    so an added piece that completes a line counts. They go in the record like `Moved` (scored, no triggers).
    **Change Layout** does both.
  - Change Goals, Begin Phase: nothing.
  - (When built) Set Tile Locked: tile changed.
- **Survival is per arrival:** one scoring pass can mix the action's arrivals with pieces moved by powers, so each
  arrival carries its own `bSurvivesScoring` (replaces the single `bArrivingPiecesSurvive` argument).
- **Scoring rule: one pass call** (`ConnectIt_ScoringRule.h`, `ConnectIt_LineScoringRule.*`): the base
  `FConnectItScoringRule` gets `ScorePass(Board, Arrivals, OutRecord, OutEvents)` → points, replacing per-position
  `ApplyScoring`. Lines implements it as find-all-then-apply: every line from every arrival is found on the same board,
  then all are applied. The base stays line-agnostic (the rule is swappable through `SetScoringRule`); the record
  reports `ScoredThrough` positions (what Scoring powers fire on) and the multipliers scoring changed.
- **`ResolveBoardChange`** takes the change record and becomes the loop shape (settle → score → scoring reactions →
  back), with empty settle and reaction steps. Callers: `CarryOutOperation`, `FConnectItMinMaxRules::ApplyMove`.
- Behaviour change: two arrivals in one step (a shift) can no longer break each other's lines.
- Tests: all existing pass; new -- a shift making two lines that share a tile scores both; each operation's change record
  (Shift: `Moved` only; Add Tiles: scored, no enter; Remove Tiles: nothing); a swap-in and a placed piece in one pass keep
  their own survival; an added tile whose piece completes a line scores.
- *As built (2026-10-22; 53/53 tests; Throughput 374,539 nodes/s against 366k-379k earlier today -- no measurable
  cost):*
  - **`FConnectItBoardChanges`** is in its own header, `Public/Board/ConnectIt_BoardChanges.h` (plain structs, inline
    storage), so the scoring rule can use it without depending on the operations header.
    - **Lists:** `Entered` / `Moved` (`FConnectItPieceArrival`: position, faction, `bSurvivesScoring`), `Exited`,
      `PieceChanged` (new + previous owner, survival), `TileChanged` (position, what, **faction** -- whose change it
      was), `ScoredThrough`, `Faction`.
    - **Helpers:** `Add*`, `GetScoringArrivals` (entered, piece changed, moved -- in that order), `HasScoringArrivals`,
      `IsEmpty`.
  - **What each operation reports:**
    - Swap reports both exits and both enters.
    - Remove reports its exit with the owner.
    - Toggle / Destroy Multiplier: tile changed, with the operation's faction.
    - Add Tiles / Change Layout: their starting pieces as moved.
    - Each operation stamps its own `ArrivingPiecesSurviveScoring()` on what it reports.
    - `CarryOutOperation` and the AI's `ApplyMove` stamp the operation's faction on the record.
  - **`FConnectItScoringRule::ScorePass(Board, Arrivals, OutChanges, OutEvents)`** replaces `ApplyScoring`. **Lines**,
    when several arrivals are involved (the owner had approved the plan; this filled a gap it didn't cover):
    - every line from every arrival is found first;
    - a line two arrivals share counts once;
    - each line keeps at most one piece, the first arrival on it that survives scoring (as today's per-position loop
      did);
    - a kept piece is never cleared by another line;
    - lines are then applied in the order found, so a tile in two lines gains +1 for each and the second sums the
      raised multiplier (as crossing lines already did).
    - The pass reports `ScoredThrough` and a Multiplier tile change (scoring faction) per tile.
  - **`ResolveBoardChange(Board, Changes, OutEvents)`** is the settle → score → reactions loop, with settle and
    reactions empty; it ends at the first pass with no lines (64-pass guard, logged). The rule set's `ApplyScoring`
    convenience lost its faction parameter (the faction comes from the tile).
  - **Tests:** `ConnectIt.Board.ScoringPass` (two arrivals' lines sharing a tile both score; two arrivals on one line
    count once; mixed survival in one pass; what a pass reports) and `ConnectIt.Board.ChangeRecord`. The operation
    tests now check the record lists; the added-tile-scores case was already in `TileOperations`.

## Stage 2: self-applying events and the display board

- **Events carry their full change** (`ConnectIt_BoardEvents.h`): `Scored` gains each tile's new multiplier;
  `TileMultiplierDestroyed` its previous value; the phased-levels events (`TilesRemoved`, `TilesAdded`,
  `LayoutChanged`, `GoalsChanged`, `PhaseChanged`) checked and completed so each can apply itself too; base `FConnectItBoardEvent` gains `CauseIndex` (index of the event that
  set it off in the same list, -1 = the action).
- **`virtual void ApplyTo(FConnectItBoardState&) const`** on every event: what it changed, from its own data.
- **Display board** on `UConnectIt_BoardStateComponent` (every machine): starts as the seeded board; each event is
  applied, from the event's own payload, when its queue entry **completes** (so a handler sees the board before its
  event, plus the event); `OnDisplayBoardChanged` delegate; `GetDisplayBoard()` for Blueprint. It never resets in the
  middle of a list -- a newer snapshot arriving while a list plays just queues its events behind.
  (Uses the plugin's existing any-tag-complete delegate; no plugin change expected.)
- **What events cover:** tiles and scores. The match-level fields no event carries (win progress, target score, game
  over, winner / loser, turn, seed) are copied from Current when the queue drains.
- **Resync:** when the queue drains and the display board's tiles or scores differ from Current, it snaps to Current and
  logs a warning. A guard: since one board change at a time, snapshots only collapse where the lock can't wait for
  visuals (a dedicated server).
- Server logic (directors, Mediator, influence map) keeps reading the real board.
- **Tile actors follow the board** (owner, 2026-10-22): a tile's identity (class, power) travels with its data, so the
  visuals move tile actors to the positions the board says -- on Board Shifted the tile registry re-keys tile actors the
  way the piece registry re-keys pieces -- and swap actors when tiles are replaced (Tiles Removed / Added; the added
  tile's class comes from the tile catalog, Stage 3).
- Tests: for each operation and a scoring move, Previous + every event's `ApplyTo` == Current, comparing tiles and scores.
- **Owner, editor:** move visual Blueprints that read the current board (tiles, score UI, `CI_PieceVisualHandler`) to
  the display board.
- *As built (2026-10-22; 54/54 tests; Throughput 368,438 nodes/s, inside 366k-379k):*
  - **Events:**
    - Every event has `ApplyTo` (`ConnectIt_BoardEvents.cpp`), and `FConnectItBoardEvent::ApplyAll(Events, Board)`
      applies a whole list.
    - Board Seeded and Layout Changed change nothing (the layout's tiles come in Tiles Removed / Tiles Added).
    - Each other event mirrors its operation exactly. Shift reads every moving tile before writing any. Tiles Added
      skips a position that is already a tile and counts starting pieces. Goals Changed restarts win progress.
  - **New fields:** `Scored::NewMultipliers` (parallel to `Positions`, each tile's value after that line -- a tile in
    two lines shows the higher value in the second) and `TileMultiplierDestroyed::PreviousMultiplier`, both filled.
  - **`FConnectItBoardState::HasSameTilesAndScores`:** compares tiles by position, in any order (piece, multiplier,
    active, occupied, shiftable), and scores.
  - **What the events don't carry:** win progress for the **last** goal group, game over and the winner. Completing the
    last group is the win, not a checkpoint, and Game Won comes from the Mediator. So these are match-level, copied
    from Current when the queue drains, as planned.
  - **Display board** (`UConnectIt_BoardStateComponent`): `GetDisplayBoard()`, `OnDisplayBoardChanged` (no
    parameters).
    - **Start:** the server starts it in `InitialiseBoardState`; a client starts it from its first snapshot's Previous.
    - **Stepping:** it binds `OnAnyTagComplete` just before queuing (so the seed is never missed) and unbinds in
      `EndPlay`. Each completing entry whose payload is a board event is applied.
    - **Counting:** queued events are counted **before** any is queued. With no visuals bound, an entry completes inside
      its own queue call, and the board mustn't catch up to Current partway through a list.
    - **Catch-up:** at zero pending it copies Current, warning if the tiles or scores differ. An empty list catches up
      at once.
  - **Tile actors on a shift:** `UConnectIt_TileRegistry::ShiftTiles(StartPositions, EndPositions)` re-keys the tile
    actors (read all, then write) and returns them in order, ready to animate. Replaced tiles already go through
    `Spawn Layout Tiles` / `Unregister Tiles At`.
  - **Tests:** new `ConnectIt.Board.Events.Replay` checks before + events = after for:
    - Place scoring two crossing lines (also checks `NewMultipliers`);
    - Swap and Capture, each completing a line;
    - Shift, by one and two steps, past a fixed tile;
    - Force Place, Capture, Remove, Toggle and Destroy (also checks `PreviousMultiplier`);
    - Remove Tiles, Add Tiles (scoring), Change Layout, Begin Phase;
    - Change Goals with a checkpoint;
    - and that the comparison itself catches a difference.
  - **Owner, editor (still to do):**
    - move the visual Blueprints to `GetDisplayBoard()` / `OnDisplayBoardChanged`;
    - call `Shift Tiles` from the Board Shifted handler and animate the actors it returns.

## Stage 3: power data, the tile catalog, builder tiles and turn stamping

*Revised 2026-10-22 (owner, second time -- see [tile-power-setup](tile-power-setup.md), "Revised: the tile catalog"):
**power values are defined once per tag, in a tile catalog**, never on placed tiles. Levels are blocked out with
**builder tiles** that name a tag; the catalog says which tile class shows each tag. Replaces the class-held power
(`TilePowerTag` defaults-only per subclass, values from each Blueprint's exposed variables): that put a copy of the
values on every board tile, and measured ~17% slower AI search even with no powered tiles (below).*

- **`FConnectItTilePower`** (`Public/Board/Powers/ConnectIt_TilePower.h`, plain thread-safe struct): `Name` (gameplay
  tag, `ConnectIt.TilePower.*` -- **a level is part of the tag**, e.g. `ConnectIt.TilePower.Poison.2`), `Trigger`
  (Piece Enter / Piece Exit / Piece Changed / Tile Changed / Scoring / Sync), `MinMultiplier` (the gate; 0 = none),
  `CooldownTurns`, `Effect` (an instanced `FConnectItTilePowerEffect` holding its values: chooses targets from the
  board, the tile, the faction and the seeded random stream, and returns the board operations to carry out).
- **Tile catalog** -- `UConnectIt_TileCatalog` (new data asset):
  - `Tiles`: a **map from power tag to an entry** (one entry per tag, by construction): the **tile class** that shows it
    and the **power** (trigger, gate, cooldown, effect and its values; its `Name` is the key).
  - `DefaultTileClass` (named `DefaultTileClass` at first; renamed by the owner 2026-10-23): the "no power" entry -- the tile class for a tile with no tag. Replaces the level config's
    `DefaultTileClass` (kept as the fallback while levels have no catalog; removed once they do -- owner, editor).
  - **Chosen on the level config** (`TileCatalog`), with a project default in a new ConnectIt project settings
    (`UDeveloperSettings`, "Default Tile Catalog"). Trying another skin or look = a copy of the catalog with other
    classes slotted in, picked on the level config.
  - The tile classes stay in the catalog (UObject references): never in the rules or the board, which the search copies
    on a worker thread.
- **Builder tile** -- `AConnectIt_BuilderTile` (new, C++; derives from `AConnectIt_TileBase`, the C++ base for the
  project's tile actors): the **only tile designers place** in a level.
  - **`TilePowerTag`** -- editable on the placed tile, picked from `ConnectIt.TilePower.*`; empty = no power. Only the
    tag: values are never set per tile.
  - The per-tile settings (starting piece, multiplier, active, shiftable; later lock and chain partner) through
    `IConnectIt_TileDataProvider`, as tiles have them today.
  - A small visual so designers see what is where (owner, editor: a Blueprint subclass showing the tag).
- **Tag in the tile's settings:** `TilePowerTag` joins `FConnectItTileInitialData` (what a placed tile and a layout
  cell give the board), so it reaches the board through the same path as the other settings.
- **Layout cells name a tag, not a class:** the cell's `TileClass` and captured power go (the catalog decides the
  class). The layout tool captures builder tiles (their tags and settings); Load Layout Into Level places builder
  tiles. Owner, editor: capture existing layouts again.
- **Tile data** (`FConnectItTileData`, plain, so board copies stay plain memory copies): `TilePowerTag`,
  `PowerReadyTurn`, and a **stable `TileId`**. The id is generated by the board when a tile joins it (build, Add Tiles;
  a counter on the board, never reused), travels with the tile's data through shifts, and is never authored -- powers
  that refer to another tile (Chained's partner) store its id, which a shift can't break. Tiles Added carries the ids it
  gave, so replaying it gives the same ones.
- **Rules:**
  - `TilePowers`: the catalog's powers, copied from the level's catalog at match start (an array keyed by `Name` --
    the rules replicate and a `TMap` doesn't). Firing (Stage 4) looks a tile's tag up here.
  - `LevelWidePowers`: the **tags** of the level-wide powers (Crumble), in firing order -- listed on the Level director
    (`LevelWidePowers`, Class Defaults), their definitions from the catalog.
  - A tile tag or level-wide tag the catalog doesn't define: warning when the board is built or tiles are added, and
    the tile has no power (its tag is cleared).
- **Board:** `TurnNumber` (replaces the unused `LastModifiedTurn`), `RandomSeed` (advanced as powers roll),
  `NextTileId`.
- **Every copy of the board resolves at the right turn:**
  - the Mediator stamps the GameState's turn before resolving;
  - the AI's decision board is stamped with the decision turn (the controller already holds `DecisionTurnNumber`), so
    the search root and the AI's simulations start there, and `ApplyMove` adds 1 per ply (cooldowns count all turns)
    -- otherwise the root carries the opponent's last turn and no power ever comes off cooldown inside the search;
  - `Simulate Operation` takes the turn to simulate at (-1 = the board's own).
- **Previewing a catalog in the editor** (layout tool library): **Preview Catalog** spawns the catalog's tile class
  over each builder tile as a **transient** actor (never saved) and hides the builder tiles with the editor's
  **temporary hide** (not saved either); **Clear Catalog Preview** undoes both. Nothing the preview does can reach the
  saved level. Owner, editor: buttons on the layout tool widget.
- **At match start the swap is real:** after the board is built from the builder tiles, the tile registry spawns the
  catalog's class for each builder tile (on every machine -- tile actors are local) and retires the builder.
  `Spawn Layout Tiles` uses the catalog too (a cell's tag picks the class).
- **Validation** (asset save / map check):
  - catalog: an entry with no tile class, no effect, or a power `Name` that isn't its key; no plain tile class;
  - builder tiles and layout cells: a tag the catalog doesn't define (levels against their own catalog, layouts against
    the project default);
  - Level director: a level-wide tag the catalog doesn't define, or listed twice;
  - level config: no catalog (warning: falls back to the project default).
- **Tests:** a builder tile's tag reaches the tile data; Add Tiles carries a cell's tag and gives new ids (replayed the
  same); a shift carries the tag, ready turn and id; the catalog's powers and the level-wide tags copy into the rules;
  an undefined tag warns and gives no power; a fresh board has no powers; `ApplyMove` advances the turn; Throughput back
  at the baseline.
- *First build (2026-10-22, the class-held design, before this revision; 55/55 tests):* built `FConnectItTilePower`,
  `AConnectIt_TileBase` with a defaults-only tag and `GetTilePower()`, the power with its values on the tile data (an
  instanced struct), tile ids, the board fields, turn stamping, level-wide powers on the Level director, layout capture
  and validation. **Throughput 303,709 nodes/s against 364,987 with the instanced struct removed** (tag, id and ready
  turn kept) -- the copy of the values on every tile. Reworked to the revision above: the turn stamping, tile ids, board
  fields and the Tiles Added ids stay; the power on the tile data, `GetTilePower`, the defaults-only tag, the layout
  cell's class and captured power, and the Level director's power definitions go.
- *As built (rework, 2026-10-22; 55/55 tests; Throughput 392,028 nodes/s against 366k-379k -- back to normal):*
  - **Catalog:** `UConnectIt_TileCatalog` (`Public/Board/Powers/`) has `Tiles` (tag -> {`TileClass`, `Power`}),
    `DefaultTileClass`, `GetTileClassFor`, `Defines`, `MakePowers` (each power named by its key), and `FindFor(world)`
    (the level config's catalog, else the project default).
  - **Settings:** `UConnectIt_ProjectSettings` (Project Settings > Game > ConnectIt) holds `DefaultTileCatalog` and
    `BuilderTileClass`. The second wasn't in the plan: Load Layout Into Level needs to know which builder Blueprint to
    place.
  - **Tiles:** `AConnectIt_TileBase` is now just the shared C++ base. `AConnectIt_BuilderTile` implements the tile
    data interface with an editable `TileData` (tag included) and `VisualVariant`, and validates its tag against its
    level's catalog.
  - **Data:** `FConnectItTileInitialData::TilePowerTag` reaches the board through `ToTileData`. `FConnectItTileData`
    carries the tag, `TileId` and `PowerReadyTurn`, all plain. `FConnectItTilePower::bLevelWide` is gone (a power is
    level-wide by being listed).
  - **Rules:** `SetTilePowers(Powers, LevelWide)` and `FindTilePower(tag)`, with `TilePowers` and `LevelWidePowers`
    (tags). The Level director has `LevelWidePowers` (tags), validated against the project default catalog.
  - **Undefined tags:** at board build an undefined tag is dropped with a warning
    (`MakeTileData(..., Catalog)`). Add Tiles has no catalog to hand: an undefined tag there just finds no power
    when firing (Stage 4), and layout validation catches it earlier.
  - **The swap:**
    - `UConnectIt_TileRegistry::SwapBuilderTiles()` is called by the board state component: on the server right
      after the board is built from the builder tiles, and on a client when its first snapshot arrives.
    - Each builder becomes its catalog class, given the builder's values and look; the builder is unregistered and
      destroyed.
    - `Spawn Layout Tiles` picks a cell's class from the catalog, falling back to the level config's
      `DefaultTileClass` when there is no catalog.
  - **Layout tool:**
    - Capture skips preview actors and takes tags through the tile data interface.
    - Load places builder tiles.
    - **Preview Tile Catalog** spawns the catalog's actors with `RF_Transient`, hidden from the outliner and tagged
      so they can be found again, and temporarily hides the builders. **Clear Tile Catalog Preview** undoes both, and
      Load clears any preview first.
  - **Validation:**
    - catalog: no tag, no class, no effect, or a power named other than its key; a missing plain class is a
      warning;
    - layouts: tags checked against the project default catalog;
    - level config: a missing catalog is a warning.
  - **Tests:** `ConnectIt.Board.TilePowerData` rewritten:
    - the catalog's classes and powers;
    - rules from the catalog, including the level-wide tags;
    - a builder tile's tag reaching the board, and an undefined one dropped;
    - ids; Add Tiles with a tag (replayed the same); a shift carrying tag, id and ready turn;
    - catalog and director validation; the turn per ply.
  - **Throughput:** 392,028 nodes/s, within noise of the baseline: the per-tile values were the whole cost. A first
    run read 69,649 while Windows had the CPU capped at 30% of its maximum frequency (thermal or power limiting, which
    comes and goes on this machine); the re-run was made with it at 99%.
  - **Moved afterwards (owner, 2026-10-22):** the catalog is named in the map's **World Settings** (`TileCatalog`), not
    the level config, to stay with the level. The project default and the builder tile class stay in ConnectIt project
    settings. 56/56 tests.
  - **Owner, editor:**
    - a builder tile Blueprint with a blockout look (set it as Builder Tile Class);
    - a tile catalog (Default Tile Class plus entries), set in maps' World Settings and as Default Tile Catalog;
    - replace placed tiles with builder tiles and capture layouts again;
    - Preview / Clear buttons on the layout tool widget;
    - clear `DefaultTileClass` on level configs once they have a catalog.

## Stage 4: firing powers in the chain

- In `ResolveBoardChange`: **settle** collects powers set off by the record's Piece Enter / Piece Exit / Piece Changed / Tile Changed
  (the tile's own power -- its `TilePowerTag` looked up in the rules' `TilePowers`, a short list; a lookup cache only if
  Throughput says so -- and the level-wide ones), skips gated-out, cooling-down and already-fired ones, fires them in Piece Enter →
  Piece Exit → Piece Changed → Tile Changed, tile order, and loops on what they changed until nothing fires. **Scoring reactions** fire the Scoring powers on the tiles
  the pass's lines ran through, in tile order.
- **Firing a power:** append **`PowerTriggered`** {Position, power name, Faction, `bNoEffect`, CauseIndex}; set
  `PowerReadyTurn = TurnNumber + CooldownTurns`; ask the effect for operations; for each whose `CanApply` holds, call
  its **`Apply`** (never `CarryOutOperation`, which would start a chain inside the chain) -- its own event, `CauseIndex`
  = the PowerTriggered event; add what it changed to the next step. **When the tile lock is built**
  ([tile-lock-and-chained-tiles-plan](tile-lock-and-chained-tiles-plan.md), after this plan), this check becomes its
  lock-aware `FConnectItRuleSet::CanCarryOut`, so an effect can't act on a locked tile.
- **Faction** (confirmed by the owner): the change that set it off.
  - Scoring: the scoring faction.
  - Piece Enter: the arriving piece's owner.
  - Piece Exit: the leaving piece's owner.
  - Piece Changed: the new owner.
  - Tile Changed: the faction of the change that caused it -- the scoring faction for a multiplier scoring raised, a power's
    faction for its effect, the operation's faction for a Level director change (-1 when it has none).
  - An effect that needs a faction (Capture, placing) finds nothing to do at -1: its operation's `CanApply` fails.
- **Sync powers** (revised 2026-10-22, owner: the chain is a tile power -- see
  [tile-lock-and-chained-tiles-plan](tile-lock-and-chained-tiles-plan.md)): a power with the **Sync** trigger runs on
  **every settle pass**, first in the pass, and keeps some state true (first one: Chained). It checks state, not the
  change record, so it catches what no trigger reports (a partner cleared by scoring). It appends Power Triggered only
  when it changes something, plus its own events; its changes feed the same pass's other triggers. Sync powers must be
  idempotent; they are **exempt from the once-per-action guard** (they only follow state, so they can't loop) and have
  no cooldown. A tile's own power or a level-wide one can be Sync. This stage adds the Sync trigger and its pass; the
  Chained power itself comes with the lock plan.
- Once-per-action guard: a bit per (tile, power slot) -- the slots are the tile's own power plus each level-wide power
  (there can be several). Bug guard: 64 settle loops or scoring passes, log + stop.
- Tests also cover a test-only Sync power (runs every pass, changes only when state calls for it, not limited by the
  once-per-action guard).
- Tests use **test-only powers** (simple effects defined in the test code, since the first real powers are deferred):
  the owner's chain (place → score through a "move a neighbour" tile → the moved piece completes a line → its tile's
  power fires);
  settle until quiet; a capture fires a Piece Changed power and no Piece Enter / Piece Exit power; cooldown 0 / 1 / 2 against the turn number; a power with
  no target still shows and cools down; same seed, same result; before + events = after on a chain.
- *As built (2026-10-22; 56/56 tests; Throughput 393,038 / 389,361 nodes/s against 366k-392k -- no cost):*
  - **`FConnectItPowerChain`** (`Private/Board/Powers/`) lives for one `ResolveBoardChange` call.
    - **Settle:** Sync powers first in each loop; their changes join that loop's triggers. Then Piece Enter → Piece
      Exit → Piece Changed → Tile Changed, in the record's order. Then loop on what changed until nothing fires.
      Everything changed is added to the step, so it is scored.
    - **Scoring reactions:** the Scoring powers on `ScoredThrough`, in order.
    - **No powers:** with no tile powers in the rules, the chain is skipped and nothing extra is copied.
  - **Firing:**
    - The gate, the cooldown (`Board.TurnNumber >= ready turn`) and the once-per-action guard are checked first.
      The guard keys on (tile id, or the position for a board built without ids; slot).
    - It appends **`PowerTriggered`** (`ConnectIt.Event.PowerTriggered`; Blueprint `Get Active Power Triggered
      Event`) and sets the ready turn.
    - The effect rolls from `FRandomStream(Board.RandomSeed)`, and the seed moves on.
    - Each operation is stamped with the power's faction and applied (`Apply`) if `CanApply`. Its events get the
      Power Triggered event as their `CauseIndex`. With none applied: `bNoEffect`, or, for Sync, the event is
      removed.
    - `PowerTriggered::ApplyTo` sets the ready turn, so before + events = after holds on a chain.
  - **Causes:**
    - Every change-record entry carries a `Cause`: the event that brought it, or -1 for the action.
    - `ScoredThrough` entries carry the scoring faction too (`FConnectItScoredTile`), which is the faction Scoring
      powers act for.
    - Scored events record their arrival's cause.
  - **Filled gaps (the plan didn't say):**
    - **A level-wide power cools down across the whole board**, not per tile:
      `FConnectItBoardState::LevelWideReadyTurns`, in the same order as the rules' `LevelWidePowers`. A tile has
      one ready turn, for its own power; per-tile cooldowns for level-wide powers would need more board data. To
      revisit with Crumble.
    - **A Sync power acts for the action's faction** with cause -1, since no single change sets it off.
  - **Tests:** new `ConnectIt.Board.TilePowerChain`, with test-only effects in `ConnectIt_MinMaxTestTerms.h` (place
    at offsets, move a neighbour, place at random, Sync clear):
    - the owner's chain, checking event order and every cause;
    - settle until quiet, where a power placing back on a fired tile doesn't fire it again;
    - capture → Piece Changed only (place → Enter, remove → Exit), each for its faction;
    - cooldown 0 / 1 / 2 against the turn, with no effect still cooling down;
    - the gate; same seed, same result;
    - a Sync power clearing twice in one action, and staying quiet when not called for;
    - no powers → scoring only;
    - before + events = after on the chains.

## Stage 5: new operations and the first powers -- deferred (moved to its own plan)

*Now [first-tile-powers-plan](first-tile-powers-plan.md) (written 2026-10-23 from the question's Stage 5 rounds).*

**Deferred (owner, 2026-10-22): to be designed another time**, in its own plan. Kept here as the starting point:
`_MovePiece` / `_AdjustMultiplier` operations with their events (`_SetTilePower` **dropped**, owner 2026-10-22: a tile
becomes another kind by Remove Tiles + Add Tiles with a different class), and the first powers sketched in the
2026-10-21 rounds (Push, Repel, Vacuum, Crumble). Without it, this plan builds the **system** (record, scoring pass,
events, display board, power data, firing) and proves it with **test-only powers** defined in the tests (Stage 4); no
power ships to levels yet.

## Stage 6: vault

Decision note (tile powers: two phases, operations as effects, display board); close the question with it; `code/`
recipe "add a tile power"; `code/systems/` flow for the chain; status lines in `tile-powers.md`,
`tile-power-triggers.md`, `board-request-objects.md` (reactions built); `three-directors.md` (the Level director lists
the level-wide power tags) and `code/recipes/script-a-level.md` (blocking out with builder tiles, picking the tile
catalog); `code/` recipe "add a tile power" is a catalog entry plus its tile class; mark
[the Crumble-threshold decision](../_decisions/2026-09-20-crumble-threshold-stamped-on-replicated-board-state.md)
superseded (rules replicate); log.

- *As built (2026-10-22):*
  - **Decisions:**
    - [tile powers in the after-move chain](../_decisions/2026-10-22-tile-powers-in-the-after-move-chain.md) (two phases,
      operations as effects, once per action, self-applying events and the display board);
    - [tile catalog and builder tiles](../_decisions/2026-10-22-tile-catalog-and-builder-tiles.md), a second note since
      it is a separate call made at Stage 3;
    - both indexed.
    - The catalog note leaves one point open for the owner: the project settings class that the 2026-10-21
      [no ConnectIt project settings](../_decisions/2026-10-21-no-connectit-project-settings.md) decision had removed.
  - **Question closed:** [tile-powers-in-the-board-pipeline](../_questions/tile-powers-in-the-board-pipeline.md) is
    answered, with a resolution. Its Stage 5 design rounds are carried into the Stage 5 plan.
  - **Code pages:**
    - new recipe `code/recipes/add-a-tile-power.md` and flow `code/systems/tile-power-chain.md` (with a diagram), both
      indexed;
    - `script-a-level.md` (builder tiles, the catalog, Level Wide Powers) and `script-a-phased-level.md` (the catalog
      replaces Default Tile Class, the layout tool places builder tiles, preview / clear, recapture old layouts)
      updated and anchored at `f6d511d`.
  - **Status lines:** `tile-powers.md`, `tile-power-triggers.md`, `board-request-objects.md` (reactions built as tile
    powers; mostly superseded) and `three-directors.md` (Level Wide Powers).
  - **Crumble-threshold decision:** superseded.
  - **Not done here:** the type pages and the `code/__INDEX.md` inventory for the new types (tile power, catalog,
    builder tile, power chain, Power Triggered, project settings). Left to a `/process-code` pass.

## Not in this plan

- The AI's evaluation knowing about powers. The search runs the same chain (it calls `ResolveBoardChange`), so it sees
  what powers do within its depth; Throughput is reported each stage.
- Post-scoring reactions; powers on a turn timer; several switches per pole.
- Player actions that send the new operations (possible once they exist).
- ~~**Known issue:** two board changes closer together than one network update reach a client as one snapshot, losing
  the first change's events.~~ **Fixed 2026-10-21** by
  [one board change at a time](../_decisions/2026-10-21-one-board-change-at-a-time.md): the server locks from each change
  until its visuals have played out, and level changes play one by one. A whole power chain is one change, so this plan
  is unaffected. The display board's resync (Stage 2) stays as a guard for what the lock doesn't cover: a dedicated
  server, where there are no visuals to wait for.

## Verification

- Each stage: build (editor closed); `Automation RunTests ConnectIt` all pass; Throughput against today's.
- Owner, PIE: no power ships with this plan, so the in-editor check is the display board -- ordinary moves (place,
  swap, shift, a scoring move, a layout change) play their events one at a time with tiles, multipliers and score
  stepping along, the same on a 2-player client. Seeing powers in PIE waits for the first-powers design (Stage 5).

## Settled before building (owner, 2026-10-21)

- In the review: a shift is not a Piece Enter / Exit; the gate is a plain `MinMultiplier`.
- Piece Exit acts for the leaving piece's owner; Tile Changed for the faction of the change that caused it (Stage 4).
- The display board applies an event when its entry **completes** (Stage 2).

## Walkthrough changes (owner, 2026-10-22)

- Trigger names everywhere: **Piece Enter, Piece Exit, Piece Changed, Tile Changed** (pre scoring) and **Scoring**.
- A capture is Piece Changed, not Exit + Enter.
- Tiles appearing or disappearing set off nothing; pieces on added tiles are still scored (Stage 1).
- The phased-levels events also apply themselves (Stage 2).
- A tile names its power by **gameplay tag**, not an index (Stage 3).
- **Sync** trigger and a stable generated **`TileId`** (Stage 3 / 4), for one-sided chained tiles.
- Powers use the lock-aware check once the tile lock exists (Stage 4).
- **Stage 5 (operations and first powers) deferred**; the system is proved with test-only powers.
- **Tile power setup** ([tile-power-setup](tile-power-setup.md)): the power is the tile class's identity --
  `TilePowerTag` defaults-only on `AConnectIt_TileBase`, values from the Blueprint subclass's exposed variables, carried
  on the tile data; `_SetTilePower` dropped; tile visuals follow the board through shifts and replacements (Stage 3, 2).
  **Revised the same day** (below).
- **Tile catalog and builder tiles** (owner, 2026-10-22, after the first Stage 3 build measured the per-tile values at
  ~17% search speed): power values are defined once per tag in a tile catalog (tag -> tile class + power; a level is
  part of the tag); designers place builder tiles that name a tag; an editor preview swaps in the catalog's classes
  without saving them; the swap is real at match start; the catalog is chosen on the level config with a project
  default (Stage 3).

Approved by the owner 2026-10-22.
