# Rules that change mid-level

Design note (2026-10-06). **Nothing is built.** From a brainstorm about one Adventure level; written up so the approach
and its consequences are on record when the level is made.

## The scenario

An Adventure level where, each turn, a random way to score is active: **horizontal**, **vertical**, or **diagonal**
(both diagonals as one mode -- three modes over the line rule's four axes).

Today a match's rules are set once: the level config holds a `FConnectItRuleSet`, the GameMode copies it when the match
starts ([rules are thread-safe structs](../_decisions/2026-10-06-rules-are-thread-safe-structs.md)), and nothing
changes it afterwards.

## Chosen approach (owner, 10-06): update the rule struct mid-level; mirror it to clients

The level's line-scoring rule *is* what changes, so change it: the GameMode's per-match rule set is mutable, and
`UConnectIt_BoardRequestMediator` reads it through a pointer, so an update applies to the very next move.

The one gap -- rules are server-only, so clients can't show "this turn: diagonals" -- is closed by having the GameMode
push a **read-only copy of the rule set to the GameState, replicated to clients**.

### Shape, when built

- `FConnectItScoringRule_Lines` gains an **allowed-axes** setting (default: all four); scoring only looks along allowed
  axes. That setting is what changes each turn.
- **One entry point on `AConnectIt_GameMode`** for changing the match's rules: apply the change to its own
  `FConnectItRuleSet`, then push the mirror. The GameMode's copy stays the authority; nothing else writes either copy,
  so they can't drift.
- `AConnectIt_GameState`: a replicated `FConnectItRuleSet` (the instanced-struct members replicate natively) with an
  `OnRep` and a rules-changed event for UI and visuals.
- **Trigger:** something server-side bound to `UTurnBasedParticipantManagerComponent::OnTurnChanged` picks the next
  mode and calls the entry point.

### Consequences

- **Mediator:** sees the change immediately (same object).
- **AI:** `AConnectIt_AIController::BeginDecision` copies the GameMode's rule set into the decision context every
  decision, so the search always plays the *current* rules. What it assumes about *future* turns is open (below).
- **AI evaluation:** the Line Potential term and the line-window geometry should only count windows along axes that can
  currently score.
- **Accepted cost:** the active axis is not part of `FConnectItBoardState`, so it isn't in the board snapshot --
  anything that replays or compares board states (and the previous/current pair visuals read) won't see a rule change.
  Reconnecting clients get it from the GameState mirror.

## Open

- **Telegraphed or hidden?** Undecided -- see [the question](../_questions/rotating-rule-randomness.md).
- **What triggers the change:** a level-specific "director" object, or a reusable *turn rule* held in the rule set
  (so "rules that change" is data on the level config rather than code per level).
- **Relation to Phase B:** [board-request-objects](board-request-objects.md) plans an ordered `reactions` array in the
  rule set (run after a move). A turn-start trigger is the same idea at a different moment; worth designing together.

## Alternatives considered

- **Keep the active axis in the board state** (a turn rule writes it into `FConnectItBoardState`; the line rule reads
  it). Claude's recommendation: replicated for free, consistent with the snapshot, rules stay pure functions. Not
  chosen -- more plumbing, and the owner sees this as the level's rule itself changing, not as extra board state.
- **A stateful rule that changes itself on a turn hook.** Rejected: copies of the rule (the GameMode's, the AI's) would
  drift, and it breaks the "no mutable state" contract that lets the background search share rule code.

Related task: "Support rules that change mid-level" in [`_tasks/active.md`](../_tasks/active.md).

## Update (2026-10-07): the mirror and the entry point now exist

Built as part of [live match setup](live-match-setup.md): `AConnectIt_GameState` has the replicated read-only
`MatchRules` (+ `OnMatchRulesChanged`), and `AConnectIt_GameMode::ModifyRules` is the one entry point that changes the
rules and refreshes the mirror. What remains for this scenario: the allowed-axes setting on
`FConnectItScoringRule_Lines`, the turn-start trigger, the AI evaluation respecting allowed axes, and the open
telegraphed-vs-hidden question.

## Update (2026-10-08): Blueprint-callable changes exist

`AConnectIt_GameMode` now has `SetLoadout(PlayerRole, Loadout, Timing)` (Blueprint-callable, server only) on top of
`ModifyRules`, and the delegates `OnWinCheckpointReached` and `OnMatchWon`, so a level Blueprint can change a side's loadout when
it completes a stage of its goals; see [win-condition-stages](win-condition-stages.md). (An earlier version of this update added
`SetWinConditions` and a pre-commit `OnWinConditionsMet` hook so a Blueprint could swap the goal itself; both were removed on
2026-10-13 in favour of declaring every goal up front.) There is still no Blueprint-callable wrapper for changing the scoring
rule's directions mid-level, so the rotating-scoring-axis level still needs one.

## Update (2026-10-08): the allowed-directions setting exists

`FConnectItScoringRule_Lines::ScoringDirections` (flags: Horizontal, Vertical, Diagonal; default all) is written --
see [line-scoring-directions](line-scoring-directions.md). Scoring only finds lines along allowed directions, and the
AI's line windows are built along the same ones. It replicates with the rule set and can be changed mid-match through
`AConnectIt_GameMode::ModifyRules`. Still not built from this note: the per-turn trigger that picks the mode, and UI
that shows it.

## Update (2026-10-21): the Level director changes the rules

The Level director ([three-directors](three-directors.md)) now has Blueprint-callable **Set Scoring Rule** (a scoring rule
struct, e.g. Lines with only some directions ticked) and **Apply Board Operation**, each with a timing (Immediately = once
the board change resolving has played out -- [one board change at a time](../_decisions/2026-10-21-one-board-change-at-a-time.md);
or Next Turn Start), through `AConnectIt_GameMode::SetScoringRule` /
`ApplyLevelOperation`. A per-turn rotation is now Level-director Blueprint work (e.g. On Turn Started -> Set Scoring Rule at
Next Turn Start). Still open: the telegraphed-vs-hidden question ([rotating-rule-randomness](../_questions/rotating-rule-randomness.md))
and UI showing the current mode (listen to the GameState's On Match Rules Changed). Recipe: [[script-a-level]].
