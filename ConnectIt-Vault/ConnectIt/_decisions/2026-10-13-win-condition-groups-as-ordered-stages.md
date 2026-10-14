---
Date: 2026-10-13
status: Active
superseded by:
tags:
  - rules
  - win-conditions
  - adventure
---
## Decision

1. **Each side lists its win condition groups in order**: `FConnectItRuleSet::PlayerWinGroups` and `AIWinGroups`
   (`TArray<FConnectItWinConditionGroup>`), replacing the single `PlayerWinConditions` / `AIWinConditions` group per side. A group
   is still "all of its conditions" (all-of is the only mode today; other modes would be a setting on the group). A single-goal
   level lists one group.
2. **Completing a group is a checkpoint; completing the last is the win.** How far each faction has got is board state
   (`FConnectItBoardState::GroupsCompleted`); groups complete in order and stay complete. `StampWinState` advances them after every
   move (and the AI's search does the same per simulated move).
3. **Two Blueprint-assignable delegates on `AConnectIt_GameMode`**: `OnWinCheckpointReached(PlayerRole, Faction, GroupIndex,
   GroupName)` for a non-final group, and `OnMatchWon(PlayerRole, Faction)` for the win. The checkpoint rides a new board event
   (`WinCheckpointReached`, tag `ConnectIt.Event.WinCheckpointReached`) so it is replicated and played after the move's visuals.
4. **Removed**: the pre-commit `OnWinConditionsMet` hook and `SetWinConditions` from the earlier same-pass design, with the
   re-check loop, `RulesRevision` and `ReevaluateWinState`. `SetLoadout` stays (a Blueprint on the checkpoint delegate calls it);
   a loadout is not part of a group.
5. **Target score**: the board's `TargetScore` follows the current group; a main-menu override changes each side's last group only.

Supersedes items 1 (one group per side) and 4 (the hook; engine-side checkpoints rejected) of
[the 2026-10-08 groups decision](2026-10-08-win-condition-groups-and-piece-count.md). Piece Count, no default and no AI fallback,
and the warnings stand.

## Why

Authoring only the first goal and relying on a script to swap in the rest hides the level's real structure and makes setup bugs
easy (a forgotten phase, a goal that only exists in a Blueprint). Declaring every goal in the level config puts the whole level in
data, in one place, with one way to build a phased level. The checkpoint and win delegates then give a Blueprint exactly what it
needs to react (change a loadout, play something) without being able to change what counts as winning. Keeping the stage count on
the board, not in the rules, keeps the rules shared plain data and lets the AI's search follow a staged goal.

## What Would Change It

Other ways to combine conditions in a group (any-of); a per-group reward such as a loadout (declarative, no Blueprint needed for the
common case); an AI evaluation that understands stage goals; many levels sharing one phase pattern (a reusable stages asset).
