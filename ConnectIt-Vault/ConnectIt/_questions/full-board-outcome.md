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

## Resolution

<Filled when status moves to answered / parked.>
