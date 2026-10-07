---
Date: 2026-09-20
status: Partly superseded
superseded by: 2026-10-07-board-events-are-structs-carried-by-the-queue (shape of a step)
tags:
  - board
  - events
  - replication
  - crumble
---
## Decision

`FConnectItBoardChangeEvent` becomes an **ordered list of steps** (design only — see
[board-request-objects](../design/board-request-objects.md)):

- A step is a flat, Blueprint-readable struct:
  `{ EventTag, SubjectPositions[], RelatedPositions[], FactionA, FactionB, Value }`. The per-turn
  summary (`bGameWon`, points) stays separate.
- **Steps are semantic** — the caller (request or reaction) appends what happened
  (`PiecesSwapped`, `TileCrumbled`); primitives emit nothing.
- `EnqueueBoardEventTags` loops over the steps instead of an `if` ladder; ordering is data.
- **The queued game-event entry carries its step index**, so a Blueprint reactor reads its own
  step's data.
- **Steps describe every position they touched**; reactors must not read final board state for
  anything a step already says.

## Why

Today one turn = one concrete change; the reactor for a tag reads the single `ChangeEvent`. Crumble
lets one turn produce place → score → several crumbles, so a `TileCrumbled` reactor can't tell which
occurrence it is handling. Semantic steps match how the visuals are authored (crack → explosion →
lava is one idea, not "piece removed" plus "tile toggled"). An index through the queue is explicit;
a shared cursor or per-tag consumption could silently desync. Snapshot Previous/Current only
bracket the whole turn, so self-describing steps avoid the same bug class as
[scoring-line visuals](2026-09-18-scoring-line-visuals-must-cover-all-mutated-positions.md).
Intermediate states are deliberately not replicated.

## What Would Change It

If visuals need the board as it stood between steps, replicating intermediate states (or per-step
deltas) would be reconsidered. Migration is additive: dual-write steps beside the old bools, switch
the enqueue and reactors, then remove the bools.

## Update (2026-10-07)

Built. The ordered list, "the queued entry carries its own data" and "steps must describe every position they
touched" all hold. What changed: a step is a polymorphic event struct, not one flat generic struct. See
[board events are structs carried by the queue](2026-10-07-board-events-are-structs-carried-by-the-queue.md).
