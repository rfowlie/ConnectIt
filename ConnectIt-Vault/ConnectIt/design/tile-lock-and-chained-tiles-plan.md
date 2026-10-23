---
type: plan
status: not-started
approved: true
approved-on: 2026-10-22
completed-on:
playtested: false
design: "[tile-lock](tile-lock.md)"
question:
decision:
requires: "[tile-powers-plan](tile-powers-plan.md)"
---

# Plan: tile lock and chained tiles

Design: [tile-lock](tile-lock.md), [chained-tiles](chained-tiles.md). First use: the first adventure level, "Placing
pieces" ([adventure-modules-and-progression](adventure-modules-and-progression.md), Level 1 draft). Written 2026-10-22
in a design-only session; **approved by the owner 2026-10-22**. Not built -- building happens in its own session.

> **Prerequisite: [tile-powers-plan](tile-powers-plan.md)** (owner, 2026-10-22: tile powers are built next). Stage 1
> (the lock) doesn't depend on it; **Stage 2 (chained tiles) needs its settle step** (tile-powers Stage 4).

## Context

Settled with the owner on 2026-10-22:

- **A locked tile** is on the board and in play but can't take a piece by a player's placement. It can hold a piece, and
  that piece counts in lines. Different from an inactive tile (out of play).
- **Each operation declares whether it works on locked tiles** (`CanOperateOnLockedTiles()`, default no). For now only
  Board Shift (the lock moves with its tile), Force Place, Remove Tiles, Change Layout, Set Tile Locked and Set Chain
  Partner can (Set Tile Power was dropped 2026-10-22); Place, Swap, Capture, Remove, Destroy / Adjust Multiplier, Toggle Active and (later) Move Piece
  can't. Adjust when something comes up.
- **The AI treats an empty locked tile as blocking** a line.
- **A chained tile** is locked exactly while its **partner** tile has no piece -- for any reason, scoring included. A
  piece placed on the partner unlocks it; when the partner empties, the chained tile locks again and **its piece is
  destroyed** (removed first, then locked). A **capture** on the partner changes nothing (the piece only changes
  faction).
- **Revised 2026-10-22 (owner): one-sided chains.** The chain is a **tile power** on the chained tile (a Chained tile
  class); only it names its partner, which can be **any tile** (no power for the chain, so it keeps its own power
  slot). **No partner is valid** -- the tile stays locked and shows a "no partner" icon. **Several chained tiles may
  share one partner.** Partners can be set mid-match with **Set Chain Partner**. The link uses a stable tile id (not a
  position, not a shared tag), so shifts don't break it. Runs on the **Sync** trigger.

Each stage builds and passes the tests before the next; the editor must be closed for builds; the owner commits between
stages.

## Stage 1: the lock

- **State:** `bIsLocked` on `FConnectItTileData` (replicated with the board, Blueprint-readable) and on
  `FConnectItTileInitialData` (so placed tiles and layout cells can start locked; `InitialiseBoardState` and Add Tiles
  copy it).
- **The placement rule:** `FConnectItBoardOperation_PlacePiece::IsTilePlaceable` = active and not occupied **and not
  locked** -- Place Piece's `CanApply`, its legality (`No Legal Move`), the AI's move generation and the hover highlight
  all follow.
- **Per-operation rule** on `FConnectItBoardOperation`:
  - `virtual bool CanOperateOnLockedTiles() const { return false; }`, overridden to `true` by Shift, Force Place, Remove
    Tiles, Change Layout and Set Tile Locked.
  - `virtual void GetTargetPositions(TArray<FGridPosition>&) const` -- the existing tiles it acts on (Place: its tile;
    Swap: both; Shift: its line; Change Goals / Begin Phase / Add Tiles: none).
  - **One check**, `FConnectItRuleSet::CanCarryOut(Operation, Board, OutWhyNot)`: `CanApply` plus "no locked target
    unless the operation allows it" ("tile (3,2) is locked"). Called where `CanApply` is called today
    (`UConnectIt_BoardRequestMediator` dispatch, `UConnectIt_BoardStateLibrary::SimulateOperation`), and by tile powers
    later.
- **Set Tile Locked** operation {Position, bLocked} → event **Tile Lock Changed** {Position, bLocked} (typed Blueprint
  getter in `UConnectIt_BoardEventLibrary`). Scores nothing. For the Level director (guided tutorial steps) and later
  tile powers (a lock change is a Tile Changed trigger).
- **Why-not for the UI:** `GetPlacementBlocker(Board, Position)` → None / Not A Tile / Inactive / Occupied / Locked,
  Blueprint-callable, so a refused click can say why.
- **The AI's evaluation** (`ConnectIt_MinMaxRules`, the open-window check): an empty locked tile blocks a window.
- Fix `bIsOccupied`'s comment (it means "has a piece").
- Tests: placement refused on a locked tile and allowed after unlocking; each operation's allow / refuse on a locked
  target per the table; Shift carries the lock; Set Tile Locked's event; legality with only locked empty tiles = No Legal
  Move; the AI's evaluation sees a window blocked by a locked tile.

## Stage 2: chained tiles

*Rewritten 2026-10-22 for the one-sided model ([chained-tiles](chained-tiles.md)).*

- **The Chained tile class:** an `AConnectIt_TileBase` subclass (Blueprint) with `TilePowerTag` =
  `ConnectIt.TilePower.Chained` (defaults-only) and a power whose trigger is **Sync** (tile-powers Stage 3 / 4). It
  starts locked.
- **Naming the partner** (an exposed value on the placed tile, like any power's tunable values):
  - in a level: an **actor picker** on the placed Chained tile (click the partner tile); empty = no partner;
  - in a layout cell: the partner's **cell position** in that layout (captured by the grid layout tool).
  When the tile joins the board (`InitialiseBoardState`, Add Tiles) it is resolved to the partner's **stable tile id**
  (tile-powers Stage 3) and stored in the chained tile's power values. A partner that isn't on the board -> no partner.
- **No partner is valid:** the tile stays locked; its visual shows a "no partner" icon. Several Chained tiles may name
  the same partner. A partner removed by Remove Tiles / Change Layout leaves its chained tiles unpartnered.
- **Set Chain Partner** operation {Chained tile position, Partner tile position or none} → event **Chain Partner
  Changed** {Chained, Partner (or none)}; works on locked tiles (it's the chain's own setup); refused if the first tile
  isn't a Chained tile. For the Level director (e.g. Change Layout to bring partners in, then Set Chain Partner).
- **Running the chain (Sync power):** on **every settle pass** (before the first scoring pass and after each one), each
  Chained tile checks its partner: partner has a piece and the tile is locked → unlock; no partner, or partner empty,
  and the tile is unlocked → remove its piece (if any), then lock. State-based, so it catches a partner cleared by
  scoring; idempotent, so it can't loop (exempt from the once-per-action guard, no cooldown). Applied directly,
  appending **Power Triggered** (only when something changes), **Piece Removed**, **Tile Lock Changed**; those count as
  Piece Exit / Tile Changed for other powers in the same settle.
- **Visuals know a tile is a partner without stored state:** `GetChainedTilesFor(Position)` (Blueprint) lists the
  Chained tiles naming it; the partner's look updates on board seed, tiles added and Chain Partner Changed.
- **Validation** (map check / layout save): a Chained tile naming a tile that isn't on the board (informational -- it
  will start unpartnered); a Chained tile naming another Chained tile (warning until a chain of chains is tested).
- Tests: place on the partner → unlocks; partner emptied (removed, swapped away, cleared by scoring) → piece destroyed
  then locked; capture on the partner → nothing; no partner → stays locked; two Chained tiles on one partner open and
  close together; Set Chain Partner links mid-match (and unlinks with none); a shift keeps the link; removing the partner
  leaves the chained tile unpartnered; Simulate Operation and the AI see the same.

## Stage 3: visuals and the tutorial (owner, editor)

- Tile visuals: a locked look; a chained look (chains) that snaps on Tile Lock Changed; a "no partner" icon; the
  partner's look (from `GetChainedTilesFor`).
- Refused-click message from `GetPlacementBlocker`.
- Level 1 "Placing pieces": the phase that teaches locked / chained tiles.

## Stage 4: vault

Decision notes (the tile lock with per-operation rules; chained tiles as derived state); `code/` recipe "lock a tile /
chain two tiles"; status lines in `tile-lock.md`, `chained-tiles.md`; plan properties; log.

## Not in this plan

- Several reasons to lock one tile at once (a lock-reason container) -- one owner per tile for now.
- Several partners for one chained tile (several chained tiles for one partner **is** in).
- Tile powers themselves (a prerequisite, see above).

## Verification

- Each stage: build (editor closed); `Automation RunTests ConnectIt` all pass.
- Owner, PIE: a board with a locked tile (can't place, hover shows it, refused click says "locked"); a chained tile --
  place on the partner, the chain opens; place on the chained tile; remove / score the partner's piece, the chain closes
  and the piece goes; vs AI, the AI never tries a locked tile.

## Settled before approval (owner, 2026-10-22)

1. ~~A chain is a pair~~ -- replaced the same day: one-sided chains, no partner valid, many chained tiles per partner,
   Set Chain Partner, the Sync trigger (see Context and Stage 2).
2. Tile powers are built next: the chain rule goes straight into their settle step, and this plan **requires the
   tile-powers plan** (Stage 2 waits for its Stage 4).
3. Chain changes reuse **Piece Removed** + **Tile Lock Changed** (no separate Chained / Unchained events); the visual
   knows a tile is chained from its class / power. New: **Chain Partner Changed** for Set Chain Partner.
