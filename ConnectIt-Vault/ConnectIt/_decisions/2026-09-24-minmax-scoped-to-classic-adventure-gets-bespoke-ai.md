---
Date: 2026-09-24
status: Active
superseded by:
tags:
  - ai
  - minmax
  - adventure-mode
  - scope
---
## Decision

MinMax is **not** the AI approach for the whole game going forward. It's scoped to **Classic
ConnectIt** — defined here as the ruleset with no actions beyond PlacePiece — where it remains a
real, worthwhile build: a pure skill-based challenge, and the intended showcase of the owner's own
AI-building aptitude.

**Adventure-mode levels get bespoke, per-level AI controllers instead**, not an attempt at optimal
play. The goal there is an interesting or unique challenge suited to that level, not the strongest
possible opponent. What a "bespoke controller" actually is (one controller per level? a shared
toolkit levels configure from?) is not decided — that's the subject of the upcoming design/plan-mode
session, not this note.

## Why

Reviewing the existing MinMax code (`Source/ConnectIt/*/MinMax/`) against the loadout/action-config
work landed since 09-18 surfaced a real problem: every additional action explodes MinMax's search
space, throwing its viability into question once actions beyond PlacePiece exist — which is true for
every mode except Classic by definition. Rather than force one AI approach to cover both a
skill-tested ruleset and a designer-curated one, the two get split: MinMax stays where it fits
(small, well-defined action space, optimal play is the point), and adventure levels — which want
variety and designer control over difficulty/feel more than optimality — get their own thing.

This is the same self-triage pattern named at the [2026-09-18 check-in](../../Development/optimal-co-developer/_meetings/2026-09-18-check-in.md)
(separating "real and confirmed" from "wishlist/consider," unprompted), this time surfacing out of
discouragement over the old code's mess rather than mid-flow momentum.

## What Would Change It

If Adventure mode's bespoke controllers converge on a common enough shape, a shared toolkit could
retroactively look like "MinMax, generalized" after all — but that's evidence to gather, not to
assume now. See `_tasks/active.md`'s "Rebuild/rework the MinMax AI" row and the 2026-09-24 check-in
for the design session this scoping sets up.
