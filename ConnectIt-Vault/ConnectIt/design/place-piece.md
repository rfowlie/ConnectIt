# Place Piece

The default, mandatory action. Every turn, a player must place a piece on an empty,
active tile — this is the baseline move the rest of the action suite (SWAP, Board Shift)
either substitutes for or layers on top of.

## How it works

- Pick any tile that's **active** and **unoccupied**. Any tile satisfying both is fair
  game — no adjacency or pattern requirement.
- The piece is placed for the acting faction. If that completes a scoring line (see
  [tile-powers.md](tile-powers.md) for how the multiplier side of scoring works), scoring
  resolves immediately as part of the same move.
- Mandatory: the turn can't auto-end on Place Piece alone being skipped — it's paired with
  SWAP as an *alternate* requirement (completing either one satisfies the turn, not both).

## Gameplay ramifications

- **It's the pressure-release valve.** Every other action (SWAP, Board Shift) is optional
  or resource-limited; Place Piece is always available as the fallback that keeps a turn
  moveable. Any future action that also wants to be "mandatory but optional-ish" should
  reckon with how it interacts with this baseline, not just with SWAP.
- **No cost, no cooldown, no per-match limit** — unlike SWAP's use-budget. This makes it
  the "cheap" move, both mechanically and in player psychology: expect it to be the
  default choice whenever a player doesn't have a clear reason to spend SWAP or a shift.
- Completing a line **clears every piece in that line except the one that completed it**
  (see [tile-powers.md](tile-powers.md)) — so placing a piece can simultaneously score
  *and* empty out a chunk of the board. Worth remembering when reasoning about board
  density: the board doesn't just fill up turn over turn, it periodically clears itself
  wherever lines complete.
