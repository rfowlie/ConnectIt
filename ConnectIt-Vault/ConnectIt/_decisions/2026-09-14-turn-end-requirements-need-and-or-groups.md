---
Date: 2026-09-14
status: Active
superseded by:
tags:
  - turn-end
  - actionscomponent
  - requirements
---

## Decision

`UConnectIt_TurnBasedActionsComponent::CanAutoEndTurn_Implementation`'s current
hardcoded two-tag OR pair (`RequiredActionTagA`/`RequiredActionTagB`, used for SWAP's
alternate-turn-end logic) needs to generalize into a configurable structure that supports
both **OR** (any one of a group of tags satisfies turn-end, today's SWAP case) and **AND**
(all tags in a group must be satisfied) — an array of requirement groups, each with its
own any/all mode, not just a bigger fixed set of slots.

The specific data shape and implementation are **not yet designed** — this decision
records the direction (must support both AND and OR, structurally, not as two more
hardcoded fields) and the concrete need that's driving it, not the final form.

## Why

A real, concrete requirement surfaced the gap: a planned adventure-mode level where a
player must make a board shift **and** place a piece to complete their turn each turn —
an AND requirement, which the current OR-only two-slot structure can't express at all
without new hardcoded fields per level. Rather than adding a third hardcoded slot (or a
second OR pair) the next time a new combination is needed, the structure itself should be
shaped for this kind of customization from the start.

This is deliberately being recorded now, while only the *need* is known, so the eventual
implementation doesn't get silently re-argued or re-discovered from scratch — but the
actual group/array design is explicitly deferred until the Board Shift action exists and
its exact requirements are known (see [2026-09-14 check-in meeting](../../Development/optimal-co-developer/_meetings/2026-09-14-check-in.md)).

## What Would Change It

- Not yet implemented — today's `RequiredActionTagA`/`RequiredActionTagB` OR pair is still
  what ships.
- The actual generalized shape (array of `{Tags, Mode: Any|All}` groups, or something
  else) is still open — to be settled once Board Shift's real requirements are in hand,
  not invented ahead of a second concrete case.
- If it turns out every future case is expressible as OR-of-ANDs or AND-of-ORs (nested),
  a flat array of any/all groups might not be enough — worth checking once Board Shift and
  the adventure-mode level are both real.
