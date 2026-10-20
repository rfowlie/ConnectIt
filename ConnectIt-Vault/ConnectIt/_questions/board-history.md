---
created: 2026-10-07
question: "Should the board keep a history of past states or moves?"
status: parked
closed-by:
tags:
  - question
---

# Should the board keep a history of past states or moves?

## Why it matters

`UConnectIt_BoardStateComponent` keeps only the previous and current board (one replicated snapshot); everything
older is discarded. A history could serve replays, undo, post-game review or AI debugging, but nothing reads one
today.

## What would answer it

A concrete feature that needs history, which would also say what kind (states or moves), where it lives (server only
or every machine) and how long it is kept.

## Current thinking

Discussed 2026-10-07 (owner + Claude); decided not to build yet.

- **Memory is not the concern.** A 7x7 board state is roughly 1.5 KB, so 100 moves is about 150 KB; a 15x15 board
  over 200 moves is around a megabyte.
- **Do not replicate a growing history.** If clients need it, each keeps its own from the snapshots it receives (a
  client joining late would only have history from when it joined).
- **A move list is the better thing to keep**: the board operations applied, in order. It is far smaller, and any
  past state can be rebuilt by re-applying operations to the starting board, since operations and the resolve step
  are deterministic plain functions.
- It is cheap to add later: every board change already goes through one function
  (`UConnectIt_BoardRequestMediator::DispatchRequest` → `SetBoardState`).

## Resolution

Parked 2026-10-07: build when something needs it.
