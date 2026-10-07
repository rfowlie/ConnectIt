---
Date: 2026-10-04
status: Active
superseded by:
tags:
  - ai
  - minmax
  - rules
---
## Decision

The MinMax search learns **when the game is over and who won from the level's own win condition**, not from a
hardcoded score threshold and not from a designer evaluation term. `IConnectIt_WinCondition` gained a C++-only
`MakeSearchWinCheck()` returning a thread-safe `FConnectItWinCheck` (`GetWinningFaction(Board)`);
`UConnectIt_ScoreThresholdWinCondition` returns one with its current threshold, and its real `CheckWinCondition`
shares the same static test. The search concept separates the two kinds of scoring:
`EvaluateTerminalState(State, Ply)` for finished games (win/loss by distance) and `EvaluateState(State)` for
unfinished positions only (the evaluation terms; no Ply). A position with no moves and no winner stays non-terminal
(scored by the terms) until [the full-board question](../_questions/full-board-outcome.md) is decided.

## Why

`EvaluateState` was doing two jobs -- detecting a won game and judging an unfinished one -- which confused the owner and
hid a real gap: the search assumed "win = reach a score" while the level's win condition is pluggable, so a future
tile-control win condition would have been invisible to the AI. Win detection is a game rule, not designer taste: as an
eval term it could be removed or set to disagree with the real game. Having the win condition provide its own test
(like the scoring rule provides `ApplyLineScoring`) keeps the AI and the game in agreement automatically.

## What Would Change It

A Blueprint-authored win condition (it can't provide a native search check -- the AI would warn and not see wins), or
a decision that a full board ends the game.
