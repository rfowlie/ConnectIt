---
created: 2026-10-28
question: "How do we stop a tile actor class from being used for a power it doesn't look like?"
status: open
closed-by:
tags:
  - question
---

# How do we stop a tile actor class from being used for a power it doesn't look like?

## Why it matters

The tile catalog maps a power tag to an entry (a tile class + the power's values). Nothing stops one tile Blueprint
from being used as the class for several different powers -- e.g. the Push-looking tile entered under Vacuum -- which
would show players the wrong thing.

## What would answer it

A rule that ties a tile class to one power (e.g. the class declares the power tag it represents, and the catalog
validates against it), or a decision that this is left to designers and review.

## Current thinking

2026-10-28 (owner): left as a question for later; the priority is that each tag is automatically tied to the right
effect and only relevant values are exposed. See [editor-notes-2026-10-28](../design/editor-notes-2026-10-28.md).

2026-10-28 (built): the tag ↔ effect half is done. Each catalog entry's tag is its effect's
([tile-catalog-effects-plan](../design/tile-catalog-effects-plan.md)), so the remaining gap is only tile class ↔ effect.
That fits the same shape: the class could declare its power tag the way an effect does, and catalog validation could
compare the two.

## Resolution

