---
type: plan
status: not-started
approved: true
approved_on: 2026-10-25
completed_on:
started:                       # YYYY-MM-DD, when the first stage was built (blank until then)
stages: 6              # how many stages the plan has
current_stage: not started   # the first stage not yet built ("complete" / "not started")
playtested: false
design: "[adventure-mode-roadmap](adventure-mode-roadmap.md)"
question: "[full-board-outcome](../_questions/full-board-outcome.md); [out-of-uses-condition](../_questions/out-of-uses-condition.md)"
decision:
requires: "[player-actions-and-items-plan](player-actions-and-items-plan.md)"
---

# Plan: level outcomes (No Moves, Stuck Player, Stuck Board, Turns), a test opponent, shift distance

Design: [adventure-mode-roadmap](adventure-mode-roadmap.md) ("Level outcomes", rounds 2-4) and
[adventure-content-definitions](adventure-content-definitions.md) ("Shift distance"). Written 2026-10-24 in a
design-only session; **approved by the owner 2026-10-25**. Not built -- building happens in its own session.

> **Prerequisite:** [player-actions-and-items-plan](player-actions-and-items-plan.md) (in progress) -- "can this side
> move" counts the item inventory and the item budget, which that plan builds (its Stage 3). Stages 4-5 here don't
> depend on it.

## Context

Settled with the owner on 2026-10-24:

- **Three states:**
  - **No Moves** -- the side can't make any move *now* (cooldowns, turn caps, no item budget this turn...) and its
    turn-end requirements aren't met → its turn is **force-ended**.
  - **Stuck Player** -- the side can **never** move again: no unlimited action, and every available action's match uses
    are spent (items count while the match budget allows them).
  - **Stuck Board** -- **neither side** has an action with uses left **and** a legal target (legality counts: unlimited
    Place on a full board is stuck).
- **The flow** after each action, and when a turn begins:
  1. the side has a move now → carry on;
  2. otherwise, if **No Moves** is one of the level's conditions → run it (it may win, lose or change phase);
  3. if it isn't, or it didn't end the match → check **Stuck Player** for this side (a listed Stuck Player condition
     runs here -- the puzzle-level loss);
  4. not stuck → **force-end the turn**; stuck → check the opponent; both stuck → **Stuck Board** outcome. Only this
     side stuck → its turns keep being force-ended while the opponent plays on.
- **Stuck Board outcome** on the **Level director**: **Lose / Win / Draw**, default **Lose**. **Draw** is new.
- An item the player holds but the budget won't allow is not a move. The stuck checks only run when No Moves triggers.
- **Turns condition** -- reaching turn N wins or loses (the simple tool for most puzzles).
- **Test opponent** -- an AI that uses its permanent actions one at a time, rotating each turn.
- **Shift distance** -- one Shift action; the level's action entry sets **max steps** (default 1); the player picks
  1..max; one use per shift. (Progression and "Big Shift" items are wishlist.)

Each stage builds and passes the tests before the next; the editor must be closed for builds; the owner commits between
stages.

## Stage 1: can this side move? (queries)

- **On the server** (uses live on the PlayerState), two queries per side, in one helper (e.g. a ConnectIt function
  library or the Mediator):
  - **`CanMoveNow(Side)`** -- any permanent action usable now (`CanUseAction`: match uses, turn cap, cooldown) with a
    legal instance on the board (`FConnectItBoardOperation::GetLegality` for each request type it produces), **or** any
    held item with item budget left this turn and a legal instance.
  - **`IsStuck(Side)`** -- ignoring cooldowns and turn caps: any permanent action with match uses left (or unlimited)
    with a legal instance, or any held item with match budget left and a legal instance. None → stuck.
  - Legality `Unknown` (a kind of move that can't tell) counts as possibly legal -- never stuck on a guess.
- **Stuck Board** = `IsStuck` for every side.
- Tests (with a seeded PlayerState / loadout where possible): a cooldown → can't move now but not stuck; uses spent →
  stuck; unlimited Place on a full board → stuck; an item over budget doesn't count; Unknown legality is not stuck.

## Stage 2: the flow and the outcomes

- **Where it runs:** the GameMode (the turn hub), after each committed action and when a turn begins (the side about to
  act), following the four steps above.
- **No Moves and Stuck Player as conditions:** two new condition kinds a level lists like others (win or lose groups;
  a phase reacts through the Level director's events). They can't be plain rule-set conditions -- they need the server's
  answers from Stage 1 -- so they're **server-evaluated conditions**: the rule set carries them as markers, the GameMode
  answers them; the AI's search treats them as unknown (never met).
- **Force-ending a turn:** the participant's turn end (`UTurnBasedParticipantComponent::ServerSubmitTurnEnd`, as the AI
  pass already does), with a board-independent log line; a client UI cue is an event (`Turn Skipped`, owner's visuals).
- **Stuck Board outcome** on `UConnectIt_LevelDirector`: `StuckBoardOutcome` (Lose / Win / Draw, default Lose); Lose /
  Win from the human side's point of view (in versus: Lose → both lose = Draw? -- see "To confirm").
- **Draw:** `bGameOver` with no winner (`WinningFactionSlot = -1`), a **Game Drawn** board event, a match-end reason
  **Stuck Board** (and **Draw** on the match result); directors hear `OnMatchDrawn`.
- Tests: the four-step flow on scripted states (move now; No Moves condition met; stuck vs not; both stuck → each
  outcome); a stuck side keeps being skipped while the opponent plays; Draw sets game over with no winner.

## Stage 3: Turns condition

- **`FConnectItMatchCondition_Turns`** (`Board/Rules/Conditions/`): met when the board's `TurnNumber` reaches N
  (counting all turns, or the side's own turns -- a setting). Board-only, so the AI's search sees it.
- Tests: met at N, not before; own-turns counting.

## Stage 4: the rotating test opponent

- **`UConnectIt_AIStrategy_RotatingActions`**: each decision takes the next permanent action in the AI's loadout (in
  list order, wrapping) that has a legal target, picks a target from the board's seeded random stream, and returns that
  one move; skips actions with no legal target; with nothing legal it returns no move (the controller passes, as today).
- For testing boards without a level goal; an AI director Blueprint with just this strategy.
- Tests: rotates Place → Swap → Shift across turns; skips an action with no target; same seed, same moves.

## Stage 5: shift distance

- **Plugin (`UnrealTurnBasedMechanics`):** `FPermanentActionConfig` gains an optional **`ActionSettings`** (instanced
  struct) -- generic per-entry settings an action can read; empty by default.
- **ConnectIt:** `FConnectItShiftSettings { MaxSteps = 1 }`. `UConnectIt_BoardShiftAction` reads the entry's settings
  (default 1 when absent) and lets the player pick 1..MaxSteps (the UI chooses how -- e.g. drag further);
  `ShiftSteps` (defaults-only, fixed per class) is removed.
- **The Mediator's check** becomes `1 <= Steps <= MaxSteps` from the requesting player's own entry for that action (not
  the action class), refusing anything else; a full lap still refused.
- One use per shift, whatever the distance.
- The AI's search keeps modelling Place only (unchanged).
- Tests: a request within the entry's max is accepted, above it refused; no settings → max 1; one use spent per shift.
- **Owner, editor:** set max steps on the loadouts that want longer shifts; the shift UI's distance picking.

## Stage 6: vault

Decision notes (level outcomes: No Moves / Stuck Player / Stuck Board with Draw; shift distance per entry, player-picked
-- supersedes part of [the 10-08 decision](../_decisions/2026-10-08-board-shift-n-steps-fixed-per-action.md)); answer
[full-board-outcome](../_questions/full-board-outcome.md) and [out-of-uses-condition](../_questions/out-of-uses-condition.md)
with them; recipes (conditions; the test opponent); log.

## Not in this plan

- Shift distance as progression; "Big Shift" items (wishlist).
- Pre-set (scripted) tutorial moves (with the teaching layer, later).
- The AI's search understanding uses, items or stuck states.

## Verification

- Each stage: build (editor closed); `Automation RunTests ConnectIt` all pass.
- Owner, PIE: a Swap-only puzzle with 3 uses and a Stuck Player lose condition loses after the third swap without the
  score; the same with a Turns condition; a board filled up with no swaps left ends with the level's Stuck Board outcome
  (try Draw); a cooldown skips a turn instead of ending the level; the rotating test opponent plays place / swap / shift
  in turn; a loadout with max steps 2 lets the player shift 1 or 2.

## Settled before approval (owner, 2026-10-25: all three as written)

1. In versus (no AI), what Stuck Board **Lose / Win** mean -- lean: only **Draw** is offered for versus presets; Lose /
   Win are from the human player's side in AI matches.
2. No Moves and Stuck Player are server-evaluated conditions the AI's search never sees as met.
3. Shift's per-entry settings live in a generic `ActionSettings` instanced struct on the plugin's permanent action
   config.
