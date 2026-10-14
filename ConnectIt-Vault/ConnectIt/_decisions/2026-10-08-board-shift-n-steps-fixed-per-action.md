---
Date: 2026-10-08
status: Active
superseded by:
tags:
  - board-shift
  - actions
  - balance
---
## Decision

A Board Shift moves its line **`Steps` places**, not always one. `FConnectItBoardOperation_Shift::Steps` (default 1)
travels with the request; `UConnectIt_BoardShiftAction::ShiftSteps` (EditDefaultsOnly) is the value an action sends.

- **Fixed per action, not chosen by the player.** A loadout offers "Shift 2" as its own action class. The server
  rejects any shift whose `Steps` differs from the sending action's `ShiftSteps`.
- **Counted in shiftable tiles.** Pinned tiles are jumped over, not counted — as a one-step shift already treated them.
- **One use** whatever N is. Balance comes from the N a loadout grants.
- **A full lap is refused** (`Steps` a multiple of the shiftable count moves nothing); `Steps < 1` is refused.
- **One slide.** Tiles go straight from start to end; the loop-visuals question stays open. `Steps` is on the
  Board Shifted event for visuals to use.
- **Scoring unchanged:** the shared `ResolveBoardChange`. Making it snapshot-based is a separate, rules-wide decision.

## Why

The rotation was one line (`Index - 1`) and the pieces of the system that matter — the operation as request payload,
piece-registry re-keying from the event's start/end pairs, the shared scoring step — already worked for any distance.
Fixing N per action keeps the first slice small (no picker UI, no preview) and makes balance a data choice in the
loadout. Counting in shiftable space keeps pinned tiles meaning the same thing they did, and gives the planned
maps-with-holes work one rule to extend. The Steps check exists because every operation field is client-supplied: a
fixed-per-action design is only real if the server enforces it.

## What Would Change It

Wanting the player to choose the distance (then `Steps` is validated against a per-action maximum instead of equality,
and a picker + destination preview are needed); wanting a large shift to cost more (the gate spends one use per
request); or the loop-visuals work deciding shifts animate as N sub-steps.
