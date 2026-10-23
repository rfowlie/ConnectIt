---
created: 2026-10-23
question: "Should the Level director get its own artificial turn (a phase after each player's turn) for level changes?"
status: open
closed-by:
tags:
  - question
---

# Should the Level director get its own artificial turn?

## Why it matters

Level changes (board changes from level triggers, phase changes) must not collide with a player's turn or a move's chain.
The idea from the owner's brain dump: treat the Level director as a third participant with a turn after each player's turn,
a clean entry point for level changes. It would not be used in the AI's calculations.

## What would answer it

A real collision in play that the existing timings don't prevent, or a level mechanic that needs a guaranteed slot between
turns that triggers can't give.

## Current thinking

2026-10-23: the owner chose to **keep this as a question only** (no plan). Today a level trigger's board change goes through
`ProcessLevelOperation` with a timing: Immediately (right after the move being carried out, never mid-scoring) or Next Turn
Start (before the next turn's directors hear it), chains capped at 64
([level-trigger-board-changes](level-trigger-board-changes.md)). Powers on a turn timer fire at turn start as their own
board change, outside a move's chain ([tile-powers-in-the-after-move-chain](../_decisions/2026-10-22-tile-powers-in-the-after-move-chain.md)).
Revisit only if a collision shows up.

## Resolution

<Filled when status moves to answered / parked.>
