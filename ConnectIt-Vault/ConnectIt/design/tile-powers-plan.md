---
type: plan
status: not-started
approved: true
approved-on: 2026-10-22
completed-on:
playtested: false
design: "[tile-power-triggers](tile-power-triggers.md)"
question: "[tile-powers-in-the-board-pipeline](../_questions/tile-powers-in-the-board-pipeline.md)"
decision:
---

# Plan: tile powers (chain reactions in the after-move step)

Design: [tile-power-triggers](tile-power-triggers.md). Discussion and every decision, round by round:
[tile-powers-in-the-board-pipeline](../_questions/tile-powers-in-the-board-pipeline.md). Power sketches:
[tile-powers](tile-powers.md). Written 2026-10-21; reviewed against the code the same day (round 10 in the question:
Shift, per-arrival survival, the search's turn number, the display board's limits, the scoring-rule interface, the
gate, the guard, Crumble); walked through with the owner and **approved 2026-10-22**. Not built -- building happens in its own session.

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
- Tests: for each operation and a scoring move, Previous + every event's `ApplyTo` == Current, comparing tiles and scores.
- **Owner, editor:** move visual Blueprints that read the current board (tiles, score UI, `CI_PieceVisualHandler`) to
  the display board.

## Stage 3: power data, authoring and turn stamping

- **`FConnectItTilePower`** (new, `Public/Board/Powers/ConnectIt_TilePower.h`, plain thread-safe struct): `Name`
  (gameplay tag, `ConnectIt.TilePower.*` -- the power's identity), `Trigger` (Piece Enter / Piece Exit / Piece Changed /
  Tile Changed / Scoring), `bLevelWide`, `MinMultiplier` (the gate; 0 = none -- a
  plain field until a second kind of gate is needed), `CooldownTurns`, `Effect` (an instanced
  `FConnectItTilePowerEffect`: chooses targets
  from the board, the tile, the faction and the seeded random stream, and returns the board operations to carry out).
- **Rule set:** `TilePowers` (array). Authored on the **Level director** (`TilePowers`, Class Defaults) and copied in
  at match start, like win conditions; replicates with the rules (clients read cooldowns / thresholds for visuals).
- **Tile data:** `PowerTag` (gameplay tag, empty = none) and `PowerReadyTurn` (int32) on `FConnectItTileData`, and
  `PowerTag` on `FConnectItTileInitialData` (so placed tiles and layout cells name their power). **A tag, not an index**
  (owner, 2026-10-22): an index into the power list breaks silently if the list is reordered or changed mid-match; a
  tag survives both and reads clearly in the editor and logs. A gameplay tag is plain data (cheap to copy into the AI's
  search, safe on its thread) and replicates compactly. Firing looks the tag up in the rule set's `TilePowers` (a short
  list; a lookup cache only if Throughput says so). A tag the level doesn't list: warning when the board is built or
  tiles are added, and the tile has no power.
- **Board:** `TurnNumber` (replaces the unused `LastModifiedTurn`) and `RandomSeed` (advanced as powers roll).
- **Every copy of the board resolves at the right turn:**
  - the Mediator stamps the GameState's turn before resolving;
  - the AI's `MakeRoot` stamps the decision turn (the controller already holds `DecisionTurnNumber`), and `ApplyMove`
    adds 1 per ply (cooldowns count all turns) -- otherwise the root carries the opponent's last turn and no power
    ever comes off cooldown inside the search;
  - `Simulate Operation`'s callers stamp the current turn.
- Validation: duplicate power names, a tile naming a power the level doesn't list (or a level-wide one), an effect left
  empty.
- Tests: a tile's power tag finds its power; an unknown tag warns and gives no power; Add Tiles carries a cell's power;
  rules copy; a fresh board has no powers; `ApplyMove` advances the turn.

## Stage 4: firing powers in the chain

- In `ResolveBoardChange`: **settle** collects powers set off by the record's Piece Enter / Piece Exit / Piece Changed / Tile Changed
  (tile's own power and level-wide ones), skips gated-out, cooling-down and already-fired ones, fires them in Piece Enter →
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
- **Sync rules in the settle step** (added 2026-10-22 for
  [tile-lock-and-chained-tiles-plan](tile-lock-and-chained-tiles-plan.md)): besides triggered powers, the rule set can
  hold rules that run on **every settle pass** and keep some state true (first one: chained tiles). They check state,
  not the change record, so they catch what no trigger reports (a partner cleared by scoring). Each appends its own
  events and its changes feed the same settle pass's triggers. They must be idempotent. This stage only adds the hook
  (an ordered list on the rule set, empty by default); chained tiles fill it.
- Once-per-action guard: a bit per (tile, power slot) -- the slots are the tile's own power plus each level-wide power
  (there can be several). Bug guard: 64 settle loops or scoring passes, log + stop.
- Tests use **test-only powers** (simple effects defined in the test code, since the first real powers are deferred):
  the owner's chain (place → score through a "move a neighbour" tile → the moved piece completes a line → its tile's
  power fires);
  settle until quiet; a capture fires a Piece Changed power and no Piece Enter / Piece Exit power; cooldown 0 / 1 / 2 against the turn number; a power with
  no target still shows and cools down; same seed, same result; before + events = after on a chain.

## Stage 5: new operations and the first powers -- deferred

**Deferred (owner, 2026-10-22): to be designed another time**, in its own plan. Kept here as the starting point:
`_MovePiece` / `_AdjustMultiplier` / `_SetTilePower` operations with their events, and the first powers sketched in the
2026-10-21 rounds (Push, Repel, Vacuum, Crumble). Without it, this plan builds the **system** (record, scoring pass,
events, display board, power data, firing) and proves it with **test-only powers** defined in the tests (Stage 4); no
power ships to levels yet.

## Stage 6: vault

Decision note (tile powers: two phases, operations as effects, display board); close the question with it; `code/`
recipe "add a tile power"; `code/systems/` flow for the chain; status lines in `tile-powers.md`,
`tile-power-triggers.md`, `board-request-objects.md` (reactions built); `three-directors.md` (the Level director owns
`TilePowers`) and `code/recipes/script-a-level.md` (powers are authored on the Level director); mark
[the Crumble-threshold decision](../_decisions/2026-09-20-crumble-threshold-stamped-on-replicated-board-state.md)
superseded (rules replicate); log.

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
- Powers use the lock-aware check once the tile lock exists (Stage 4).
- **Stage 5 (operations and first powers) deferred**; the system is proved with test-only powers.

Approved by the owner 2026-10-22.
