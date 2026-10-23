---
type: plan
status: in-progress
approved: true
approved-on: 2026-10-23
completed-on:
playtested: false
design: "[tile-power-triggers](tile-power-triggers.md)"
question: "[tile-powers-in-the-board-pipeline](../_questions/tile-powers-in-the-board-pipeline.md)"
decision:
requires:
  - "[tile-powers-plan](tile-powers-plan.md)"
  - "[tile-lock-and-chained-tiles-plan](tile-lock-and-chained-tiles-plan.md)"
---

# Plan: the first tile powers (Push, Repel, Vacuum) and tile hooks

The tile-powers plan's Stage 5, moved to its own plan (owner, 2026-10-22). It comes after the tile lock, as the owner
asked: every power answers whether it can touch a locked tile (`CanCarryOut`). Designed with the owner over five rounds
on 2026-10-22: [the question's Stage 5 rounds](../_questions/tile-powers-in-the-board-pipeline.md). Written 2026-10-23;
**approved by the owner 2026-10-23** (the open points settled below).

## Context

Settled in the design rounds:

- **Move Piece** keeps the piece's owner. It is a Piece Exit at From plus a Piece Enter at To, and To must be empty and
  active (and, with the lock, unlocked).
- **Multiplier floor and ceiling per tile type**, on the catalog entry.
  - Some powered tiles shouldn't gain multiplier, or shouldn't add to scores at all (ceiling 0).
  - Plain tiles: floor 1, no ceiling.
  - A 0-multiplier tile still forms lines; it adds 0 points.
- **Push** (Scoring): first find which orthogonal neighbours can be pushed one tile away, then pick one at random among
  them, so a player who sees one option can predict it. It never pushes its own tile's piece.
- **Repel** is a **countdown**:
  - every turn (all turns) counts down; at 0 it resets, even with no piece on the tile;
  - when it reaches 0 with a piece on the tile, the piece moves to a random open tile among the 8 around it, if there
    is one;
  - it ticks at **turn start**, as its own board change, for the faction whose turn it is;
  - the AI's search ignores it for now.
- **Vacuum** fires on Piece Exit **and** Scoring: it pulls the nearest piece along one of the 8 directions, picked at
  random, onto its own (empty) tile -- Repel's opposite, without a timer.
- **Crumble** is set aside: a board power, for its own plan.
- **No level suffix** on power tags.
- **Tile hooks:**
  - `AConnectIt_TileBase` gets one virtual per kind of tile reaction.
  - A visualizer gets the board event and calls the hook, passing a **reaction handle**. The hook calls **Finish**
    when its animation ends (cache the handle; like setting up responses to game events). The event queue waits on the
    visualizer, and the visualizer waits on every handle it handed out.
  - No wrapper: the visualizers call hooks only on the right actors.
  - Each hook's C++ default finishes at once.
  - Tiles are pooled, never destroyed; returning one to the pool finishes its handles.

Each stage builds and passes the tests before the next; the editor must be closed for builds; the owner commits between
stages.

## Stage 1: Move Piece, Adjust Multiplier and multiplier limits

- **Move Piece** `{From, To}` -- request type `ConnectIt.Game.State.MovePiece`:
  - `CanApply`: a piece at From; To a tile, empty and active, not From.
  - Targets From and To, so the lock check refuses a locked one.
  - `Apply`: the piece keeps its owner. It reports `AddExited(From)` and `AddEntered(To)`; the moved piece survives
    scoring, like a placed one.
  - Event **Piece Moved** `{From, To, Faction}` (applies itself; Blueprint getter), so visuals slide the piece.
  - The piece registry re-keys the piece's actor, as it does for a shift.
- **Adjust Multiplier** `{Position, Delta}` -- request type `ConnectIt.Game.State.AdjustMultiplier`:
  - `CanApply`: a tile, and a change that isn't zero after the limits.
  - Targets its tile (not on locked tiles, like Destroy Multiplier).
  - `Apply`: clamped to the tile type's limits; reports a Multiplier tile change.
  - Event **Multiplier Changed** `{Position, Previous, New}` (applies itself; getter).
- **Limits per tile type:**
  - Catalog entries get `MultiplierFloor` (default 1) and `MultiplierCeiling` (default -1 = none). They ride on the
    power definition copied into the rules.
  - `FConnectItRuleSet::ClampMultiplier(TileData, Value)` looks the tile's tag up; plain tiles get floor 1, no
    ceiling.
  - Applied wherever a multiplier changes:
    - the scoring pass's +1 (the scoring rule gets the rules' clamp; Scored's `NewMultipliers` shows the clamped
      value);
    - Adjust Multiplier;
    - Destroy Multiplier's reset to 1 (clamped too, so a ceiling-0 tile stays 0);
    - a tile's starting multiplier, when it joins the board.
  - A 0-multiplier tile still forms lines and adds 0 points (no change needed: points are the sum of multipliers).
- **The gate field is renamed** from `MinMultiplier` to `FireAtMultiplier`, so it isn't confused with the floor.
- Tests:
  - Move Piece: allowed and refused (occupied target, inactive, locked), its record, its event, replay;
  - Adjust Multiplier: clamped both ways; refused when it changes nothing; replay;
  - limits through scoring and through Destroy;
  - the operation catalog grows by 2.
- *As built (2026-10-23; 59/59 tests; Throughput 346k-362k nodes/s, the same as before):*
  - **Move Piece** (`FConnectItBoardOperation_MovePiece`, `ConnectIt.Game.State.MovePiece`) and event **Piece Moved**
    (`ConnectIt.Event.PieceMoved`, applies itself; Blueprint getter).
    - It targets From and To, so neither may be locked.
    - `UConnectIt_PieceRegistry::MovePieceActor(From, To)` (Blueprint) re-keys the piece actor and returns it to
      slide.
  - **Adjust Multiplier** (`FConnectItBoardOperation_AdjustMultiplier`, `ConnectIt.Game.State.AdjustMultiplier`) and
    event **Multiplier Changed** (`ConnectIt.Event.MultiplierChanged`).
    - `CanApply` refuses a Delta of 0. A change the limits swallow still plays its event (Previous = New) and reports
      no tile change.
  - **Limits:**
    - `MultiplierFloor` (1) and `MultiplierCeiling` (-1 = none) on `FConnectItTilePower` (the catalog entry's power;
      a tile type is its tag); `FConnectItTilePower::ClampMultiplier(Power or null, Value)`;
      `FConnectItRuleSet::ClampMultiplier(Tile, Value)`.
    - **How operations reach the rules (not in the plan):** operations never see the rules, so the change record
      carries a pointer to them (`FConnectItBoardChanges::Rules`). It is set by CarryOutOperation, the scoring pass,
      the power chain and the AI's `ApplyMove`. `FConnectItBoardChanges::ClampMultiplier` asks it, with plain-tile
      limits when it's unset (e.g. a test calling `Apply` directly).
    - Applied in the scoring pass's +1 (`ApplyScoringLine` takes the record), Destroy's reset (the event gained
      `NewMultiplier`), Adjust, Add Tiles (the event's cell carries the clamped value, so a replay matches) and the
      board build (`MakeTileData`, from the catalog entry).
    - **Behaviour change:** a plain tile's starting multiplier below 1 is now raised to 1.
  - **The gate is renamed** `FireAtMultiplier`, with a CoreRedirect from `MinMultiplier`.
  - **Operation catalog:** 17.
  - **Tests:** new `ConnectIt.Board.MoveAndMultiplier`:
    - Move Piece: moved, owner kept, record, event, replay, refused cases including locked tiles;
    - Adjust: plain floor, a capped tile's ceiling and floor, 0 refused, locked refused;
    - scoring with a ceiling-0 and a capped tile (points and replay);
    - Destroy to the floor;
    - Add Tiles and the board build clamping starting values.

## Stage 2: trigger sets, Push and Vacuum

- **A power has a set of triggers** (`Triggers`, a bitmask over the trigger enum), replacing the single `Trigger`.
  - Firing checks "has this trigger".
  - The once-per-action guard is unchanged (one power on a tile fires at most once per action, whatever the trigger).
  - Sync stays a trigger of its own (a Sync power has only Sync).
  - Existing catalog entries are set again (owner, editor).
- **Push** -- `FConnectItTilePowerEffect_Push` (Scoring), tag `ConnectIt.TilePower.Push`:
  - Candidates: each orthogonal neighbour holding a piece whose next tile beyond (away from the Push tile) the rules
    allow it to move to -- empty, active, unlocked, and the piece's tile unlocked (`CanCarryOut` on the Move Piece).
  - Pick one at random (the board's seed) and Move Piece it. No candidates: no effect (it still shows and cools down).
  - Never its own tile's piece.
- **Vacuum** -- `FConnectItTilePowerEffect_Vacuum` (Piece Exit + Scoring), tag `ConnectIt.TilePower.Vacuum`:
  - Only when its own tile is empty (after a Scoring trigger: when its piece was cleared).
  - Candidates: the nearest piece along each of the 8 directions, over empty tiles. An edge or an inactive tile stops
    the look. A locked piece can't be moved, so it isn't a candidate.
  - Pick one at random and Move Piece it onto the Vacuum tile, at any distance.
- Tests:
  - Push: only valid pushes, a single valid option is always the one picked, never its own piece, nothing to push =
    no effect, a locked neighbour is skipped;
  - Vacuum: fires on Exit and on Scoring, pulls only into an empty tile, nearest per direction, blocked by inactive
    tiles, skips locked pieces;
  - a chain: a pushed piece completes a line; Vacuum pulls a piece that completes another;
  - same seed, same pick; replay.
- *As built (2026-10-23; 60/60 tests; Throughput 358,461 nodes/s, unchanged):*
  - **Before this stage (owner's request):**
    - the catalog's `TileClass` and `DefaultTileClass` (renamed by the owner from `PlainTileClass`, with a
      CoreRedirect) are `TSubclassOf<AConnectIt_TileBase>`, so the class picker offers only ConnectIt tiles, which
      carry the Stage 4 hooks;
    - the three includes left over from the owner's `ConnectIt_GridPiece` → `ConnectIt_PieceBase` rename were
      updated.
  - **Trigger sets:** `FConnectItTilePower::Triggers` is an int32 bitmask over `EConnectItPowerTrigger` (editor
    `Bitmask`), with `HasTrigger`, `SetTrigger` and `AddTrigger`. Catalog validation adds two errors: Sync with
    another trigger, and no trigger.
  - **The effect context carries the rules** (`FConnectItTilePowerContext::Rules`, set by the chain), so effects can
    ask `CanCarryOut` before choosing a target. Not in the plan; needed for the lock-aware candidate lists.
  - **Push and Vacuum:**
    - `FConnectItTilePowerEffect_Push` and `FConnectItTilePowerEffect_Vacuum`, with native tags
      `ConnectIt.TilePower.Push` / `.Vacuum`.
    - Both build candidate Move Pieces the rules allow, then pick one with the seeded stream. A single candidate
      doesn't roll.
    - Vacuum looks along each of the 8 directions over empty tiles; an edge, a missing tile or an inactive tile stops
      the look.
  - **Tests:** new `ConnectIt.Board.PushAndVacuum`:
    - trigger sets, and the Sync-plus-other validation;
    - Push: the single valid option every seed (own piece, a blocked or locked target excluded), none valid,
      same seed same pick;
    - a Push chain where the pushed piece completes a line for the other side (replay);
    - Vacuum: the nearest movable piece (past empty tiles, stopped by inactive ones, locked skipped), not into an
      occupied tile;
    - Vacuum on Piece Exit (replay) and on Scoring (fires once).

## Stage 3: turn-start powers and Repel

- **New trigger, Turn Start:** fires on every tile that has it when a turn starts and its ready turn has come; then it
  cools down -- the countdown. `CooldownTurns` is the countdown length (all turns count).
  - **The countdown starts when the tile joins the board:** a Turn Start power's ready turn is the turn then plus its
    cooldown, so it fires after N turns rather than at once.
- **Where it runs:** at turn start the GameMode commits a **Turn Start** operation for the faction whose turn it is.
  - It goes first in the turn-start level changes (one board change at a time), before the directors hear the turn
    start.
  - It only does so when some tile's power has the Turn Start trigger.
  - The operation changes nothing itself. It marks its record "turn started", and the chain's settle then fires the
    Turn Start powers on every tile (like Sync, but once).
  - It appends no event unless a power fires.
- **Repel** -- `FConnectItTilePowerEffect_Repel` (Turn Start), tag `ConnectIt.TilePower.Repel`:
  - With a piece on its tile: Move Piece it to a random open tile among the 8 around it (empty, active, unlocked).
    None open: it stays.
  - With no piece: nothing, but the countdown still resets (bNoEffect).
- **The tile shows "turns left"** from its ready turn: Blueprint `Get Power Turns Left (Board, Position)`.
- **The AI's search** doesn't run Turn Start powers (for now, owner): it plans as if Repel never ticks. Noted in the
  recipe.
- Tests:
  - the countdown (fires every N turns, resets with no piece, starts counting at build or add);
  - Repel moves to an open neighbour, stays when boxed in, never onto a locked tile;
  - the Turn Start change is committed only when some tile has the trigger;
  - a repelled piece can score;
  - replay.
- *As built (2026-10-23; 61/61 tests; Throughput 359,319 nodes/s, unchanged):*
  - **Trigger:** `EConnectItPowerTrigger::TurnStart`, added last so existing bitmask values keep their meaning.
  - **Turn Start operation:** `FConnectItBoardOperation_TurnStart` (`ConnectIt.Game.State.TurnStart`; the operation
    catalog is now 18) sets `FConnectItBoardChanges::bTurnStarted`.
  - **Settle:** fires the Turn Start powers first, once (`FConnectItPowerChain::RunTurnStartPowers`), before Sync and
    the triggered powers, so those react to what Repel moved. Cooldown and the once-per-action guard apply as usual;
    the countdown resets even with nothing to do (`bNoEffect`).
  - **The GameMode:** at turn start, after the turn's queued level changes join the line, it puts a Turn Start change
    first (`FConnectItLevelChangeQueue::AddFirst`), for the active faction, when
    `FConnectItRuleSet::HasTurnStartPowerReady(board, turn)`.
  - **Countdown start:**
    - the board build sets a Turn Start power's ready turn to its cooldown (built at turn 0);
    - Add Tiles sets it to the board's turn plus the cooldown, and Tiles Added carries `PowerReadyTurns` so a replay
      matches.
  - **Repel:** `FConnectItTilePowerEffect_Repel` (`ConnectIt.TilePower.Repel`) uses the Push / Vacuum candidate
    helpers over the 8 neighbours.
  - **Blueprint:** `Get Power Turns Left (State, Position, CurrentTurn)`, where CurrentTurn is the GameState's Active
    Turn Number (the board's own turn is its last change's).
  - **Tests:** new `ConnectIt.Board.TurnStartPowers`:
    - the countdown fires on turns 3 and 6 of 7, the first moves the piece, the second (no piece) still resets;
    - replay every turn;
    - not ready → no change;
    - the countdown starts on Add Tiles (turn 5 → 8) and at board build (3);
    - Repel boxed in by locked and inactive tiles does nothing, with a single open tile always used;
    - a repelled piece completes a line.

## Stage 4: tile hooks and reaction handles

- **`AConnectIt_TileBase` hooks** -- `BlueprintNativeEvent`s, each `(Event, Reaction)`:
  - `OnPowerTriggered` (Power Triggered on this tile);
  - `OnPieceEnter` / `OnPieceExit` (a piece placed on, swapped in, captured, moved in, removed from, moved off this tile);
  - `OnMultiplierChanged` (scored through, Adjust or Destroy Multiplier);
  - `OnLockChanged` (Tile Lock Changed);
  - `OnActiveChanged` (Tile Active Toggled);
  - `OnChainPartnerChanged` (this tile's partner, or this tile became or stopped being a partner).
  - Each C++ default calls `Reaction->Finish()` at once.
- **`UConnectIt_TileReaction`** (a small UObject): `Finish()`. A second Finish is ignored. Returning the tile to the
  pool finishes any it holds.
- **`UConnectIt_TileVisualizer`**, owned by the board registry subsystem on every machine:
  - It registers a gated task for each board event tag that concerns tiles.
  - When the event plays, it reads the payload, finds the tile actors at its positions (tile registry), and calls each
    one's hook with a fresh reaction.
  - It completes its task when every reaction has finished, or at once if there was no actor.
  - Runs alongside the existing piece visuals; the queue waits for both.
- **Design note** (in the recipe): a hook that overrides must call `Finish` -- cache the reaction, finish when the
  animation ends.
- Tests: what they can without a world -- reaction finish counting and a second Finish. The dispatch is checked in PIE.
- *As built (2026-10-23; 62/62 tests; Throughput 361,177 nodes/s, unchanged):*
  - **`UConnectIt_TileReaction`** (`Public/Board/ConnectIt_TileReaction.h`):
    - `Finish` (Blueprint), `IsFinished`, `Tile`, and a C++ `OnFinished` callback;
    - a second Finish is ignored;
    - finishing removes it from its tile.
  - **`AConnectIt_TileBase` hooks** (`BlueprintNativeEvent`, each with a Reaction; defaults finish):
    - `OnPowerTriggered(Event)`;
    - `OnPieceEnter` / `OnPieceExit(Faction, Cause)`, where Cause is the board event's tag;
    - `OnMultiplierChanged(NewMultiplier)`, `OnLockChanged(bLocked)`, `OnActiveChanged(bActive)`;
    - `OnChainPartnerChanged(Event)`, on the chained tile and its new partner.
  - **The tile holds its reactions:** `HoldReaction`, `ForgetReaction`, and `FinishPendingReactions` (Blueprint, for
    a pool to call; `EndPlay` calls it too).
  - **`UConnectIt_TileVisualizer`** (`Public/Board/ConnectIt_TileVisualizer.h`), created by the board registry
    subsystem after the tile registry, on every machine:
    - **Tasks:** one persistent gated task per handled tag (12: Power Triggered, Piece Placed / Swapped / Captured /
      Removed / Moved, Scored, Multiplier Changed, Tile Multiplier Destroyed, Tile Lock Changed, Tile Active
      Toggled, Chain Partner Changed), all with one handler that reads the queue's active payload.
    - **Swaps:** the pieces' owners come from the display board, which still shows the board before the event.
    - **Scored:** calls `OnMultiplierChanged` on each tile of the line. Cleared pieces aren't a tile hook (the piece
      visuals show them).
  - **Completion is deferred to the next tick when it happens during the visualizer's own dispatch** (no tile actor,
    or every hook finished at once). The plugin's event queue calls each gated task from inside a loop over that
    phase's tasks, and completing a task removes it from that same array. A later Finish completes at once.
  - **Every handled event now waits at least a frame** for the visualizer, even with no tile actor. Unnoticeable at
    one event per frame, but a long chain plays a frame per event slower.
  - **Tests:** new `ConnectIt.Board.TileHooks`:
    - a reaction finishes once;
    - all seven hook defaults finish;
    - a tile finishing what it holds counts each reaction once;
    - the 12 handled tags.
    - The dispatch itself needs a world: check it in PIE.

## Stage 5: vault

- A decision note: the first powers (Move Piece, limits per tile type, trigger sets, turn-start powers, hooks with
  reaction handles).
- Recipes: "add a tile power" (trigger sets, limits, Turn Start, hooks) and "lock a tile" (Move Piece and Adjust in the
  table).
- The flow `tile-power-chain` (Turn Start).
- Status lines in `tile-powers.md` and `tile-power-triggers.md`.
- The question's Stage 5 rounds marked built.
- Plan properties; log.

## Not in this plan

- Crumble and other board / map powers (their own plan).
- Tiles growing in power (levels).
- The AI's search running Turn Start powers; the AI's evaluation knowing about powers.
- Real tile pooling, if it isn't there yet: Stage 4 only finishes handles on return to the pool.
- Owner, editor: the catalog entries (Push, Repel, Vacuum, Chained), tile classes and their hook animations, the sandbox
  level.

## Settled at approval (owner, 2026-10-23)

1. **Push:** it looks for pieces one tile away with an empty space behind them, then picks one at random.
2. **Repel is a countdown that begins the first turn after the tile joins the board:** its ready turn is the joining turn
   plus N (as Stage 3 says).
3. **Trigger sets replace the single trigger.**
4. **The hook list** is fine for now; more will be added as needed.
