# Chained tiles

Design note (2026-10-22), owner's idea; **nothing decided beyond the idea, nothing built**. First use: teaching "some
tiles aren't placeable" in the first adventure level
([adventure-modules-and-progression](adventure-modules-and-progression.md), Level 1 draft).

## The idea (owner)

A **chained tile** always has a **partner tile**. While the partner holds no piece, the chained tile is chained: no piece
can be placed on it. A piece placed on the partner **unchains** it. If the piece leaves the partner, the chained tile is
**chained again** -- and any piece on it is destroyed.

## How it maps onto what exists (Claude)

- **A sibling of [switch and pole tiles](switch-and-pole-tiles.md)**: one tile's occupancy controls another tile. The same
  two lessons apply:
  - **Link by group tag, not by position.** Board Shift moves tile data, so a partner stored as a position breaks after a
    shift; a shared tag on the chained tile and its partner survives moves (and allows one partner for several chained
    tiles, or the reverse, later).
  - **Model it as derived state, not as reactions to events.** "A chained tile is chained exactly when its partner has no
    piece" -- one rule, re-checked after every change, idempotent.
- **Why derived state matters here** -- two cases where reacting to Enter / Exit events gets it wrong:
  - **Capture** counted as Exit + Enter at the time (tile-power round 7). As events: the partner's piece "leaves"
    (re-chain, destroy the chained tile's piece), then "arrives" (unchain) -- the piece is lost for nothing. As derived
    state: the partner never stopped being occupied, nothing happens. *(Owner, 2026-10-22: this is why a capture is now
    a **Piece Change**, its own trigger, not Exit + Enter -- see [tile-power-triggers](tile-power-triggers.md).)*
  - **Scoring clears the partner's piece**, which is *not* an Exit (tile-power decision). As events the chained tile would
    stay open forever. As derived state it re-chains in the next settle step (destroying its piece).
- **One power, one trigger** (tile-power decision) doesn't fit either: the partner needs both "on enter" and "on exit".
  Derived state sidesteps that: a **level-wide rule** ("sync chains") checked in the settle step whenever something
  changed, like Crumble but on occupancy.
- **It needs the tile lock** -- the "not placeable" state; designed in [tile-lock](tile-lock.md) (2026-10-22). A chained
  tile is a locked tile whose lock is driven by its partner. So: design the lock state first (a tile data flag checked by
  `IsTilePlaceable`, replicated, readable by visuals), then chains as the rule that sets it.
- **Events**: `TileChained` / `TileUnchained` {Position, Partner}, and the destroyed piece as an ordinary `Piece Removed`
  (so it counts as a Tile Exit for tile powers, and the display board applies it).

## Timing

Chains are checked in the **settle (pre-scoring) step**, so:

- Placing on the partner unchains the tile before scoring -- the chained tile is still empty, so nothing changes for this
  move's lines.
- A piece leaving the partner (moved, swapped, removed) re-chains the tile and destroys its piece **before** scoring --
  so it can break a line that would otherwise have scored. That's the interesting part for puzzles.
- If scoring clears the partner, the chained tile re-chains in the settle step after scoring (next pass).

## Decided (owner, 2026-10-22)

- **Scoring clearing the partner's piece re-chains the tile** and destroys any piece on it -- the derived-state rule:
  chained whenever the partner has no piece, for any reason.
- **A capture on the partner changes nothing** (the piece only changes faction; Piece Change, not Exit + Enter).

## Open
- Can the **opponent** unchain your tile by placing on the partner, and does that destroy your piece later if their
  piece leaves? (Derived state: any piece on the partner holds it open, whoever's.)
- Is the destroyed piece's loss announced (a "chain snaps" visual) beyond Piece Removed?
- One partner → one chained tile, or many?
- Board Shift with chains: do chained tiles / partners shift (tags make it safe), or are they unshiftable anchors?
- The AI: as a rule in the after-move step it's seen by the search automatically.
