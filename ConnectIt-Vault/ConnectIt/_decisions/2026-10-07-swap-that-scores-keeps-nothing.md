---
Date: 2026-10-07
status: Active
superseded by:
tags:
  - game-design
  - balance
  - scoring
---
## Decision

When a **swap** completes something that scores, the swapped-in piece is cleared along with the rest; nothing is
kept. Placing a piece still leaves the completing piece standing, as do Force Place, Capture and Shift.

- The rule is fixed in code on the operation: `FConnectItBoardOperation::ArrivingPiecesSurviveScoring()` (default
  true; `FConnectItBoardOperation_SwapPieces` returns false). Whoever applies an operation passes its answer to
  `FConnectItRuleSet::ResolveBoardChange`, which hands it to the scoring rule. The Mediator and the AI's search both
  do, so they agree.
- `FConnectItBoardEvent_Scored` carries `ClearedPositions`: the positions whose piece that score removed. Visuals
  despawn exactly those. (`Positions` remains every tile that took part.)

## Why

A balance trial after play testing (owner). Scoring by swap was as good as scoring by placement while also moving an
opponent's piece; making it cost the piece is the counterweight.

`ClearedPositions` was needed regardless: with board events played separately, a Scored event's listener can no
longer see the operation that caused it, so it could not know whether the completing piece stayed.

## What Would Change It

Play testing showing swaps are now too weak, or wanting this to differ per level (then it becomes a setting on the
scoring rule keyed by request type, which was considered and not chosen for the trial).

## Consequences noted

- A swap can complete the other faction's line; that arriving piece is cleared too.
- A scoring swap leaves an empty tile with a raised multiplier open to either player.
- Two lines crossing at the swapped piece both still score; the shared position appears in the first Scored event's
  `ClearedPositions` only.
