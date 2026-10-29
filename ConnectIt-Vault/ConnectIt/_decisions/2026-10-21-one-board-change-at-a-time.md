---
Date: 2026-10-21
status: Active
superseded by:
tags:
  - architecture
  - networking
  - board
summary: 'One board change at a time: the server''s GameMode locks from a change until its visuals have played out (a completion marker on the event queue); requests are refused until the board settles, level changes queue and play one by one, the AI decides only from a settled board; the GameState mirrors "busy" for UI'
---
## Decision

**One board change at a time.** A board change (a player or AI move, or a level operation) resolves from the moment it
is carried out until its visuals have played out. Until it has, and while level changes wait to play, nothing else
changes the board.

- **The lock is ConnectIt-side**, on the server's GameMode, not a new plugin turn phase: the turn stays active and only
  the board is busy.
- **"Visuals done" = the server's own event queue.** `SetBoardState` queues the `ConnectIt.Event.BoardChangeComplete`
  marker behind the change's events; when it completes, the lock lifts.
- **Requests are refused** until the board is settled (`AConnectIt_GameMode::IsBoardSettled`), and the requester's
  action re-enables. The GameState mirrors "busy" (`IsBoardChangeResolving`) so clients don't send requests and UI can
  block input.
- **Level changes queue, never refused.** Immediately = once the current change has played out. Several play one after
  another, each its own change with its own visuals. Turn-start changes join the same line.
- **The AI decides only from a settled board.** It decides again if the board changed while it thought.
- **A safety timeout** (`BoardChangeTimeoutSeconds`, 15 s) logs what is stuck and lifts the lock.

## Why

The board snapshot is one replicated property. Two changes closer together than one network update reach a client as
one snapshot, and the first change's events are lost: the board jumps, and its animations never play. This happened
with an Immediately level reaction to a move, with several turn-start changes, and with an AI making two moves in a
turn. The owner's rule: a second change while one is resolving is an error, not something to merge. Waiting for the
visuals also gives each change room on screen.

## What Would Change It

- **Online play with remote clients' pacing mattering:** each client confirms its playback finished (with a timeout)
  before the lock lifts.
- **A dedicated server:** it has no visuals, so the lock lifts at once and close changes could merge again. Merging
  same-moment changes into one snapshot is the fallback.
- **Gameplay that needs simultaneous changes** (e.g. both sides acting in one tick).

Refines the Immediately timing of
[2026-10-21-director-hub-and-level-changes](2026-10-21-director-hub-and-level-changes.md). Plan:
[one-board-change-at-a-time-plan](../design/one-board-change-at-a-time-plan.md).
