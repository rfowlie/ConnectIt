---
created: 2026-10-21
question: "How should tile powers fit into the board pipeline?"
status: open
closed-by:
tags:
  - question
---

# How should tile powers fit into the board pipeline?

## Why it matters

A tile power ([tile-powers](../design/tile-powers.md)) is usually a reaction to a board change that makes another board
change: a line scores through a Poison tile, so its diagonal neighbours lose multiplier. Two things in the code already
react to board changes: the rules' after-move step (`FConnectItRuleSet::ResolveBoardChange`, which only scores today) and
the Level director's triggers (Blueprint, after the move, not seen by the AI's search). Where powers run decides whether
the AI and `Simulate Operation` can see them, whether a chain of effects is one move or several, and whether effects are
written in C++ or Blueprint.

## What would answer it

Owner calls on: where powers run; whether a power belongs to a tile or to the level; what sets a power off and what
happens to it after it fires; which faction a power's effect acts for, and whether a chain can score and win. Then a plan
and a `_decisions/` note.

## Current thinking

**2026-10-21 (owner + Claude), round 1.**

- **Decided (owner): powers run in the rules' after-move step** (`ResolveBoardChange`), not in the Level director. Why: the
  AI's search and `Simulate Operation` go through the same step, so they see powers; a chain of effects stays inside one
  move and one event list. Cost: a power's effect is C++ (a thread-safe struct); designers set its values as data. The
  Level director keeps scripted moments and could give a tile a power through a new board operation.
- **Shape discussed** (not decided): a power's effect issues the existing board operations (Remove, Capture, Force Place,
  Toggle, Destroy Multiplier, plus a new Adjust Multiplier); the after-move step resolves in waves (operation → score →
  powers fire on what changed → their operations → score what they touched → ...) until nothing fires or a cap is
  reached; each tile fires at most once per move; fixed order (wave, then tile index); `StampWinState` once at the end; a
  new `PowerTriggered` event before each effect's own events.
- **Limits found in the code:**
  - Triggers can't read the event list: the search passes `null` for events. The step needs a small, always-produced
    record of what changed (positions a line scored through; pieces that arrived, left or changed owner; multipliers that
    changed). `FConnectItTouchedPositions` is the start of one.
  - Tile data is copied for every position the search looks at: a tile should hold a 1-byte index into a power table on
    the rule set, not a tag or a struct.
  - The rules now replicate (`AConnectIt_GameState::MatchRules`), so clients can read a power's threshold for charge
    visuals (Crumble's cracks). The 2026-09-20 plan to copy the Crumble threshold onto the board state is no longer needed.
  - Board Shift moves tile data, so a power on a tile moves with it.
  - Scoring goes to the occupant of the tile, not to whoever caused the change.

**Claude's leans on the open three** (round 2, for the owner to confirm or change):

- **Tile or level: one mechanism for both.** Powers are an ordered list of power structs on the rule set. A tile power
  only acts on tiles holding its index; a level-wide rule (Crumble, pole sync from
  [switch-and-pole-tiles](../design/switch-and-pole-tiles.md)) ignores the index. The power list would be authored on the
  Level director and copied into the rule set at match start, the same pattern as win conditions. A tile Blueprint names
  its starting power (a gameplay tag through `IConnectIt_TileDataProvider`), resolved to the index when the board is built.
- **What sets it off: an event plus an optional gate.** One event from a small fixed list (a line scored through this
  tile, a piece arrived, a piece left, the owner changed, a neighbour changed) and an optional state check (e.g.
  multiplier ≥ 3). This covers the 09-18 sketch ("once the multiplier is past N, the next line through it fires"), and
  Crumble becomes "scored through, multiplier ≥ 5". Lifecycle: **once** (the power is removed from the tile) or **every
  time** (once per move at most). A recharge comes free when the effect resets its own tile's multiplier, so no new state
  is needed. Powers on a turn timer ("every 3 turns") are left out: the rules don't see turns.
- **Which faction: the one whose change set it off** (the scoring faction for "scored through", the arriving piece's
  owner for "arrived"), passed down a chain. Most sketched powers don't need a faction at all (Poison, Overgrowth,
  Crumble, Magnet); only Siphon (capture) and anything that places pieces do. **Chains score and can win**: scoring is by
  occupant, and the win check runs once after the chain, the same as level operations. A chain can hand points, or the
  match, to the other side; the AI sees that and plays around it.

**2026-10-21, round 3: owner steps back from the brainstormed powers to the logic.**

- **Owner's model.** There are *actions* (place, swap, shift...) and *reactions*. A reaction only happens after an action
  and can chain: P1 places → a line scores → a scored tile has *Push* (pushes a random adjacent piece one tile) → the
  pushed piece completes another line → that line runs over another powered tile → ...
- **Owner's leans:** one power per tile at a time; powers have a cooldown (0 = fires every time it's triggered).
  **Priority: the visuals must understand a chain**; how the AI's search handles it matters less for now. (This replaces
  round 2's "used once / every time, recharge by resetting the multiplier".)
- **The visual side already plays a chain as a list.** A board change is an ordered event list; the board state
  component queues each event with its own data, and the queue waits for one event's visuals before the next
  (`EnqueueBoardEventTags`; the piece registry re-keys per event from that event's data). A chain is just a longer list
  in the order things happened: `PiecePlaced, Scored, PowerTriggered, PieceMoved, Scored, PowerTriggered, ... , GameWon`.
  What's new is on the server: the after-move step needs its own queue of reactions waiting to fire.
- **Touched positions are not enough for the visuals.** Clients only get the board before the action and after the whole
  chain; nothing in between. Each event has to carry everything needed to animate it. Gaps found:
  - No event says a multiplier changed: `Scored` doesn't carry the +1 per tile, and `TileMultiplierDestroyed` has only the
    position. In a chain a tile can go 2 → 3 → 1 → 2.
  - No event for one piece moving one tile (`BoardShifted` describes a whole line).
  - No event for a power firing, or for its state changing (cooldown started).
  - Anything reading the *current* board when the snapshot arrives (`OnBoardStateChanged` listeners, any Blueprint
    reading Current) jumps to the end of the chain before it has played.
- **For the reactions themselves** (server), touched positions say *where* but not *how* (scored through, arrived, left,
  changed owner), which the triggers need.
- **Cooldown needs a turn number on the board:** `FConnectItBoardState::LastModifiedTurn` is set to 0 at init and never
  updated; the turn number lives on the GameState.

**2026-10-21, round 4.**

- **Decided (owner):**
  - **Strict waves**: everything one wave triggers fires before anything those powers cause.
  - **Cooldowns count all turns** (both sides'). Counting only your own turns, or actions, stays a possible design
    option.
  - **The win is checked once, after the chain finishes** (as `StampWinState` already does after scoring).
- **Owner's point: scoring comes first** after an action's changes. A piece placed on a Push tile that pushes itself away
  before it can score would feel wrong. That splits powers into two trigger families: **movement** (a piece arrived on,
  left, or changed owner on this tile) and **scoring** (a line scored through this tile).
- **Current code path** (every path ends in `ResolveBoardChange`): action builds an operation → `ProcessRequest`
  (loadout gate) → `DispatchRequest` (game over? stamp faction, `CanApply`) → `FConnectItRuleSet::CarryOutOperation`
  (`Operation.Apply` → `ResolveBoardChange`: for each touched position in turn, `ApplyScoring` finds lines, adds
  multiplier, +1 multiplier, clears pieces, appends `Scored`) → `StampWinState` → `Game Won` → `SetBoardState` →
  `EnqueueBoardEventTags` on every machine. The Level director's `ProcessLevelOperation` joins at `DispatchRequest`;
  the AI's search and `Simulate Operation` call the same two steps.
- **Proposed wave step** (replaces the body of `ResolveBoardChange`): input = what the last step changed (wave 0: the
  action). A, score at every position a piece arrived at; B, collect powers triggered by this wave's movement changes or
  its scoring, in tile order, skipping tiles on cooldown or already fired this action; C, fire them one after another,
  each against the board as it is then, recording what each changed; D, those changes are the next wave. Stops when B
  finds nothing, or at a cap. Written recursively or as a loop; same thing.
- **Claude's leans on what this raises:**
  - Score first is the fixed rule. "Fires before scoring" could become a per-power option later (a trap tile) without
    changing the loop.
  - A piece cleared by scoring counts as "scored through", not "left", so the two families don't overlap.
  - Scoring within a wave should find every line from every arrival before clearing any. Today it is one position at a
    time, so scoring one arrival can break a line another arrival made.
- **Still open:** whether a piece moved by a power stays on its tile when it completes a line (the operation's
  `ArrivingPiecesSurviveScoring` covers wave 0 only); the display board for the visuals (round 3).

**2026-10-21, round 5: owner's calls.**

- **Display board:** every machine's visuals read a display board that starts as the board from before the action and
  is updated by each event as it plays; it matches the snapshot's Current when the list finishes. Visuals never read the
  snapshot directly. Server-side logic (directors, the Mediator) keeps reading the real board.
- **Scoring within a wave:** find every line made by every arrival first, then score and clear them all (changes
  today's one-position-at-a-time `ResolveBoardChange`).
- **A piece moved by a power that completes a line:** the operation the power issues decides, through
  `ArrivingPiecesSurviveScoring`, as for actions (Place keeps its piece, Swap keeps nothing).
- **Level-wide rules use the same system:** a level-wide rule (Crumble) is a power that applies to every tile, on top
  of any power a tile holds. So the once-per-action guard is per tile *and* power, not per tile.
- **Order within a wave: movement powers first, then scoring powers**, each group in tile order. (Claude had leaned
  tile order regardless of family.)
- So one wave is: A, score every line the wave's arrivals made; B, collect triggered powers; C, fire movement powers,
  then scoring powers, each in tile order, against the board as it is then; D, what they changed is the next wave.

**Claude's defaults, for the owner to confirm or change before a plan:**

- A piece cleared by scoring counts as "scored through", not "left".
- Each event records which earlier event caused it.
- Random choices come from a seed stored on the board, so `Simulate Operation` and the AI's search get the same result
  as the real move; the event records the outcome.
- A power's effect acts for the faction whose change set it off, passed down the chain.
- A power with no valid target still adds its Power Triggered event (marked "no effect") and still starts its cooldown.
- Every event can apply itself to a board; that is how the display board advances. A test checks that the board before
  the action plus every event equals the board after.
- Cooldown is stored as the turn the power is ready again; the Mediator stamps the turn number on the board.
- A wave cap as a safety net, on top of the once-per-action guard.
- New events: multiplier changed (or the new multipliers on `Scored`), Piece Moved, Power Triggered, and Power Changed
  (a tile given or losing a power).

**2026-10-21, round 6: pre-scoring powers come back.**

- **Owner:** some tiles should fire *before* scoring; it adds a layer of complexity. This reverses "score first is the
  fixed rule" from round 4: timing becomes a per-power setting. Triggers proposed: Tile Enter, Tile Exit, Scoring, Tile
  Change; timings: Pre Scoring, Post Scoring.
- **Chart and the wave with both timings:** [tile-power-triggers](../design/tile-power-triggers.md). A wave becomes
  changes in → settle (pre powers) → find lines → pre-scoring Scoring powers → score → react (post powers) → next wave.
- **Open there:** Scoring × pre read as "a line is about to score" (between finding lines and applying them); whether
  the settle step runs until nothing more fires; where Capture belongs; one cell per power; the react order.

**2026-10-21, round 7: the chart's open points, decided (owner).**

- Pre-scoring powers settle until nothing more fires, then the wave scores.
- Scoring powers are post only (the "about to score" moment is dropped).
- One trigger and one timing per power.
- Firing order within a step: Enter, Exit, Scoring, Change (Change last, so Crumble sees what scoring powers did).
- A capture counts as both Tile Exit and Tile Enter (owner's suggestion; Claude agreed). Quirk: an Exit power that needs
  an empty tile finds it occupied after a capture and does nothing.
- The wave is now: changes in → settle (pre: Enter, Exit, Change, until quiet) → score (all lines at once) → react
  (post: Enter, Exit, Scoring, Change) → next wave. Updated in [tile-power-triggers](../design/tile-power-triggers.md).
- **Still unconfirmed:** Claude's round-5 defaults (cleared-by-scoring is not an exit, cause index on events, seeded
  randomness, faction passed down the chain, a power with no target still shows and starts its cooldown, events apply
  themselves to the display board, ready-again turn, wave cap, the new events).

**2026-10-21, round 8: two phases, no post-scoring reactions (owner).**

- **Owner:** post-scoring reactions are almost indistinguishable from scoring reactions and complicate the chain. Two
  phases only: **pre scoring** and **scoring**. Resolve every pre-scoring reaction and its chain; then score; if scoring
  sets off reactions, resolve them and score again; conclude only when there is no new scoring and no reaction to
  scoring. A post-scoring step could be inserted at the end later; omitted for now.
- **Director, checkpoint and win reactions** come after the chain, as new board requests (the Level director's existing
  path already works this way).
- **Effect on earlier rounds:** the per-power pre/post timing (round 6-7) is gone; a power's phase follows from its
  trigger (Enter / Exit / Change: pre scoring; Scoring: scoring). The "React" step and its Enter, Exit, Scoring, Change
  order are replaced by: settle (Enter, Exit, Change) → score → scoring powers → back to settle.
- **Claude's reading, to confirm:** what scoring and the scoring powers change goes back through the settle step before
  the next scoring pass -- the multipliers scoring raised (so Crumble, a Change power, fires there) and what scoring
  powers moved (so a pushed piece landing on a Repel tile is repelled). The chain ends at the first scoring pass that
  finds no lines. Written up in [tile-power-triggers](../design/tile-power-triggers.md).
- **Confirmed (owner, round 8):** that reading is right -- scoring's and the scoring powers' changes go back through
  settle before the next scoring pass, and the chain ends at the first scoring pass with no lines.

**2026-10-21, round 9: the round-5 defaults (owner).**

- **Agreed:** a piece cleared by scoring sets off Scoring powers, never Tile Exit (Tile Exit = a piece moved off the
  tile by anything other than scoring); seeded randomness; a power with no effect still shows and starts its cooldown;
  events apply themselves to the display board, with the before + events = after test; a cause index on events if it
  stays cheap.
- **Wave cap:** owner points out the once-per-action guard already guarantees the chain ends (finite tiles, one power
  each, plus level-wide rules). Agreed: the cap is only a bug guard (log + stop), not a rule.
- **Powers are operations (owner's direction):** "operation" covers player actions and tile powers alike -- anything
  that changes the board. A power's effect is a board operation with its own event, so a power and a player action can
  share it (a Push action and a Push tile). A power adds trigger, gate, cooldown and how it picks its target; an action
  adds the loadout and the player's chosen target. Random targets are picked (from the board's seed) before the
  operation runs, so the operation itself stays deterministic. Power Triggered stays as its own event, before the
  operation's event.
- **Clarification asked:** which faction a power's operation acts for (examples given); how a cooldown counts (stored as
  the turn the power is ready again, compared rather than ticked).
- **Decided (owner):** a power acts for the faction whose change set it off -- the scoring faction for Scoring powers,
  the arriving piece's owner for Enter. (Claude's extension, to confirm in the plan: Exit = the leaving piece's owner;
  Change = the faction of the change that caused it, e.g. the scoring faction for a multiplier raised by scoring; -1
  when nobody, e.g. a Level director toggle.)
- **Cooldown as explained:** a tile stores ready turn = the turn it fired + cooldown; it can fire when the current turn
  ≥ ready turn. Cooldown 0 = again in a later action of the same turn; 1 = the opponent's turn; 2 = your next turn. The
  Mediator stamps the current turn on the board (`LastModifiedTurn` is never updated today).

**2026-10-21: plan written** at the owner's request: [tile-powers-plan](../design/tile-powers-plan.md) (six stages; not
approved yet). Three points to confirm are listed at its end.

**2026-10-21, round 10: the plan reviewed against the code (Claude; owner answered three calls).**

- **Owner's calls:**
  - **A shift is neither Tile Enter nor Tile Exit.** Board Shift moves whole tile data, so power, cooldown and piece
    travel together; shifted positions are only scored (a separate `Moved` list in the change record).
  - **The gate is a plain `MinMultiplier`** (0 = none) until a second kind of gate is needed.
  - **`_SetTilePower` stays in Stage 5.**
- **Fixed in the plan:**
  - Scoring survival is per arrival (one pass mixes the action's pieces with pieces moved by powers).
  - The AI's root board is stamped with the decision turn and `ApplyMove` adds 1 per ply. Otherwise cooldowns are off
    by one at the root and never expire inside the search.
  - A power's operations call `Apply`, not `CarryOutOperation` (which would nest a chain).
  - The display board covers tiles and scores; match-level fields are copied from Current when the queue drains; it
    resyncs and warns if it ends up different.
  - The scoring rule gets one line-agnostic `ScorePass` call instead of `FindLines` / `ApplyLines` on the base.
  - The once-per-action guard has a bit per level-wide power, not one for all of them.
  - Crumble does nothing on an inactive tile (Toggle would otherwise reactivate it).
- **Known issue noted, not fixed:** two board changes in one frame can reach a client as one snapshot, losing the first
  change's events (predates powers).

**2026-10-21, round 11: the plan's last three points (owner).**

- **Faction:** Exit acts for the leaving piece's owner; Change for the faction of the change that caused it (-1 when
  nobody).
- **Display board:** applies an event when its queue entry completes.
- **First powers:** Push, Repel, Crumble, **plus Vacuum** so every trigger ships. Claude's reading of Vacuum, written
  into the plan for the owner to correct: the nearest piece in the tile's four orthogonal lines (ties broken by the
  seed) is pulled into the emptied tile, at any distance.
- **Collapsing snapshots (owner's direction):** lean on the turn / match phases. An operation phase ends only when its
  visuals are complete, and level changes happen at turn end and turn start, so board changes are spaced out. Gaps
  noted in the plan: an Immediately level change runs in the same call as the move, and several turn-start changes
  apply back-to-back. Either they wait for visuals, or same-moment changes merge into one board change. Not decided.
- The plan has nothing left to confirm; it waits for approval.

**2026-10-21, round 12: one board change at a time (discussed, not decided).**

- **Owner:** a second request arriving while one is resolving and its visuals play is a serious error. Proposal: a
  successful request moves the match into a "processing request" state; the Mediator refuses new requests until the
  visuals' completion event arrives.
- **Claude, from the code:**
  - The client already freezes its action stack until the outcome RPC (`bAwaitingRequestConfirmation`), but that
    arrives straight after the server call, not after the visuals. The server's only guard is `IsDispatching`, which
    lasts for the synchronous call.
  - The turn-end gate already waits for the server's queued board events (FIFO), but only the server's.
  - **"Visuals complete" needs an owner:** the server's queue reflects the host's visuals; on a dedicated server it
    would be instant; clients' playback isn't awaited.
- **Claude's leans:**
  - The server owns the lock: player requests are refused (the existing failure path re-enables the client's action).
  - Level changes queue behind the lock rather than being refused (Immediately = after this change's visuals; several
    turn-start changes play one after another).
  - Release on a marker tag queued after the change's events (`BindOnTagComplete`), with a safety timeout.
  - The busy state is replicated so client UI can block input.
  - A ConnectIt-side state, not a new plugin turn phase.
  - Client acknowledgement before release comes later, with online play (still undecided).
- **Decided (owner):** keep it ConnectIt-side; server visuals for now. Plan:
  [one-board-change-at-a-time-plan](../design/one-board-change-at-a-time-plan.md).
- **Built and settled (2026-10-21):** [one board change at a time](../_decisions/2026-10-21-one-board-change-at-a-time.md).
  For tile powers this means a whole chain is one board change. The chain's events play before anything else changes
  the board; director reactions follow as their own changes. The tile-powers plan's known issue is closed.

## Resolution

