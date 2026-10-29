---
Date: 2026-10-22
status: Active
superseded by:
tags:
  - architecture
  - board
  - tile-powers
summary: 'Tile powers run in ResolveBoardChange as one chain per action: settle (Sync, then Enter / Exit / Changed / Tile Changed, until quiet) and scoring (find all lines then apply; Scoring powers after); ends at the first pass with no lines; effects are board operations applied in the chain; once per tile per action; events apply themselves and record their cause; visuals read a display board'
---
## Decision

Tile powers run inside the rules' after-move step, `FConnectItRuleSet::ResolveBoardChange`, as one chain per action:

- **Two phases.**
  - **Settle** (before each scoring pass): Sync powers first, then the Piece Enter, Piece Exit, Piece Changed and Tile
    Changed powers the change sets off, in that order; then again on what they changed, until nothing more fires.
  - **Scoring:** one pass finds every line from every arrival, then applies them all. The Scoring powers on the tiles
    the lines ran through fire after it, and what they changed goes back through settle.
  - The chain ends at the first scoring pass with no lines. The win is checked once, after the chain. There are no
    post-scoring reactions.
- **A power's effect is board operations**, the same kind player actions send.
  - Each is applied with `Apply`, never `CarryOutOperation`, so what it changes stays in this chain.
  - It acts for the faction of the change that set it off.
  - An operation that can't apply is skipped.
- **Each power fires at most once per tile per action**, which ends every chain. Sync powers are the exception: they
  only follow state.
  - A gate (the tile's multiplier) and a cooldown (turns, against the board's turn number) can stop a power.
  - Random choices roll from a seed on the board.
- **Operations report what they really did** in a change record (`FConnectItBoardChanges`). It is what triggers read,
  so the AI's search, which keeps no event list, runs the same chain.
- **Every event applies itself** (`ApplyTo`) and records its cause (`CauseIndex`). Visuals read a **display board**:
  the board before the change, stepped along one event at a time as each finishes playing.

## Why

- **In the after-move step, not the directors:** every path gets powers the same way -- the Mediator, Simulate
  Operation and the AI's search. Then the AI sees what powers do within its depth.
- **Settle before scoring:** a piece moved by a power can complete a line in the same action.
- **Back through settle after scoring:** what scoring changed can set off powers in turn.
- **Ending at the first empty scoring pass, once per action:** chains always end, without a tunable depth limit.
- **Operations as effects:** powers and player actions share the same building blocks, checks and events.
- **The display board:** a change's events play over several seconds. Visuals that read the final board would show
  the end of a chain before it had happened.

## What Would Change It

- **Post-scoring reactions** (powers that react to a whole action's result) -- a third phase.
- **Turn-timer powers** (Repel's countdown, the Stage 5 design): these fire at turn start as their own board change,
  outside a move's chain.
- **A chain too deep for the AI's search to afford:** Throughput is reported each stage.

Plan: [tile-powers-plan](../design/tile-powers-plan.md). Design: [tile-power-triggers](../design/tile-power-triggers.md).
Question: [tile-powers-in-the-board-pipeline](../_questions/tile-powers-in-the-board-pipeline.md). Builds on
[one board change at a time](2026-10-21-one-board-change-at-a-time.md) (a whole chain is one board change) and
[board events are structs carried by the queue](2026-10-07-board-events-are-structs-carried-by-the-queue.md).
Supersedes [the Crumble threshold on replicated board state](2026-09-20-crumble-threshold-stamped-on-replicated-board-state.md):
powers and their gates live in the rules, which now replicate.
