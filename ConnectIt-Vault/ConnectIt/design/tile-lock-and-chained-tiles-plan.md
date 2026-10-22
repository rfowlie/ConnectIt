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
  Board Shift (the lock moves with its tile), Force Place, Remove Tiles, Change Layout, Set Tile Locked and (later)
  Set Tile Power can; Place, Swap, Capture, Remove, Destroy / Adjust Multiplier, Toggle Active and (later) Move Piece
  can't. Adjust when something comes up.
- **The AI treats an empty locked tile as blocking** a line.
- **A chained tile** is locked exactly while its **partner** tile has no piece -- for any reason, scoring included. A
  piece placed on the partner unlocks it; when the partner empties, the chained tile locks again and **its piece is
  destroyed** (removed first, then locked). A **capture** on the partner changes nothing (the piece only changes
  faction). Linked by a shared tag, not by position, so shifts don't break the link. **For now a chain is a pair: one
  partner, one chained tile** (more per group can be tested later).

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

- **Tile data:** a chain role (None / Partner / Chained) and a chain group, both on `FConnectItTileInitialData` (role +
  a `ChainTag` gameplay tag) and on `FConnectItTileData` (role + the `ChainTag` itself -- a tag, not a resolved index,
  for the same reason as tile powers (owner, 2026-10-22); plain data, cheap to copy in the search). A chained tile also
  starts locked.
- **Validation** (board build and layout assets): a chain group must be exactly one partner and one chained tile;
  anything else -> warning, the group is ignored.
- **The chain rule is a settle rule** in the tile-powers chain (`FConnectItRuleSet::ResolveBoardChange`, tile-powers
  Stage 4), so every path runs it (the Mediator, Simulate Operation, the AI's search):
  - **Each settle pass** (before the first scoring pass, and again after every scoring pass, since what scoring changed
    goes back through settle): for each chain, if the partner has a piece and the chained tile is locked → unlock; if the
    partner is empty and the chained tile is unlocked → remove its piece (if any), then lock.
  - It is a **sync rule, not a triggered power**: it runs every settle pass and checks state, so it also catches a
    partner cleared by scoring (which is not a Piece Exit, so no trigger would fire). It is idempotent and can't loop.
  - Applied directly (it is the rule, not an operation going through the lock check), appending **Piece Removed** then
    **Tile Lock Changed** for each change. Its lock changes count as **Tile Changed** for tile powers, and the removed
    piece as a **Piece Exit**, so they can set off powers in the same settle.
  - **Needs from the tile-powers plan:** a hook for sync rules in the settle step (added to that plan's Stage 4).
- Tests: place on the partner → chained tile unlocks; the partner's piece removed / swapped away → chained tile's piece
  destroyed and the tile locked (piece removed before lock); scoring clears the partner → re-chains after scoring; capture
  on the partner → nothing; a shift moving both keeps the link; Simulate Operation and the AI see the same.

## Stage 3: visuals and the tutorial (owner, editor)

- Tile visuals: a locked look, and a chained look (chains) that snaps on Tile Lock Changed for a chained tile.
- Refused-click message from `GetPlacementBlocker`.
- Level 1 "Placing pieces": the phase that teaches locked / chained tiles.

## Stage 4: vault

Decision notes (the tile lock with per-operation rules; chained tiles as derived state); `code/` recipe "lock a tile /
chain two tiles"; status lines in `tile-lock.md`, `chained-tiles.md`; plan properties; log.

## Not in this plan

- Several reasons to lock one tile at once (a lock-reason container) -- one owner per tile for now.
- Several partners for one chained tile, or several chained tiles for one partner.
- Tile powers themselves (a prerequisite, see above).

## Verification

- Each stage: build (editor closed); `Automation RunTests ConnectIt` all pass.
- Owner, PIE: a board with a locked tile (can't place, hover shows it, refused click says "locked"); a chained pair --
  place on the partner, the chain opens; place on the chained tile; remove / score the partner's piece, the chain closes
  and the piece goes; vs AI, the AI never tries a locked tile.

## Settled before approval (owner, 2026-10-22)

1. A chain is a pair -- one partner, one chained tile -- for now; more per group to be tested later.
2. Tile powers are built next: the chain rule goes straight into their settle step, and this plan **requires the
   tile-powers plan** (Stage 2 waits for its Stage 4).
3. Chain changes reuse **Piece Removed** + **Tile Lock Changed** (no separate Chained / Unchained events); the visual
   knows a tile is chained from its tile data.
