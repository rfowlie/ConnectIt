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

2026-10-24 (owner): in a puzzle level, **running out of uses without meeting the goals is a loss**. Claude: rather than
a separate "Out of Uses" condition, this folds into the **stuck board** rule ([full-board-outcome](full-board-outcome.md)):
if "can make a move" counts uses left and the item budget, a player who has run out can't move; with the opponent unable
to move too (or absent), the board is stuck and the level's stuck outcome (usually a loss) applies. Checked by the server
(uses live on the PlayerState), so the AI's search doesn't see it. Next: the same small plan.

2026-10-24, later (owner, clarifying): make it a **No Moves** condition for puzzle levels -- **the player has used up
all their moves and hasn't won, so they lose.** A per-side **lose condition** a level lists (not the board-wide stuck
rule): it holds when that side can't make any move with what it has left (permanent action uses, item budget and items,
and whether any of those has a legal instance on the board). Stuck board stays the separate, every-level rule for "no
participant can move". Checked on the server (uses live on the PlayerState); the AI's search doesn't see it.

2026-10-24, round 4 (owner): the puzzle case is **Stuck Player** (no unlimited action, every available action's match
uses spent) as a lose condition -- not No Moves, which now means "can't move *this turn*, force-end the turn". A
**Turns** condition (reach turn N → lose) gets the same result more simply for single-action puzzles. Flow and
definitions: [adventure-mode-roadmap](../design/adventure-mode-roadmap.md), round 4.

## Resolution

<Filled when status moves to answered / parked.>
