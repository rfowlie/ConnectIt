---
Date: 2026-10-23
status: Active
superseded by:
tags:
  - architecture
  - board
  - tile-powers
  - tile-lock
---
## Decision

A **Chained tile** is locked exactly while its **partner** has no piece. It is a tile power, not a special case:

- **A Sync power:** `ConnectIt.TilePower.Chained` with `FConnectItTilePowerEffect_Chained`, a catalog entry like any
  power.
  - It checks the board on every settle pass. Partner has a piece and the tile is locked → unlock. No partner, or an
    empty one, and the tile is unlocked → remove its piece, then lock.
  - So it follows a partner emptied by anything, scoring included. A capture changes nothing.
- **One-sided, by tile id:**
  - Only the Chained tile names its partner. The partner can be any tile (it keeps its own power slot), and several
    Chained tiles may share one.
  - The link is the partner's stable tile id (`ChainPartnerId`), which survives shifts; a partner removed from the
    board reads as none.
  - Designers pick the partner on the builder tile; layouts carry it as a position; **Set Chain Partner** changes it
    mid-match.
- **The seeded board is settled once** under the match's rules, so chained tiles start in the state they keep, with
  no designer setting.

## Why

- **As a power:** the chain is "keep this true whatever happened", which is what Sync powers are. It needs nothing
  special in the pipeline, and the AI's search and Simulate Operation see it like any power.
- **State-based:** a partner cleared by scoring is not a Piece Exit, so a triggered power would miss it.
- **One-sided:** a link written on one tile can't disagree with itself (two tiles pointing at each other can). "No
  partner" is a valid, visible state (a phase can bring partners in later).
- **Tile id, not position:** a shift moves tiles; their ids move with them.

## What Would Change It

- A chain of chains (a Chained tile's partner is Chained) proving confusing or unstable -- warned about until tested.
- Several partners for one Chained tile.
- Search speed, if many Sync powers on large boards cost too much (each runs every settle pass).

Plan: [tile-lock-and-chained-tiles-plan](../design/tile-lock-and-chained-tiles-plan.md) Stage 2. Design:
[chained-tiles](../design/chained-tiles.md). Builds on
[tile powers in the after-move chain](2026-10-22-tile-powers-in-the-after-move-chain.md) and
[tile catalog and builder tiles](2026-10-22-tile-catalog-and-builder-tiles.md).
