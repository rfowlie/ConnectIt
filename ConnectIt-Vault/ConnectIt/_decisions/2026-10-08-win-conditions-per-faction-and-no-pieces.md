---
Date: 2026-10-08
status: Partly superseded
superseded by: "[2026-10-08-win-condition-groups-and-piece-count](2026-10-08-win-condition-groups-and-piece-count.md) (item 2, No Pieces, became Piece Count; item 3, the Per Faction composite, was removed)"
tags:
  - rules
  - win-conditions
  - adventure
---
> Item 3 (the Per Faction composite) was removed the same day: the rule set has separate player and AI win
> conditions ([decision](2026-10-08-player-and-ai-win-conditions-on-rules.md)). Item 2 (No Pieces) became **Piece Count**
> with no `StartingPieces` / `bMustHaveScored`, later the same day
> ([decision](2026-10-08-win-condition-groups-and-piece-count.md)). Item 1 (`HasFactionWon`) stands.

## Decision

1. **`FConnectItWinCondition` gains `HasFactionWon(Board, Faction)`** ("has this faction, specifically, won?"). The default
   derives it from `GetWinningFaction`; conditions that can answer directly override it (Score Threshold does).
2. **New win condition `FConnectItWinCondition_NoPieces`**: a faction wins when it has no pieces on the board. It requires the
   faction to have scored at least once by default (`bMustHaveScored`), so an opponent that simply starts empty doesn't
   win on the first move. `StartingPieces` is only the progress-bar denominator.
3. **New composite `FConnectItWinCondition_PerFaction`**: a list of `{ Faction, Condition }`. A faction wins when any of
   its own entries says so; a faction with no entry cannot win. Progress is the best of a faction's entries. The rule
   set's existing single `WinCondition` slot is unchanged: the composite goes in it.
   - Several factions satisfied at once: the lowest slot wins (as for every condition).
   - The single "target score" some UI reads is the first score-based entry's; a main-menu target-score override is
     applied to every score-based entry. Per-faction UI should read the board's per-faction win progress.

## Why

Adventure levels commonly want different goals for the player and the AI (the swap puzzle: the player clears their army, the
AI races to a score). A single shared condition couldn't say that, and asking the existing `GetWinningFaction` for one
faction is wrong when two factions both satisfy a condition (it names only the lowest). `HasFactionWon` is the per-faction
question the composite needs. Keeping the single rule-set slot and nesting the choice inside a composite avoids changing the
rule set's shape, so existing level configs keep working. The game and the AI's search both ask the rule set's win
condition, so both follow the composite with no change of their own.

## What Would Change It

Needing the AI's search to evaluate positions by a non-score goal (its evaluation terms are still score-based); or per-faction
conditions becoming universal, at which point the rule set could hold the per-faction list directly instead of a composite.
