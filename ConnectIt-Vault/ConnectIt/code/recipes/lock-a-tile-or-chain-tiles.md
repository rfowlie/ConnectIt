---
schema: recipes
task: Lock a tile (in play, but no placing on it) -- from a builder tile, a layout, or the Level director mid-match -- and chain a tile to a partner (locked while the partner has no piece); make an operation that may or may not act on locked tiles.
touches:
  - builder tiles (Is Locked, Tile Power Tag = ConnectIt.TilePower.Chained, Chain Partner Tile)
  - the tile catalog (an entry for ConnectIt.TilePower.Chained: its tile class and the Chained power)
  - the Level director (Set Tile Locked, Set Chain Partner)
  - Source/ConnectIt/Public/Board/Operations/ConnectIt_BoardOperation.h (CanOperateOnLockedTiles, GetTargetPositions)
reconciled: 2026-10-23
commit: 2226fcf
---

# Recipe: lock a tile, or chain tiles

## Goal

- **A locked tile:** on the board and in play -- its piece counts in lines -- but no one can place on it, and only some
  operations act on it. For a guided tutorial step, or as the Chained tile's state.
- **A Chained tile:** locked exactly while its **partner** has no piece. A piece on the partner opens it. When the partner
  empties (removed, cleared by scoring...), the chained tile's piece is destroyed and it locks again.

Decisions: [the tile lock](../../_decisions/2026-10-23-tile-lock-per-operation-rule.md),
[chained tiles as a Sync power](../../_decisions/2026-10-23-chained-tiles-as-a-sync-power.md).

## Prerequisites

- Builder tiles and a tile catalog ([[add-a-tile-power]] covers both).
- For chained tiles: a catalog entry for `ConnectIt.TilePower.Chained` -- **Tile Class**: the chained tile's look;
  **Power**: Trigger **Sync**, Effect **Chained** (nothing else to set).

## Steps

1. **Start a tile locked:** on its builder tile (or a layout cell), tick **Is Locked**.
2. **Lock or unlock mid-match:** from the Level director, **Apply Board Operation** with **Set Tile Locked** (Position,
   Locked). Already in that state: refused (logged), nothing changes.
3. **Chain a tile:**
   1. On its builder tile, set **Tile Power Tag** = `ConnectIt.TilePower.Chained`.
   2. Set **Chain Partner Tile**: click the partner builder tile, which can be any tile. Several chained tiles may share
      one partner.
   3. Leave Is Locked alone: when the board is built it is settled once, and a chained tile starts locked unless its
      partner starts with a piece.
   - **No partner** is valid: the tile stays locked (show a "no partner" icon).
   - **Mid-match:** **Set Chain Partner** (Position, Has Partner, Partner Position) links or unlinks. A phase can bring
     the partners in with Change Layout, then link them.
4. **Visuals** (the tile class Blueprints):
   - **Locked look:** the tile's `bIsLocked` (display board), and **Tile Lock Changed** (`Get Active Tile Lock Changed
     Event`).
   - **A chained tile's partner:** **Get Chain Partner** (false = no partner).
   - **A partner's look:** **Get Chained Tiles For** (its position) on board seed, Tiles Added and **Chain Partner
     Changed**.
   - **A refused click's message:** **Get Placement Blocker** (Not A Tile / Inactive / Occupied / Locked).
5. **A new operation (C++):** by default it may not act on locked tiles. Override `GetTargetPositions` to list the
   existing tiles it acts on, so the check can refuse a locked one. Or override `CanOperateOnLockedTiles` to return true
   if it should reach them (as Shift does: the lock moves with its tile).

## Verify

- Output Log: a refused request names the tile -- `... rejected -- tile (3,2) is locked`.
- Chaining: place on the partner; the chain opens (Power Triggered, then Tile Lock Changed). Remove the partner's
  piece: the chained tile's piece goes, then it locks.
- Tests: `ConnectIt.Board.TileLock`, `ConnectIt.Board.ChainedTiles`.

## Pitfalls

- **Locked is not inactive:** a locked tile is in play -- its piece scores and can be cleared by scoring; Force Place and
  Shift still reach it. Inactive is out of play.
- **Which operations reach locked tiles:** Shift, Force Place, Remove Tiles, Change Layout, Set Tile Locked, Set Chain
  Partner. Place, Swap, Capture, Remove, Destroy Multiplier and Toggle Active don't, and neither do tile powers' effects
  built from them.
- **Setting a chained tile's lock by hand fights the chain:** its Sync power puts the lock back to match the partner on
  the next settle. Change the partner instead.
- **Chain partner on a tile that isn't Chained:** ignored (validation warns). A chain of chains (a Chained partner)
  warns until it's tested.
- **The partner must exist when the tile joins the board:** a builder tile pointing at nothing, or a layout cell
  pointing outside its layout, starts unpartnered (warning).

## See also

- Recipes: [[add-a-tile-power]], [[script-a-level]], [[script-a-phased-level]]
- Flow: [[tile-power-chain]]
- Design: [tile-lock](../../design/tile-lock.md), [chained-tiles](../../design/chained-tiles.md); plan:
  [tile-lock-and-chained-tiles-plan](../../design/tile-lock-and-chained-tiles-plan.md)
