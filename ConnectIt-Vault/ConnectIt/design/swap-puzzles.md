# Swap puzzles: Swap as the only action

Design note (2026-10-08), design/ is schema-less. Two Adventure levels where the player's only action is **Swap**, each
testing a different way to keep the player supplied with pieces. Builds on level 5 ("Swap Heist") of
[adventure-mode](adventure-mode.md) and its [setup checklist](adventure-levels-swap-heist-and-shift-puzzle.md). Only
the win conditions below are built; the replenishment reaction is not.

## The two levels

- **A. Clear the Board (finite army).** The player starts with a fixed army and can never add pieces. Each score clears a
  line (4 pieces), so the level is won by scoring until none are left. Win: **Piece Count Equal 0** (plus a small Score Threshold so it can't be won without scoring). Loss: the AI reaches its score
  target first (defence matters).
- **B. Reinforcements (replenished army).** Same swap-only play, but when the player scores, 3 new player pieces appear
  on random empty tiles (never completing a line). Net -1 piece per score. Needs a new after-score reaction (not built).
  Win: a **score target** (Piece Count Equal 0 would never trigger, since a score always spawns pieces).

Both give the player and the AI different goals: the level config's Rules set a **Player Win Condition** and an **AI Win
Condition** (see below).

## How Swap-only plays (facts that drive the numbers)

- A swap trades positions, so **piece counts never change by swapping**; only the player's own scoring removes their
  pieces. The AI cannot remove player pieces.
- A swap drops an **AI piece onto the tile the player just left**, so every swap is also a risk of handing the AI a
  line. That is the built-in defensive tension.
- A swap that scores clears the **whole line including the swapped-in piece** ([swap that scores keeps nothing](swap-scores-keep-nothing.md)).
- The AI places one piece per turn and is blind to swaps, so the board fills: size it for the whole match.
- The AI needs a piece on the board for the player's first swap to be legal. A player with 0 pieces has no legal swap and
  can only run out the turn timer.

## Starting numbers (to be tuned in playtest)

| | A. Clear the Board | B. Reinforcements |
|---|---|---|
| Board | 7x7, connect 4 | 7x7, connect 4 |
| Player start | 12 pieces, scattered, no line potential | 8 pieces |
| AI start | 4 pieces, not threatening | 4 pieces |
| Player win conditions | Piece Count Equal 0 AND Score Threshold 4 (the 12 starting pieces come from the grid; the score condition stops a win without scoring) | Score Threshold about 16-20 |
| AI win conditions | Score Threshold ~12 | Score Threshold ~12 |
| Swap | permanent action, 1 per turn (verify an unlimited swap works) | same |
| AI | Place only, MinMax depth 3, ~20% weaker-move chance | same |

With replenishment the maximum number of scores is `P - 3` for `P` starting pieces (each score nets -1 piece and a line
needs 4), so 8 pieces allow 5 scores, 20 points at multiplier 1. Keep the player's target below that.

Tune in this order after a playtest: AI strength, AI starting pieces, board size, targets.

## Win conditions (built)

- **Piece Count** (`FConnectItWinCondition_PieceCount`): a comparison (Less Than / Equal / Greater Than) against a count of the
  faction's pieces on the board; "no pieces" is Equal 0. It is true for any empty board, so level A pairs it with a Score
  Threshold. Progress is measured from the starting piece count the board remembers (from the grid), not a number typed in.
- **Player and AI win groups** (on `FConnectItRuleSet`, so on the level config's Rules): `PlayerWinGroups` and `AIWinGroups`,
  each side's goals in the order they are completed; a goal (group) is conditions that must all hold. Both sides are set
  explicitly (no default, no fallback; the AI does not borrow the player's). For these levels: Player = one group of Piece Count
  Equal 0 + Score Threshold 4 (A) or one group of Score Threshold (B), AI = one group of Score Threshold. See
  [the decision](../_decisions/2026-10-13-win-condition-groups-as-ordered-stages.md) and
  [win-condition-stages](win-condition-stages.md).
- A phased variant of A ("score first, then clear the board with a new loadout") is two player groups, `Score` then `Clear`,
  plus a level Blueprint on `OnWinCheckpointReached` calling `SetLoadout`; see the recipe in that note.

## Not built / open

- **B's replenishment reaction** (place N pieces on score, avoiding lines). It fits the planned after-move "reactions" step
  of the rule set; the placed pieces would be reported as piece-placed events for the visuals.
- Who moves first. (The AI's slot is now read when it registers, so which slot the human gets no longer matters to the win
  conditions.) **2026-10-20: swap-only levels need the AI to move first, or AI pieces placed at the start** -- a swap trades
  an own piece with another, so with no AI piece on the board the player has no legal move.
- Whether an unwinnable position (player below 4 pieces, target unmet) needs a defined outcome; today the AI scoring or the
  turn timer ends it. Related to the [full-board question](../_questions/full-board-outcome.md). **2026-10-20: it becomes a
  player lose condition** once the [three directors](three-directors.md) land: *Not Enough Pieces To Score* (fewer pieces than
  the scoring rule's line length) and/or *No Legal Move: Swap Pieces* -- both built (2026-10-20). Players learn to count.

## Exhaustion levels: piece-count rules (2026-10-20)

- **"Divisible by the line length" isn't enough.** The Lines rule clears every piece of a scoring line: a 5-in-a-row clears 5,
  and two lines crossing at the completing piece clear 4 + 4 - 1 = 7. A starting count that is a multiple of 4 can still miss
  zero unless the layout makes long or crossing lines impossible -- or the level accepts it (and the stuck state is a loss).
- **The AI can strand the player.** If the AI clears all of its own pieces (its own scoring), the player has no swap target
  left: *No Legal Move: Swap Pieces* holds. A level rule, or something the AI's strategy must avoid.
- **A side with no legal move passes**: the AI ends its turn at once; the UI can grey out actions with *Has Any Legal Move*.
- The UI's single "target score" is the current group's (the first faction's that has a score-based one); per-faction win
  progress is the number to show when the two differ.
- The AI's search scores positions by score, so an AI whose own goal was a Piece Count would not steer toward it.
