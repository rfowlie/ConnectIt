# Player and AI win conditions

Design note (2026-10-08, updated 2026-10-13), design/ is schema-less. **Built, not compiled; tests not run.** Settled in
[the decision](../_decisions/2026-10-08-player-and-ai-win-conditions-on-rules.md), reshaped by
[win condition groups](../_decisions/2026-10-08-win-condition-groups-and-piece-count.md) and then
[ordered stages](../_decisions/2026-10-13-win-condition-groups-as-ordered-stages.md); the current shape, the win path and the
phased-level recipe are in [win-condition-stages](win-condition-stages.md). This note keeps the checklist.

## Shape (current)

- `FConnectItRuleSet` (the level config's `Rules`) holds **`PlayerWinGroups`** and **`AIWinGroups`**: each side's win condition
  groups in the order they are completed (a group = conditions that must all hold; completing the last group is the win), plus
  **`AIFaction`**. **No default and no fallback**: both sides are set explicitly, and the AI does not borrow the player's.
- `AIFaction` is written by `AConnectIt_GameMode::SpawnAndRegisterAI` after `RegisterAIParticipant`, from the AI's player-state
  slot, through `ModifyRules` (which republishes the replicated mirror, `MatchRules`). Online matches leave it unset, so
  everyone uses the player's list.
- Each faction's progress through its list is `FConnectItBoardState::GroupsCompleted`, advanced by `StampWinState` (and by the
  AI's search after each simulated move). The rule set answers `HasFactionWon`, `GetWinningFaction` (lowest slot that has won),
  `GetProgress(Faction)`, `GetTargetScore(Board)` (the current group's), `SetTargetScore` (the last group of each list).
- `FConnectItMinMaxRules` has no win-condition pointer; it asks `Rules.GetWinningFaction` on the board it advanced.

## No default

A new level config starts with empty lists. `IsDataValid` warns for each (the player's also makes the asset invalid, as does a group
with no conditions), and the GameMode logs a warning at match start (the AI's only when an AI is in the match). Tests that need a
win condition use the `RulesWonAt(Threshold)` helper. **Existing level configs lose their win conditions** (a list of groups can't
be redirected from the earlier single value): re-pick them on each, the validation warnings list them.

## To check in the editor (nothing has been run)

- Rebuild with the editor closed, then `Automation RunTests ConnectIt.Board.WinConditions` and `ConnectIt.AI`.
- Re-pick Player and AI Win Groups on each existing level config (including the test config, the sandbox level and the
  swap-puzzle config); confirm each group shows a Name and a condition picker that lists Score Threshold and Piece Count and hides
  the base struct.
- PIE vs the AI with Player = one group of Piece Count Equal 0 + Score Threshold, AI = one group of Score Threshold: clearing the
  pieces after scoring wins for the player, the AI reaching its score wins for the AI. In a 2-player PIE the client should see the
  same rule set and `GroupsCompleted` (replication of the nested instanced structs is unverified).
- The log line "AI controller spawned and registered as ... (faction N)" shows the expected slot.

## Caveats

- A main-menu target-score override changes only the **last** group of each list (checkpoint groups keep their designed values).
- The AI's evaluation is score-based, so an AI whose own goal was a Piece Count would not steer toward it.
- The single `TargetScore` on the board is one number (the current group's); per-faction UI should show `WinProgress`.
