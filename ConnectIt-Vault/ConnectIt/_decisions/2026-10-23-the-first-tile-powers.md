---
Date: 2026-10-23
status: Active
superseded by:
tags:
  - architecture
  - board
  - tile-powers
summary: 'The first powers: Move Piece and Adjust Multiplier operations; multiplier floor and ceiling per tile type (gate renamed FireAtMultiplier); trigger sets; a Turn Start countdown trigger committed as its own change at turn start; Push, Vacuum, Repel pick at random among moves the rules allow; tile hooks with reaction handles the visualizer waits on (no wrapper)'
---
## Decision

The first real tile powers (Push, Vacuum, Repel) and the pieces they needed:

- **Move Piece** `{From, To}`: a new board operation that keeps the piece's owner.
  - It is a Piece Exit at From plus a Piece Enter at To. To must be empty and active, and neither tile locked.
  - Event **Piece Moved**, so visuals slide the piece.
- **Adjust Multiplier** `{Position, Delta}`: a new board operation, with event **Multiplier Changed**.
- **Multiplier floor and ceiling per tile type,** on the catalog entry's power (`MultiplierFloor`, `MultiplierCeiling`).
  - Plain tiles: floor 1, no ceiling.
  - Applied wherever a multiplier changes: scoring's +1, Adjust, Destroy's reset and a tile's starting value.
  - A ceiling-0 tile still forms lines and adds 0 points.
  - The gate is renamed `FireAtMultiplier`, so it isn't confused with the floor.
- **A power has a set of triggers** (`Triggers`, a bitmask), not one. Sync stays alone.
- **Turn Start trigger:** a countdown.
  - At turn start the GameMode commits a **Turn Start** change first, for the faction whose turn it is. It does so only
    when a Turn Start power is ready.
  - That change's settle fires the Turn Start powers once, before Sync and the triggered powers.
  - The countdown is the cooldown, over all turns. It starts when the tile joins the board, and it resets even with
    nothing to do.
  - The AI's search doesn't run it (for now).
- **The three powers:**
  - **Push** (Scoring): pushes one orthogonal neighbour one tile away, picked at random among those that can move.
    Never its own piece.
  - **Vacuum** (Piece Exit and Scoring): when its tile is empty, pulls the nearest piece along one of the 8
    directions, picked at random.
  - **Repel** (Turn Start): moves the piece on its tile to a random open tile among the 8 around it.
  - Each builds only moves the rules allow (`CanCarryOut`, the lock included) and rolls from the board's seeded stream.
- **Tile hooks with reaction handles:**
  - `AConnectIt_TileBase` has one `BlueprintNativeEvent` per kind of tile reaction, each passed a
    `UConnectIt_TileReaction`.
  - `UConnectIt_TileVisualizer` calls the hooks when the events play and holds the event queue until every reaction
    has called **Finish**.
  - No wrapper. A hook that overrides must call Finish, and the C++ defaults finish at once. A tile returned to the
    pool finishes what it holds.

## Why

- **Move Piece and Adjust Multiplier are ordinary operations**, so powers, the Level director and future actions share
  them, and the lock check covers them.
- **Limits belong to the tile type:** some powered tiles shouldn't gain multiplier or add to scores. A catalog entry is
  the one place a tile type is defined.
- **Trigger sets:** Vacuum needs two triggers. A set keeps one power one definition.
- **Turn Start as its own board change:** Repel's move plays out, and can score, before the player decides. It doesn't
  blur into the previous move's chain, and one board change still runs at a time.
- **Candidate lists before the roll:** a player who sees one valid option can predict it, and a power acts whenever it
  can.
- **Reaction handles, not a wrapper:** a Blueprint event returns before its animation ends, so a wrapper can't know
  when it ends. One handle per hook call means one event over several tiles waits for all of them. Waiting keeps a
  chain playing in order.

## What Would Change It

- The AI losing to Repel because its search can't see Turn Start powers: run them in the search.
- Designers forgetting Finish often enough to stall the queue: revisit a safety timeout or an opt-in hold.
- A power needing a faction other than whose turn it is at turn start (Repel's faction is a placeholder, owner).
- Powers gaining levels (tiles growing in power).

Plan: [first-tile-powers-plan](../design/first-tile-powers-plan.md). Design rounds:
[tile-powers-in-the-board-pipeline](../_questions/tile-powers-in-the-board-pipeline.md) (Stage 5 rounds 1-5). Builds on
[tile powers in the after-move chain](2026-10-22-tile-powers-in-the-after-move-chain.md),
[tile catalog and builder tiles](2026-10-22-tile-catalog-and-builder-tiles.md) and
[the tile lock](2026-10-23-tile-lock-per-operation-rule.md).
