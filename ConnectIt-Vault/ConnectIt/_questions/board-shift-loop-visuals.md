---
created: 2026-09-17
question: "How should tiles that wrap around the board during a shift be identified and visually distinguished?"
status: open
closed-by:
tags:
  - question
---

# How should tiles that wrap around the board during a shift be identified and visually distinguished?

## Why it matters

A board-shift rotation (`UConnectIt_BoardRequestMediator::HandleBoardShiftRequest`) moves
the tile at the last shiftable slot in a line back to the first shiftable slot — a wrap,
not a step. Every other moved entry in `ShiftStartPositions`/`ShiftEndPositions` just
advances one slot in `ShiftDirection`. Nothing today distinguishes the wrap entry from a
normal one, and a straight lerp (`BuildRaisedShiftPath`) between a wrap entry's start/end
would visually fly the tile across the entire board through the middle instead of reading
as "this one went around the edge."

## What would answer it

- **Identification:** a way to know, per moved tile, whether it wrapped — likely computed
  in the same Mediator rotation pass that already builds `ShiftStartPositions`/
  `ShiftEndPositions` (it already knows which entry is the last-slot-to-first-slot case),
  exposed on `FConnectItBoardChangeEvent` as either a parallel `TArray<bool>` or a
  `TArray<FGridPosition> LoopedPositions` subset, so Blueprint-side visuals don't have to
  re-derive it from position math.
- **Visual treatment:** candidate approaches discussed, cheapest first —
  1. Teleport-with-fade: shrink/fade out at the exit edge, snap, grow/fade in at the entry
     edge.
  2. Portal pair: a brief VFX/flash at both the exit and entry edge tiles.
  3. Off-board arc: path bulges outward past the board boundary and curves back in on the
     opposite side — most literal, needs a path builder that knows board bounds.

## Current thinking

Not designed yet — flagged during a design chat, no direction chosen. Leaning toward
option 1 (teleport-with-fade) as the cheapest first pass if this gets picked up, since it's
a trivial Blueprint branch off the identification data rather than new VFX work, but
nothing is committed.
