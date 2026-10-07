# MinMax win detection from the level win condition

Design + plan note (2026-10-04). Approved in plan mode.

## Context
`FConnectItMinMaxRules::EvaluateState` currently does two jobs: it detects a finished game (score ≥ `WinThreshold`
→ ±(`WinValue` − Ply)) and scores unfinished positions with the designer's evaluation terms. That confused the owner,
and it hides a real limitation: the search hardcodes "win = reach a score", while the level's win condition is
pluggable (`IConnectIt_WinCondition`) -- a future "tile control" win condition would be invisible to the AI.
Agreed (10-04): win detection is a game rule, so it comes from the level's own win condition (not a designer eval
term); terminal positions get their own scoring function; `EvaluateState` only ever sees unfinished positions.
A position with no legal moves and no winner (e.g. full board) keeps today's behaviour -- scored by the terms -- and
gets a design note: what a full board means is undecided (online: maybe a draw; adventure: usually a loss, unless the
level's objective is filling the board).

## Design

### Win check -- provided by the real win condition (same idea as `ApplyLineScoring` for scoring)
- `Source/ConnectIt/Public/Board/Rules/ConnectIt_WinCondition.h`: plain C++ `FConnectItWinCheck` (virtual
  `int32 GetWinningFaction(const FConnectItBoardState&) const`, -1 = nobody) -- a thread-safe snapshot of a win
  condition for code that can't touch the UObject (the AI search). `IConnectIt_WinCondition` gains a C++-only
  `virtual TSharedPtr<const FConnectItWinCheck, ESPMode::ThreadSafe> MakeSearchWinCheck() const` (default null =
  "the AI can't see this kind of win").
- `UConnectIt_ScoreThresholdWinCondition`: `FConnectItScoreThresholdWinCheck { float Threshold; }` built from its
  current `WinScoreThreshold` (so the menu's target score is included). Its real `CheckWinCondition` and the check
  share one static `GetWinningFaction(ScoreBoard, Threshold)` so they can't drift.
- `UConnectIt_BoardRules::MakeSearchWinCheck()` wrapper (game thread).

### Search concept + `TAlphaBeta` (`Plugins/UnrealGameIntelligence/.../Search/MinMax/GI_MinMaxAlphaBeta.h`)
- `c_game`: `EvaluateState(State)` (no Ply) + new `EvaluateTerminalState(State, Ply)`.
- `Negamax`: `if (IsTerminalState) return EvaluateTerminalState(State, Ply);` then
  `if (Depth <= 0 || no moves) return EvaluateState(State);`. Ply now only reaches terminal scoring; the Ply comments
  are updated to say so. Keep the owner's `Run` early-out.

### Rules (`Source/ConnectIt/{Public,Private}/MinMax/ConnectIt_MinMaxRules.*`)
- Constructor takes the win check instead of `WinThreshold`; `GetWinThreshold` removed.
- `IsTerminalState` = win check reports a winner. `EvaluateTerminalState` = `WinValue − Ply` / `−(WinValue − Ply)` /
  0 (no winner). `EvaluateState` = Σ weight × term, then the clamp. No win check → warn once at construction; the AI
  can't see wins (today's behaviour for a non-score win condition).
- Comment at the no-moves branch pointing at the full-board design question.

### Decision context + controller + MinMax strategy
- `FConnectItAIDecisionContext`: C++-only (non-UPROPERTY) `WinCheck` shared pointer; `WinScoreThreshold` kept as
  info for Blueprint strategies.
- `AConnectIt_AIController::BeginDecision`: `Context.WinCheck = BoardRules->MakeSearchWinCheck()` (warn if null).
- `UConnectIt_AIStrategy_MinMax`: passes `Context.WinCheck` into the rules.

## Tests (`Source/ConnectIt/Private/Tests/ConnectIt_MinMaxTests.cpp`)
- Helper builds rules with a score-threshold win check; existing 7 tests keep passing.
- New `WinConditionDrivesTerminal`: a test-only win check ("whoever owns (0,0) wins") makes the search take (0,0) --
  proves a non-score win condition plugs in with no AI changes.

## Vault
- Step 0: plan moved here from the plan-mode staging file (done); decision `2026-10-04-minmax-win-detection-from-win-condition.md`; a `_questions/` note on what a full
  board means (owner asked for the note); log + index; `UnrealGameIntelligence/code/MinMax.md` concept update.

## Verification
- Build (editor closed); `Automation RunTests ConnectIt.AI` -- 8 pass, throughput unchanged.
- PIE vs the AI: still takes winning lines and blocks; a menu target-score override still changes when it wins.

## Status (2026-10-04) — implemented, built, unit-tested

Build clean; 8/8 `ConnectIt.AI` tests pass (new: `WinConditionDrivesTerminal` -- a corner-ownership win check plugs in
with no AI changes); throughput unchanged (~387k nodes/s, depth 5 in 1.5 s). Decision:
[2026-10-04-minmax-win-detection-from-win-condition](../_decisions/2026-10-04-minmax-win-detection-from-win-condition.md).
Full-board question filed: [_questions/full-board-outcome](../_questions/full-board-outcome.md).
