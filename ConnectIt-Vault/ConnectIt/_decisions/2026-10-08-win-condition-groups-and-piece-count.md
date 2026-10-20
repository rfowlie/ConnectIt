---
Date: 2026-10-08
status: Partly superseded
superseded by: "[2026-10-13-win-condition-groups-as-ordered-stages](2026-10-13-win-condition-groups-as-ordered-stages.md) (items 1 and 4: one group per side became an ordered list of groups; the pre-commit hook and SetWinConditions were removed)"
tags:
  - rules
  - win-conditions
  - adventure
---
> Items 1 (a single group per side) and 4 (the `OnWinConditionsMet` hook, `SetWinConditions`) were replaced on 2026-10-13: each
> side now lists its groups in order, with checkpoint and win delegates
> ([decision](2026-10-13-win-condition-groups-as-ordered-stages.md)). Items 2 (no AI fallback, warnings) and 3 (Piece Count)
> stand.

## Decision

1. **A side's win conditions are a group that must ALL hold.** `FConnectItRuleSet` has `PlayerWinConditions` and `AIWinConditions`,
   each an `FConnectItWinConditionGroup` (a list of win conditions). A faction wins when its group is non-empty and every
   condition in it holds; an **empty group never wins**. This replaces the single `PlayerWinCondition` / `AIWinCondition`
   fields built earlier the same day.
2. **No fallback between sides.** The AI does not borrow the player's conditions: an empty AI group means the AI cannot win.
   The only sharing is by definition: a match with no AI (`AIFaction == INDEX_NONE`, Online) uses the player's group for
   every faction. A missing group is a warning in the level config's data validation and in the log at match start.
3. **No Pieces becomes Piece Count** (`FConnectItWinCondition_PieceCount`): a comparison (Less Than / Equal / Greater Than)
   against a count. "No pieces" is Equal 0. `StartingPieces` and `bMustHaveScored` are gone: progress is measured from the
   starting piece count the board remembers per faction (`FConnectItBoardState::StartingPieces`, stamped from the designer's
   tile data when the board is built), and "must have scored" is now simply a Score Threshold in the same group.
4. **Phased levels ("checkpoints") are scripted by the level, not built into the engine.** The GameMode broadcasts
   `OnWinConditionsMet(Role, Faction)` *before* a win is committed, and has Blueprint-callable `SetWinConditions` and
   `SetLoadout`. A level Blueprint turns a win into a checkpoint by replacing the win conditions (and loadout) from that
   event; the Mediator re-checks the win under the new conditions, and if they were changed the game goes on. See
   [the flow](../design/win-condition-stages.md) (the later design).

Supersedes the Per Faction composite and No Pieces parts of
[the earlier same-day decision](2026-10-08-win-conditions-per-faction-and-no-pieces.md) and the single-field shape of
[player and AI win conditions](2026-10-08-player-and-ai-win-conditions-on-rules.md). `HasFactionWon` on the base condition and
the lowest-slot tie-break stand.

## Why

- **ALL, in a list**: one flat list per side is what a designer reads; "clear the board, having scored" is two small conditions
  instead of a flag inside one. A wrapper condition that nests others was rejected earlier by the owner, and the list keeps
  the rule set's shape the same.
- **No fallback**: hidden inheritance (the AI quietly wins the player's way) is the kind of behind-the-scenes assumption that
  makes a level hard to read. Setting both sides explicitly, with a warning when one is missing, keeps the level data the
  single source of truth.
- **Count and starting pieces come from the grid**: the designer already places the starting pieces on tiles; asking for the
  number again could disagree with the board.
- **Hook, not checkpoint data**: the win is committed into the board snapshot (`bGameOver` latches, the Mediator then rejects
  every move, `GameWon` is replicated and queued), so by the time anything can *listen* to the win event it is too late to
  continue. Intercepting before the commit keeps one win path and lets each level own its phases, instead of a stage counter,
  a checkpoint event and a loadout field in the core rules.

## What Would Change It

Many levels sharing the same phase pattern (then a reusable data asset describing phases, driving the same hook); the AI
needing to plan toward a phase change (its search sees phase 1's goal as a terminal state); an AI with a non-score goal (the
search still evaluates by score).
