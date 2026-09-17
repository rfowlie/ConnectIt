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
