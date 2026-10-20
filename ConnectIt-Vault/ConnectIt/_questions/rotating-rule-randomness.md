---
created: 2026-10-06
question: "When a level's rule changes at random each turn, is the next rule telegraphed or hidden -- and what does the AI assume?"
status: open
closed-by:
tags:
  - question
---

# When a level's rule changes at random each turn, is the next rule telegraphed or hidden -- and what does the AI assume?

## Why it matters

It decides what the mechanic *is* for the player (plan around what's coming vs react to what you get), how the change is
stored and replicated, and whether the MinMax AI can look ahead through a rule change at all. Raised by the
rotating-scoring-axis Adventure level -- see [mid-level-rule-changes](../design/mid-level-rule-changes.md).

## What would answer it

A design call for that level (and whether it should be a per-level setting), ideally after trying both in a playtest.

## Current thinking

Owner, 2026-10-06: undecided.

- **Hidden until the turn starts:** nobody can plan for it. The AI's lookahead must either assume the current rule
  persists (cheap, wrong beyond its own move) or average over the possibilities at each future turn (correct; every
  future turn costs three times the work).
- **Telegraphed queue:** the server rolls a few turns ahead and publishes the upcoming modes. Players can set up lines
  for an axis they know is coming; advancing the turn becomes deterministic, so the AI could search through it exactly.
  Needs the queue stored somewhere clients and the AI can read (with the chosen approach: alongside the replicated
  rule set on the GameState).

## Resolution

<Filled when status moves to answered / parked.>
