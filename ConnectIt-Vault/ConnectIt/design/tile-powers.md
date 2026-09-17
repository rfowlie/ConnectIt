# Tiles With Powers

Potential feature: give individual tiles an activatable **power** — instead of (or
alongside) just scoring points, a tile can trigger an effect on the board once it's been
scored on enough. The multiplier already tracks "how many times has a line been completed
through this tile" for free; a power just gives that number a second job.

## Why this fits what already exists

Every tile already has a `Multiplier` (`FConnectItTileData::Multiplier`) that increments by
1.0 every time a scoring line runs through it (`UConnectIt_LineScoringRule::ApplyScoringLine`
— every tile in a completed line gets `+1.0` multiplier, not just the piece that completed
it). That's already a natural, board-legible "charge" counter with zero new state needed —
a power is just: **once this tile's multiplier crosses some threshold, the *next* time a
line scores through it, something else happens too.**

The Mediator's request vocabulary already has most of the building blocks a power would
need to actually *do* something, without new plumbing:

| Effect a power wants | Existing request type |
|---|---|
| Change a tile's multiplier | `HandleDestroyTileMultiplierRequest` (resets to 1.0) — lowering/raising by an arbitrary amount would need a small extension, not a new mechanism |
| Remove a piece from the board | `HandleRemovePieceRequest` |
| Flip a piece's ownership | `HandleCapturePieceRequest` |
| Toggle a tile in/out of play | `HandleToggleTileActiveRequest` |
| Place a piece somewhere | `HandleForcePlacePieceRequest` (bypasses the normal "must be empty and active" check) |

A power is mostly a **trigger condition + a target selector**, wired to server-side
operations that mostly already exist.

## Open design questions

Not settled — flagging so an implementation doesn't have to rediscover these:

- **Timing.** `ApplyScoringLine` clears every non-completing piece in the line *and*
  increments multiplier in the same pass. A tile can cross a power's threshold in the exact
  moment its own piece is cleared. Does the power still fire for a tile that's now empty?
  (Probably yes — the power is about the tile "activating," not about what's currently on
  it.)
- **Who triggers it.** Does only the owning faction's line-completion trigger a tile's
  power, or does *anyone* scoring through it set it off — including an opponent, possibly
  against their own interest?
- **Consent / telegraphing.** Automatic on threshold-cross, or does the board need to
  visibly warn a charged tile is about to trigger before it does? A poison tile that just
  silently detonates the first time you're not paying attention reads very differently from
  one that's been visibly smoldering for two turns.
- **Cascades.** Can one power's effect (e.g. lowering a neighbor's multiplier, or raising
  one past its own threshold) trigger a second power in the same resolution? Chain
  reactions are a natural fit for this system but need an explicit stopping condition
  (max cascade depth, or "each tile can only trigger once per scoring event") so they can't
  loop.
- **Floor/ceiling.** Multiplier changes from a power (poison's `-2`, say) need a defined
  floor — does a tile's multiplier clamp at some minimum (1.0, matching its starting
  value?), or can it go negative / reduce future scoring below the base?
- **Stacking with normal scoring.** Does the power replace that tile's point contribution
  for the completing line, or fire *alongside* normal scoring, additively?

## Example powers

Sketches, not finished designs — each names a trigger, a target, and an effect, using the
threshold/target/effect shape above.

### Poison

- **Trigger:** Multiplier 3+.
- **Target:** The 4 diagonal neighbors.
- **Effect:** Each diagonal neighbor's multiplier drops by 2.
- **Notes:** The given example. A pure denial tool — punishes the opponent (or you) for
  building up a tile without immediately capitalizing on it, and makes diagonal
  neighborhoods of a poison tile actively risky to invest multiplier into. Interacts
  sharply with Board Shift: shifting a poison tile relocates its threat zone, and shifting
  one of its victims out of diagonal range is a legitimate counterplay.

### Magnet

- **Trigger:** Multiplier 3+.
- **Target:** The nearest occupied tile in each of the 4 orthogonal directions.
- **Effect:** Pulls each one exactly one tile closer to the magnet (a single-step,
  localized version of Board Shift, centered on this tile rather than a whole line).
- **Notes:** Double-edged the same way SWAP is — it can drag your own scattered pieces
  into a tighter formation, or drag an opponent's pieces into alignment they didn't ask
  for. Needs a defined behavior for "nothing occupied in that direction" (no-op for that
  direction, not an error).

### Overgrowth

- **Trigger:** Multiplier 4+.
- **Target:** The 4 orthogonal neighbors.
- **Effect:** Each orthogonal neighbor's multiplier rises by 1.
- **Notes:** Poison's positive mirror — rewards building around a hotspot instead of
  punishing it. Needs a multiplier ceiling somewhere, or a well-fed Overgrowth tile plus
  its neighbors can snowball into disproportionate late-game scoring. Probably the power
  most worth prototyping first, since it's the simplest to reason about and to balance.

### Crumble

- **Trigger:** Multiplier 5+ (deliberately the highest threshold — a late-game, rare
  event).
- **Target:** Itself.
- **Effect:** The tile deactivates (`bIsActive = false`) and, if occupied, its piece is
  removed. The tile leaves play — not scored on again, not selectable — until something
  else reactivates it, if anything ever does.
- **Notes:** A "burn it down" mechanic with real tension: scoring on this tile repeatedly
  is profitable in the short term but eventually shrinks the board and denies the position
  to everyone, including whoever kept feeding it. Opponents get a real, visible reason to
  deliberately avoid completing lines through a tile they can see is close to crumbling —
  the first power sketch here that gives a player an incentive to *not* score somewhere.

### Siphon

- **Trigger:** Multiplier 3+.
- **Target:** The farthest occupied enemy-owned tile along the same row.
- **Effect:** Captures that piece — flips it to the triggering faction (same operation as
  the Capture action).
- **Notes:** A direct steal, no repositioning involved. Needs a defined no-op for "no
  enemy piece in that row" (nothing happens, doesn't fail loudly). Strongest of the five as
  written — a straight ownership flip with no cost to the triggering side — worth
  reconsidering the threshold or adding a cost/cooldown if it turns out to dominate
  playtesting.
