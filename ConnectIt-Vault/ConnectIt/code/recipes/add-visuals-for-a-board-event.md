---
schema: recipes
task: Add visuals for a board event -- an event visual (one event kind), a board visualizer Blueprint (one per map, per look) or a tile / piece hook -- holding the event's handle while it animates, without ever touching the registries' bookkeeping.
touches:
  - Source/ConnectIt/Public/Board/ConnectIt_BoardVisualizer.h (the visualizer: one function per board event, the handle, the arriving / leaving actors)
  - Source/ConnectIt/Public/Board/ConnectIt_VisualHandle.h (Hold / Release)
  - Source/ConnectIt/Public/Board/Actor/ConnectIt_TileBase.h (tile hooks)
  - Source/ConnectIt/Public/Board/Actor/ConnectIt_PieceBase.h (piece hooks)
  - Source/ConnectIt/Public/Framework/Data/ConnectIt_LevelConfigDataAsset.h (Visualizer Class)
  - Source/ConnectIt/Public/Framework/Library/ConnectIt_BoardEventLibrary.h (the playing event's cause, an event copy's tag)
  - Source/ConnectIt/Public/Board/ConnectIt_EventVisual.h (one event kind's visuals, listed on the level config)
  - a Blueprint subclass of AConnectIt_BoardVisualizer, set on the level config
reconciled: 2026-10-30
commit: 075f269
status: 'current'
---

# Recipe: add visuals for a board event

## Goal

Something visible when a board event plays -- a piece sliding, a burst on a scored line, a banner on a phase change --
that the game waits for, on every machine. Design: [board-visualizer](../../design/board-visualizer.md); decisions:
[one visualizer per map](../../_decisions/2026-10-29-one-board-visualizer-per-map.md),
[registries own grid positions](../../_decisions/2026-10-30-registries-own-grid-positions.md).

## Prerequisites

- The map's level config. Its **Visualizer Class** is empty (the C++ base) until you make a Blueprint.
- How events play: [[board-state-single-source-of-truth]].

## Steps

1. **Pick the place** -- every hook and function gets a **copy of the event** and the **Handle**:
   - **On the actor:** a tile or piece hook, when the visual belongs to that kind of tile or piece and should show
     whatever caused it (e.g. a Push tile's push animation).
     - Tile hooks: On Tile Power Triggered, On Piece Enter / Exit, On Multiplier Changed, On Lock Changed, On Active
       Changed, On Chain Partner Changed.
     - Piece hooks: On Placed, On Moved, On Swapped, On Captured, On Removed, On Cleared By Score -- each with its
       typed event. A hook another event leads to gets one made for it (a starting piece's Piece Placed; a shifted
       piece's Piece Moved), its Cause Index that event.
     - On Piece Enter / Exit get the source event as an instanced struct (its tag: **Get Board Event Tag**) and the
       faction.
   - **One event kind, swapped in:** an **event visual** -- a Blueprint of that kind's class
     (`ConnectIt_EventVisual_PiecePlaced`...), implementing **On Play (Event, Handle)**, listed in the level config's
     **Event Visuals** (one per kind). It plays instead of the visualizer's function; call **Play Default** in On Play
     to run that too.
   - **On the visualizer:** an event function, for a visual that spans the board or belongs to the look rather than
     one actor (effects, a line burst, camera, banners). Make a Blueprint subclass of `ConnectIt_BoardVisualizer` and
     set it as the level config's **Visualizer Class**.
2. **Override the function** with its event struct and the **Handle**, e.g. **On Piece Moved (Event, Handle)**:
   - **call the parent** to keep the actors' own hooks and add to them;
   - or **don't**, to replace them. The registries are kept right either way; see Pitfalls.
3. **Hold while animating:** call **Handle > Hold** when the animation starts and **Handle > Release** when it ends
   (from a timeline's Finished, after a Delay, on an effect's end...). One Release per Hold. The event, and the game
   behind it, waits for every hold. Nothing held means it moves on at once.
4. **Use what the visualizer gives you** while the event plays:
   - **Get Tile At / Get Piece At** (typed, no casts);
   - **Get Arriving Pieces** (just placed or spawned, already mapped and placed);
   - **Get Leaving Pieces / Get Leaving Tiles** (no longer in the registry; returned to the pool or destroyed once the
     visuals are done, so animate them out);
   - **Get Piece Location** (a tile's location plus **Piece Offset**).
5. **Why it happened:** **Get Board Event Cause** (or **Get Board Event Cause Tag**), e.g. to play a different move when
   a tile power caused it. **Get Active Board Event Index** gives the playing event's place in its change.
6. **A new kind of event** (a new event struct) gets its own function on the visualizer, plus its `Dispatch` line.
   Until then it goes to **On Board Event (Payload, Handle)**. The test `ConnectIt.Visuals.Events` counts the kinds.

## Verify

- PIE: the event plays your visual and the next one waits for it.
- Override **On Piece Moved** without calling the parent: the piece still ends on the right tile.
- Forget a Release: the Output Log names the holder after **Hold Timeout Seconds** (10 by default), and the game goes
  on.
- A 2-player client plays the same visuals (the visualizer is local to each machine).
- Leave a piece off its tile on purpose: one `reconcile --` error naming it, and it's put back.
- Tests: `ConnectIt.Visuals.HookDefaults`, `.Reconcile`, `.EventVisuals`; `ConnectIt.Board.PiecePositionCache`.

## Pitfalls

- **Don't set positions.** A piece's **Get Grid Position** is the registry's (it writes it when it maps the piece);
  an `InstantiatePiece` override only makes the piece.
- **Reconcile catches what a visual leaves wrong:** after each event (and each played-out change) every actor it named
  is checked against the registries -- off its cell / tile, or a stale position -- with an error naming it, and put
  right (`ConnectIt.Visualizer.Reconcile` 0 off / 1 log / 2 log and fix). An error there is a visual to fix (often an
  animation that didn't Hold). A piece's pooled activation animation (`ActivatePoolObject`) isn't held by any handle:
  keep it in place, or animate in On Placed with a Hold.
- **Don't do registry bookkeeping in Blueprint.** The visualizer already does it in C++ before your function runs:
  spawning pieces, re-keying moves / swaps / shifts, unmapping removed pieces and tiles, spawning added tiles. It
  finishes after the visuals: releases pieces, destroys tiles, snaps moved actors onto their cells. Doing it again
  double-moves things.
- **Hold before the first latent node.** A Hold after a Delay comes too late: the event may already be done.
- A piece's resting height is **Piece Offset** on the visualizer: the actor is placed there once the visuals finish.
- Timed visuals go in hooks and functions. The resting look (multiplier text, lock state) goes in **On Refreshed From
  Board**, which follows the display board.

## See also

- [[board-state-single-source-of-truth]], [[tile-power-chain]], [[add-a-tile-power]], [[add-a-board-request-type]]
