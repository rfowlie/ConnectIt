# Board Shift

Pick a tile, and the whole row, column, or diagonal line running through it rotates one
step in the chosen direction — wrapping the far end around to the near end. Currently
unrestricted: no resource cost, no faction check, and not yet wired into any turn-end
requirement.

## How it works

- Pick a tile; the direction (Up/Down/Left/Right, or one of the 4 diagonals) is fixed per
  action instance, not chosen at selection time — a loadout would need a separate Board
  Shift instance per direction it wants to offer.
- Every tile sharing that line (same row, column, or diagonal axis) gets collected, sorted
  in order along the shift direction, then **whole tile data** — piece, multiplier,
  active-state, all of it — rotates one step along that order. The far end's data wraps
  around to the near end. No piece is ever removed from the board by a shift, only moved.
- A tile can opt out via `bCanShift = false` — it stays exactly where it is and is skipped
  entirely; the rest of the line rotates around it as if it weren't part of the line.
- Diagonal shifts work the same as orthogonal ones — the line-collection logic doesn't
  distinguish axis shape, so a diagonal shift is just as valid (and just as disruptive) as
  a row or column shift.
- Scoring is re-checked afterward for every occupied tile in the *original* line, including
  ones that didn't move (an unshiftable tile's occupant can still newly complete a line
  thanks to its neighbors relocating around it).

## Gameplay ramifications

- **Currently free and repeatable** — no use-budget like SWAP, not yet part of the
  turn-end requirement set. As designed today, nothing stops a loadout offering multiple
  Board Shift instances (one per direction) from letting a player shift several different
  lines in a single turn. Worth a deliberate call before shipping: should shifting cost
  something, or be turn-limited across all directions combined, the way SWAP is
  budget-limited?
- **Nothing is ever destroyed by a shift** — pieces only relocate within their line, wrapping
  rather than falling off. This is a real design invariant, not an incidental detail: it
  means a piece placed near a board edge is never at risk of being shifted *off* the board,
  only cycled back around into play. Board Shift is a repositioning tool, never a removal
  tool.
- **Diagonal shifts are meaningfully more disruptive** than orthogonal ones — they cut
  across both axes at once, touching a set of tiles a player is less likely to be tracking
  compared to "my row" or "my column." Worth deciding whether diagonals should be offered
  at all in early levels, or reserved for later/harder ones.
- **`bCanShift = false` tiles create fixed anchors** a shift rotates around — useful for
  level/puzzle design (an obstacle a player has to work with rather than around), but also
  a real clarity question: does the board visually distinguish a shift-locked tile from a
  normal one *before* a player commits to a shift? Without that, "why didn't tile X move"
  reads as a bug, not a rule.
- Because whole tile data (not just the piece) travels with a shift, a multiplier tile
  physically relocates when its line is shifted — see [tile-powers.md](tile-powers.md) for
  why that matters once tiles can carry activatable powers, not just a scoring bonus.

## Wishlist — maps with holes (added 2026-09-18, not scheduled)

Some planned maps will have gaps or holes in the grid. A hole is not one thing, and Board
Shift will need to tell two kinds apart:

- **Fillable holes** — a shift can carry tile data across or into the gap (the line
  effectively closes over it, or the gap travels with the rotation).
- **Fixed holes** — a shift can't move anything into, out of, or across the gap. Closest
  existing analogue is `bCanShift = false`, but that's a property of a *tile that exists*;
  a hole has no tile at all, so it needs its own representation.

What it would touch when picked up:

- **Line collection** (`GetTilesByDirection`) currently tolerates gaps in the walk but
  treats every registered tile as rotatable; it would need to distinguish the two hole
  kinds while collecting.
- **The Mediator's rotation** (`HandleBoardShiftRequest`) rotates the whole ordered
  `Positions` array with wrap-around. A fixed hole mid-line has to split or skip the
  rotation (like `bCanShift = false` does today), and a fillable hole has to define
  whether the gap itself moves.
- **The "nothing is ever destroyed" invariant** above must survive either hole kind.
- **Clarity**: same open question as `bCanShift = false` — the board should show a player
  which holes a shift can and can't use before they commit.

Open: how holes are represented in `FConnectItBoardState` (absent tile vs. a tile flagged
as a hole), and whether fillability is per-hole or per-map. Wishlist only — no design
decision made.
