---
created: 2026-10-20
question: "What should lose conditions do in an online (versus) match with no AI?"
status: open
closed-by:
tags:
  - question
---

# What should lose conditions do in an online (versus) match with no AI?

## Why it matters

Lose conditions are being designed per side, owned by the Player director and the AI director
([three-directors](../design/three-directors.md)). In a match against the AI the sides are clear. Online, both factions are
human and today play the same win goals (the player's list applies to every faction), so it's open whether they should
also share lose conditions, have none, or get per-slot ones.

## What would answer it

A design call for versus play: does any versus mode need "you lose when stuck" (e.g. an online swap mode), and should both
players be held to the same rule?

## Current thinking

2026-10-20: the owner left it **undecided**. Until answered, lose conditions apply **only in matches with an AI**; a versus
match ignores them (with a warning if a preset sets any). The natural default if versus needs them: both players share the
Player director's lose conditions, as they already share its win goals.

## Resolution

<Filled when status moves to answered / parked.>
