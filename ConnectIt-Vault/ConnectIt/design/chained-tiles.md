# Chained tiles

Design note (2026-10-22), owner's idea; **nothing built**. Plan: [tile-lock-and-chained-tiles-plan](tile-lock-and-chained-tiles-plan.md) (approved 2026-10-22, not built). First use: teaching "some
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
    a **Piece Changed**, its own trigger, not Exit + Enter -- see [tile-power-triggers](tile-power-triggers.md).)*
  - **Scoring clears the partner's piece**, which is *not* an Exit (tile-power decision). As events the chained tile would
    stay open forever. As derived state it re-chains in the next settle step (destroying its piece).
- **One power, one trigger** (tile-power decision) doesn't fit either: the partner needs both "on enter" and "on exit".
  Derived state sidesteps that: a **level-wide rule** ("sync chains") checked in the settle step whenever something
  changed, like Crumble but on occupancy.
- **It needs the tile lock** -- the "not placeable" state; designed in [tile-lock](tile-lock.md) (2026-10-22). A chained
  tile is a locked tile whose lock is driven by its partner. So: design the lock state first (a tile data flag checked by
  `IsTilePlaceable`, replicated, readable by visuals), then chains as the rule that sets it.
- **Events**: `TileChained` / `TileUnchained` {Position, Partner}, and the destroyed piece as an ordinary `Piece Removed`
  (so it counts as a Piece Exit for tile powers, and the display board applies it).

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
- **A capture on the partner changes nothing** (the piece only changes faction; Piece Changed, not Exit + Enter).

- **Order when re-chaining:** remove the piece, then lock -- the [tile lock](tile-lock.md) refuses Remove Piece on a tile
  that is already locked.

## Adopted (owner, 2026-10-22): the chain as a tile power, set up from one side

**Owner:** the chain role is a tile power. Should the partner ("chain breaker") be a power too? If both have to point
at each other during map setup, is that a recipe for errors? Maybe only the chained tile names its partner, and at
runtime the partner is told it's connected (and may change its look).

**Claude's take: one-sided.** Only the **Chained** tile is a powered tile class; it names its partner. The partner is
**any tile**, with no power for the chain:

- **One source of truth.** A link written on one tile can't disagree with itself; two tiles pointing at each other can
  (A says B, B says C, or one side forgotten).
- **The partner keeps its own power slot.** One power per tile: if "partner" were a power, a partner couldn't also be,
  say, a Push tile. As plain data it can.
- **It fits the tile-power setup decisions:** the Chained tile class fixes *what* it is (`TilePowerTag` =
  `ConnectIt.TilePower.Chained`, defaults-only); **which partner** is an exposed value on the placed tile, like any
  power's tunable values.
- **Pointing at the partner:** in a level, an actor picker on the placed Chained tile (the designer clicks the partner
  tile). In a layout cell, the partner's cell position (fixed when the layout is authored). Either way it's resolved
  when the tile joins the board to a **stable tile id** -- generated, never authored, carried with each tile's data --
  so shifts don't break the link (a position would; this replaces the shared `ChainTag`). The owner's concern about
  indices was authored ones that drift; this id is created by the board and never typed by anyone.
- **The partner "knows" it's connected without storing anything:** whether a tile is someone's partner is derived from
  the board (some chained tile names its id). Visuals ask a helper (`GetChainedTilesFor(Position)`) when the board is
  seeded or tiles are added, and change the partner's look -- nothing to keep in sync.
- **How it runs:** as a power whose trigger is **Sync** -- checked on every settle pass against its partner's state
  (a partner cleared by scoring is not a Piece Exit, so no ordinary trigger would see it). This is the sync-rule hook
  already in tile-powers Stage 4, now authored as a tile's power instead of a level rule. Sync powers are exempt from
  the once-per-action guard (they only follow state, so they can't loop: lock when the partner is empty, unlock when
  it isn't) and have no cooldown. Power Triggered is appended only when they change something.
- **Pitfalls:** a partner that's removed (Remove Tiles / Change Layout) leaves the chained tile locked for good --
  validate layouts, and the tile can show "broken chain"; a Chained tile with no partner set -> validation error; a
  Chained tile pointing at another Chained tile is allowed (a chain of chains) but worth a warning until tested.

**Owner on the pitfalls (2026-10-22) -- both are gameplay, not errors:**

- **A chained tile with no partner stays locked** and shows a separate **"no partner"** icon. Intended: e.g. phase 1
  has a few chains with no partners; phase 2 brings the partners in, or the Level director assigns them. So "no partner
  set" is a valid state (no validation error; at most an informational note in the layout tool), and a partner removed
  by a layout change simply leaves its chained tiles unpartnered.
- **Several chained tiles may name the same partner** (many-to-one): one piece on the partner opens all of them;
  emptying it locks them all. This replaces "a chain is a pair" from the lock plan.

**Follows from that (Claude):** a partner assigned *during* a match needs a way to set it:

- **Set Chain Partner {Chained tile, Partner tile or none}** -- a board operation with an event (the chained tile's
  link changes; the partner's look updates). It changes a power's *value*, not its identity, so it fits "a tile's power
  never changes at runtime". The Level director uses it after bringing partners in (e.g. Change Layout, then Set Chain
  Partner for each chain). Allowed on a locked tile (it's the chain's own setup).
- Alternative for layouts only: a layout cell may name a partner **position** that only becomes a tile in a later
  layout -- resolved when that tile is added. More automatic, but it means remembering positions across shifts; the
  explicit operation is simpler and visible in the Level director's script. Lean: the operation.

**Adopted** in full (one-sided chains, no partner valid, many chained tiles per partner, Set Chain Partner, the Sync
trigger). Applied to [tile-lock-and-chained-tiles-plan](tile-lock-and-chained-tiles-plan.md) Stage 2 (role +
`ChainTag` on tile data → a Chained tile class + partner id) and adds **Sync** to the trigger list in
[tile-power-triggers](tile-power-triggers.md) and the tile-powers plan.

## Open
- Can the **opponent** unchain your tile by placing on the partner, and does that destroy your piece later if their
  piece leaves? (Derived state: any piece on the partner holds it open, whoever's.)
- Is the destroyed piece's loss announced (a "chain snaps" visual) beyond Piece Removed?
- ~~One partner → one chained tile, or many?~~ Decided for now (owner, 2026-10-22): a chain is a pair; more per group
  to be tested later.
- Where the rule runs: decided -- a sync rule in the tile-powers settle step (built next), see the plan.
- Board Shift with chains: do chained tiles / partners shift (tags make it safe), or are they unshiftable anchors?
- The AI: as a rule in the after-move step it's seen by the search automatically.
