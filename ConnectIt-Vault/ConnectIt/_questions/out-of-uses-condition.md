---
created: 2026-10-20
question: "Do we need an 'Out of Uses: X' condition alongside 'No Legal Move: X'?"
status: open
closed-by:
tags:
  - question
---

# Do we need an "Out of Uses: X" condition alongside "No Legal Move: X"?

## Why it matters

*No Legal Move: X* (built 2026-10-20) looks only at the board: "is there any legal X here for this faction?". It does not
look at the loadout -- a faction whose swap action has no uses left still "has a legal swap" by that condition. A level
where running out of a numbered action should end the match (or be a goal) can't express that today.

## What would answer it

A level design that needs it. Building it means the condition needs data a plain condition can't read (the faction's
action state lives on its PlayerState), so the server would hand it over as plain data (e.g. per faction, per move kind,
"uses left"), and the AI's search wouldn't see it.

## Current thinking

2026-10-20: kept separate on purpose -- board-only conditions are safe everywhere (including the search). Temporary
unavailability (cooldowns, per-turn caps) is a **pass**, not a loss: the UI greys out actions with no viable use
(*Has Any Legal Move*), and the AI passes when nothing is legal. Revisit when a level asks for it.

## Resolution

<Filled when status moves to answered / parked.>
