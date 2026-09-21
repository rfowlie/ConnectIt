---
Date: 2026-09-20
status: Active
superseded by:
tags:
  - board
  - mediator
  - architecture
  - crumble
---
## Decision

Board changes are restructured into layers (design only — nothing built yet; see
[board-request-objects](../design/board-request-objects.md)):

- **Primitives** — small pure state mutations (remove piece, set tile active, set multiplier, set
  piece owner). They emit no events.
- **Request objects** — one per action, paired **1:1 by class** (the action holds
  `TSubclassOf<RequestObject>`, tag derived from the CDO). Each declares its payload, validates, and
  composes primitives. The mediator instantiates them from the loadouts.
- **One shared pipeline in the mediator** — validate → mutate → score touched occupied positions by
  current occupant → reactions → win check → finalize event → `SetBoardState`.
- **Reactions** — an ordered `BoardReactions` array on `LevelConfig` (empty by default, so a mechanic
  like Crumble is on or off per level). They call the same primitives and run to a fixpoint with a
  depth cap.

## Why

The seven `Handle*Request` methods repeat one tail, so a new board-wide step (Crumble between
scoring and the win check) would need adding to five handlers — the misalignment risk the owner
flagged. Dispatch and `ProducesRequestType` are two hardcoded places for the same pairing, and the
actions already disagree on how `RequestType` is authored. `ProducesRequestType` is 1:1 in practice
(each action returns exactly one tag), and five request types have no action at all — they are
already primitive-shaped, so making them primitives is a promotion, not a rewrite. Mirroring how
actions are configured keeps one idiom.

## What Would Change It

An action that genuinely produces several request types would need a list on the action instead of a
single class. If primitives prove too fine-grained for a real request, a request may mutate state
directly for that case — but reactions should still go through primitives.
