---
Date: 2026-09-18
status: Active
superseded by:
tags:
  - scoring
  - visuals
  - swap
  - convention
---
## Decision

Any action that can complete a scoring line must feed the scoring-line **visual update**
every position the action mutated — not just the positions the scoring rule reports. For
SWAP that means both swap positions **and** the position of the piece that triggered the
line; for a placement it means the placed position. Anything writing a new scoring-capable
action (or a new scoring rule) has to check this explicitly.

## Why

A real bug, found in playtesting and fixed by the owner on 2026-09-18: when a SWAP
completed a line, the scoring-line visual update removed the visuals for the scoring
line's pieces — including the one piece that is *supposed to remain* (the piece that
completed the line). Board state still had that piece registered, so the two layers
disagreed:

- The tile looked empty but was occupied, so a piece **could not be placed there**.
- The player then appeared to **score a line from 3 in a row**, because the invisible
  piece was still counting toward it.

The visual update only knew about some of the positions involved in the move. Fix: the
scoring-line visual path now takes the placed-piece position as well as the swap
positions.

This is the same shape as the registry desync — presentation drifting from
`FConnectItBoardState` — but a different cause: there the position→actor map went stale;
here the visual update was handed an incomplete position set. Board state itself was
correct throughout.

## How to apply

- New action that can score → list every position it mutates and confirm each reaches the
  scoring-line visual update. Include the recipe's new check in
  [add-a-scoring-rule](../code/recipes/add-a-scoring-rule.md).
- Note `ScoringLinePositions` on `FConnectItBoardChangeEvent` is the union of lines the
  *rule* found; it is not by itself the full set of positions a move touched.
- Verify in PIE by completing a line **via the new action**, then confirming the
  completing piece is still visible and its tile still rejects a second placement.

## What Would Change It

A scoring-visual path that derives its positions from board-state diffs instead of being
handed a list would make this class of bug impossible by construction. Not built.
