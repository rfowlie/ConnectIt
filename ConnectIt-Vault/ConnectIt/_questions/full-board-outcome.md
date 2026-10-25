---
created: 2026-10-04
question: "What should a full board (no legal moves, nobody has won) mean for the match?"
status: open
closed-by:
tags:
  - question
---

# What should a full board (no legal moves, nobody has won) mean for the match?

## Why it matters

Nothing defines it today. The real game has no rule for it, and the MinMax AI treats such a position as unfinished: it
is not terminal (`FConnectItMinMaxRules::IsTerminalState` only asks the level's win condition), so the search scores it
with the designer's evaluation terms like any other position. ConnectIt's line clearing makes a full board rare, but
not impossible (inactive tiles, blockers, small maps). Whatever the answer, both the match flow and the AI's search
need to agree on it.

## What would answer it

A design decision per mode, then expressing it as a rule the game and the AI share -- most likely through the win
condition (`FConnectItWinCondition` in the match's `FConnectItRuleSet` -- since 2026-10-06 the game and the AI's
search call the same rule struct, so one answer there covers both).

## Current thinking

Owner, 2026-10-04: it probably differs by mode --

- **Online / versus:** possibly a **draw**.
- **Adventure:** most of the time a **loss** for the player,
- **unless** the level's objective is to fill the board, in which case it's the win.

That suggests it belongs to the level's win condition (per level), not a global rule. If it becomes a terminal outcome,
the search would need `IsTerminalState` to also report "no moves left" and `EvaluateTerminalState` to return the
draw/loss/win value for it. Until decided, the AI keeps judging a full board with its evaluation terms.

2026-10-20: the pieces to express it now exist. The condition *No Legal Move: Place Piece* (board-only) is met on a full
board, and conditions are evaluated by the rule set, so the AI's search sees the same answer. Once lose conditions land
with the three directors ([three-directors](../design/three-directors.md)), an adventure level can list it as the
player's lose condition (or as a goal, for a fill-the-board level) -- the per-level answer above. A **draw** is still not
expressible (no draw outcome exists). Separately, a side with no legal move now **passes**: the AI ends its turn at once,
and the UI can grey out actions with *Has Any Legal Move*.

2026-10-24 (owner): **reframed as a "stuck board".** A full board isn't the problem -- Swap or Shift can still score
and clear it. What ends a level is **no participant able to make any move**. Every level must say what that means, so
it becomes **its own parameter** on the level (not one of the optional conditions): Adventure usually = the player
**loses**; exceptions possible (a "freeze the board" level where stuck = **win**). Claude: "able to make a move" should
count uses (permanent action uses left, item budget), which also answers
[out-of-uses-condition](out-of-uses-condition.md); a **draw** outcome still doesn't exist (matters for versus, not
Adventure); the parameter fits the Level director (it owns the level's rules) with the AI's search seeing a stuck
board as terminal only as far as it can tell (it doesn't track uses). Next: a small plan. See
[adventure-mode-roadmap](../design/adventure-mode-roadmap.md).

2026-10-24, round 4 (owner): **Stuck Board = both players have no way to change the board**, reached through the flow
in [adventure-mode-roadmap](../design/adventure-mode-roadmap.md) (No Moves → Stuck Player for this side → for the
opponent → Stuck Board outcome, Lose / Win / Draw on the Level director, default Lose). Claude: it should also count
legality (unlimited Place on a full board).

## Resolution

<Filled when status moves to answered / parked.>
