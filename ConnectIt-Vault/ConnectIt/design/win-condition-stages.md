# Win condition stages: groups, checkpoints and the win

Design note (2026-10-13), design/ is schema-less. **Built, not compiled; tests not run.** Replaces the 2026-10-08 note
`win-condition-lists-and-runtime-changes` (a single group per side, plus a pre-commit hook to swap goals from a level
Blueprint). The decision is
[win-condition-groups-as-ordered-stages](../_decisions/2026-10-13-win-condition-groups-as-ordered-stages.md).

## Shape

- A **win condition group** (`FConnectItWinConditionGroup`) is one goal: a list of win conditions that must ALL hold
  (all-of is the only mode for now; a group with no conditions is never satisfied), plus an optional `Name` label.
- `FConnectItRuleSet` (the level config's `Rules`) has `PlayerWinGroups` and `AIWinGroups`: each side's groups **in the order they
  must be completed**. A single-goal level lists one group. Both sides are set explicitly, with no default and no fallback (the AI
  never borrows the player's list; in a match with no AI the player's list applies to every faction).
- Conditions today: **Score Threshold** and **Piece Count** (Less Than / Equal / Greater Than a count; progress is measured from
  `FConnectItBoardState::StartingPieces`, taken from the designer's tile data).
- **Progress is board state**: `FConnectItBoardState::GroupsCompleted` (per faction slot). Groups complete in order and **stay
  complete** (latched). The rules stay plain shared data, and the AI's search carries each side's progress with the board.

## Flow

After every move, `FConnectItRuleSet::StampWinState(Board, &ChangeEvent)` (called by the Mediator, and by Blueprint "Simulate
Operation") runs `AdvanceWinGroups`:

1. Per faction, while its next group is satisfied, `GroupsCompleted++`. One move can complete several in a row.
2. Each group completed that is **not the last** appends a `WinCheckpointReached { Faction, GroupIndex, GroupName }` board event.
   Completing the last group appends nothing: it is the win.
3. Progress (`WinProgress`) and the target score are stamped, and `bGameOver` / `WinningFactionSlot` are set for the lowest slot
   that has completed all its groups. The Mediator then appends `GameWon`, as before.

The checkpoint event travels the normal board-event path (replicated, queued on the game event subsystem after the move's scoring
visuals). `AConnectIt_GameMode` listens for its tag and broadcasts, on the server:

- **`OnWinCheckpointReached(PlayerRole, Faction, GroupIndex, GroupName)`**: a non-final group was completed.
- **`OnMatchWon(PlayerRole, Faction)`**: from `HandleGameOver`, the actual win (all groups complete). The match result is written
  to the GameState and `EndMatch()` runs right after.

(`PlayerRole`, not `Role`: `AActor` already has a `Role`.) The AI's search calls `AdvanceWinGroups` after each simulated move, so a
staged goal is a terminal state only when the last group completes.

## Recipe: "score 20, then clear the board with a new loadout"

1. **Level config** `Rules`: `PlayerWinGroups` = group `Score` (Score Threshold 20), then group `Clear` (Piece Count Equal 0);
   `AIWinGroups` = the AI's goal. The whole level is declared in data.
2. **Level Blueprint** (server only): on BeginPlay, *Get Game Mode* (null on clients: branch on *Is Valid*), cast to the ConnectIt
   game mode, **Assign On Win Checkpoint Reached**. In the handler, if `PlayerRole == Player` and `GroupIndex == 0` (or the name is
   `Score`), call **Set Loadout** (Player, the stage-2 loadout). Optionally assign **On Match Won** for a victory screen.
3. **What happens**: reaching 20 completes group 0 (a checkpoint, not a win); the event plays after the scoring visuals; the
   handler queues the loadout; it takes effect when that side's next turn begins. The UI's target score now reads the second group.
   Clearing the pieces completes the last group: `GameWon`, `OnMatchWon`, the match ends.

## Details

- **Target score**: the board's single `TargetScore` is the **current** group's (first score-based condition of the first faction
  whose current group has one). A main-menu target override changes only each side's **last** group; checkpoints keep their
  designed values. Per-faction UI should read `WinProgress` (groups completed plus progress through the current group, over the
  number of groups).
- **Loadout**: `SetLoadout(PlayerRole, Loadout, Timing)`, default **next turn start** (swapping re-seeds every action's use counts
  and cooldowns). `Immediately` is allowed except while a move is being carried out; the checkpoint delegate can run inside that
  window (an event with nothing to wait for completes at once), where it quietly becomes next-turn. Keep delegate handlers light.
- **Warnings**: the level config's data validation (empty player list = warning + invalid; empty AI list = warning; a group with no
  conditions = warning + invalid, since it blocks every group after it) and a log warning at match start.
- Piece Count Equal 0 is true for any empty board: pair it with a Score Threshold in the same group (or as an earlier group) when a
  level should require scoring first.

## Not built / open

- Other ways to combine conditions inside a group (any-of): a setting on the group, later.
- A loadout field on the group (declarative stage rewards): not built, the Blueprint handles it for now.
- The AI's evaluation is score-based, so an AI whose stage goals are Piece Counts does not steer toward them.
- Verify in PIE: the delegate firing once per group after the scoring visuals; the loadout swap at the next turn start and the
  client's action bar rebuilding; `GroupsCompleted` replicating to a client.
