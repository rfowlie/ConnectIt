---
Date: 2026-09-20
status: Active
superseded by:
tags:
  - board
  - crumble
  - replication
  - visuals
---
## Decision

The Crumble threshold is **stamped onto replicated board state** at board init, following the
`TargetScore` precedent, so clients can drive the crack visuals from state (tile multiplier vs
threshold). The explosion is an event step; the cracks are not.

Crumble itself is a per-level board reaction (an entry in `LevelConfig`'s `BoardReactions`), not a
tile-carried power — see [board-request-objects](../design/board-request-objects.md).

## Why

`UConnectIt_BoardRules` and its strategies are server-only, so a client visual can't read the
threshold from the reaction. `TargetScore` already solves the same problem: the rule stamps the
value into replicated state, and UI reads that instead of the server-only object. Cracks track a
continuously changing quantity, so deriving them from state avoids an event per multiplier change.

## What Would Change It

If several reactions each need client-visible parameters, a general "reaction parameters" block in
state (or reading the client-reachable level config) would replace per-value stamping. The
lava-vs-toggled-off tile state is a separate, still-open question.
