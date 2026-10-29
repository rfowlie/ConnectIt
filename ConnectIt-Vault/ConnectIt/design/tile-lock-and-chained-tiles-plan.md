---
type: plan
status: complete
approved: true
approved_on: 2026-10-22
completed_on: 2026-10-23
started:             2026-10-22          # YYYY-MM-DD, when the first stage was built (blank until then)
stages: 4              # how many stages the plan has
current_stage: complete   # the first stage not yet built ("complete" / "not started")
playtested: false
design: "[tile-lock](tile-lock.md)"
question:
decision: "[tile lock per-operation rule](../_decisions/2026-10-23-tile-lock-per-operation-rule.md); [chained tiles as a Sync power](../_decisions/2026-10-23-chained-tiles-as-a-sync-power.md)"
requires: "[tile-powers-plan](tile-powers-plan.md)"
---

# Plan: tile lock and chained tiles

Design: [tile-lock](tile-lock.md), [chained-tiles](chained-tiles.md). First use: the first adventure level, "Placing
pieces" ([adventure-modules-and-progression](adventure-modules-and-progression.md), Level 1 draft). Written 2026-10-22
in a design-only session; **approved by the owner 2026-10-22**. **Revised 2026-10-22 (evening) for the tile catalog and
builder tiles** -- **re-approved by the owner the same evening**, including the Sync pass on the seeded board; what changed is listed
under "Revision for the tile catalog" at the end. **Complete 2026-10-23** (Stages 1, 2, 4); Stage 3 (editor) left to the
owner, by the owner's call.

> **Prerequisite met:** the [tile-powers-plan](tile-powers-plan.md) is complete (its settle step and Sync powers are
> built). This plan comes **before** the first real powers (tile-powers Stage 5, its own plan), because every power and
> operation must answer whether it can touch a locked tile (owner, 2026-10-22).

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
- **With the tile catalog (2026-10-22, evening):** the Chained tile is a **catalog entry**
  (`ConnectIt.TilePower.Chained`: its tile class shows the chain, its power is Sync). Designers place a **builder tile**
  with that tag and pick its partner there. The partner is a per-tile **link**, not a power value, so it rides with the
  tile's settings and its board data, never in the catalog.

Each stage builds and passes the tests before the next; the editor must be closed for builds; the owner commits between
stages.

## Stage 1: the lock

- **State:** `bIsLocked` on `FConnectItTileData` (replicated with the board, Blueprint-readable) and on
  `FConnectItTileInitialData` (so builder tiles and layout cells can start locked; `InitialiseBoardState` and Add Tiles
  copy it). Compared by `HasSameTilesAndScores`; Tile Lock Changed applies itself.
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
    (`UConnectIt_BoardRequestMediator` dispatch, `UConnectIt_BoardStateLibrary::SimulateOperation`, the Level director's
    operations) **and by tile powers' effects** (`FConnectItPowerChain::TryFire`, built in tile-powers Stage 4 with
    `CanApply`).
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
- *As built (2026-10-22; 57/57 tests; Throughput 356k-363k nodes/s with or without the lock checks):*
  - **Lock state:**
    - `bIsLocked` on `FConnectItTileData` and `FConnectItTileInitialData` (builder tiles and layout cells can start
      locked; `ToTileData` copies it).
    - It is compared by `HasSameTilesAndScores`.
    - `EConnectItTileChange::Lock` is reported by Set Tile Locked.
    - `bIsOccupied`'s comment is fixed.
  - **The rule:**
    - `FConnectItBoardOperation::CanOperateOnLockedTiles` is true for Shift, Force Place, Remove Tiles, Change Layout
      and Set Tile Locked.
    - `GetTargetPositions` is implemented for the operations that can't act on locked tiles: Place, Swap, Capture,
      Remove, Destroy Multiplier, Toggle.
    - `FConnectItRuleSet::CanCarryOut` is called by the Mediator's dispatch (players and the Level director), Simulate
      Operation, and the tile power chain's effects.
  - **Placement:** `IsTilePlaceable` adds `!bIsLocked`, so Place's check, its legality, the AI's moves and the hover
    highlight follow. Swap's legality skips locked pieces.
  - **Set Tile Locked:**
    - `FConnectItBoardOperation_SetTileLocked` (`ConnectIt.Game.State.SetTileLocked`) is refused when the tile is
      already that way, which keeps the Chained Sync power idempotent.
    - Event `Tile Lock Changed` (`ConnectIt.Event.TileLockChanged`, applies itself; Blueprint getter).
    - The operation catalog is now 14.
  - **Blueprint:** `Get Placement Blocker` (None / Not A Tile / Inactive / Occupied / Locked) on the board state
    library.
  - **AI:** `LinePotential` treats an empty locked tile as blocking.
  - **Tests:** new `ConnectIt.Board.TileLock`:
    - placement, the blocker, and unlocking;
    - each operation on a locked target, refused or allowed per the table;
    - Shift carries the lock;
    - Set Tile Locked: its event, replay, and refusing a no-op;
    - legality for Place and Swap;
    - the AI's evaluation;
    - a power's effect refused on a locked tile.
  - **Throughput:** an apparent drop from Stage 4's ~390k is the machine. With the two hot-path lock checks removed,
    readings are the same (349k-363k).

## Stage 2: chained tiles

*Rewritten 2026-10-22 for the one-sided model ([chained-tiles](chained-tiles.md)); revised the same evening for the tile
catalog and builder tiles.*

- **The Chained power:** a C++ effect, `FConnectItTilePowerEffect_Chained`, and a native tag `ConnectIt.TilePower.Chained`.
  The owner's catalog gets an entry for it: trigger **Sync**, this effect, and the tile class that shows a chained tile.
  - The effect reads its tile's partner from the board. If the partner has a piece and the tile is locked: Set Tile
    Locked (false). If there is no partner, or it is empty, and the tile is unlocked: Remove Piece (if any), then Set
    Tile Locked (true).
  - State-based, so it catches a partner cleared by scoring. Idempotent, so it can't loop: exempt from the
    once-per-action guard, no cooldown, and Power Triggered only when something changes (tile-powers Stage 4, as built).
  - Its changes are Piece Exit / Tile Changed for other powers in the same settle.
- **The partner is a per-tile link:**
  - **Settings** (`FConnectItTileInitialData`): `bHasChainPartner` + `ChainPartner` (a grid position). Layout cells
    carry these as captured.
  - **Builder tile:** an actor picker, `Chain Partner Tile` (click the partner builder tile; empty = no partner). Its
    grid position fills the settings.
  - **Board** (`FConnectItTileData`): `ChainPartnerId` (the partner's stable tile id; INDEX_NONE = none). It travels
    with the tile through shifts.
  - **Resolved when tiles join the board:** `InitialiseBoardState`, once every tile has its id; Add Tiles, against the
    board after adding, so a partner can be an existing tile or one added alongside. A partner position that isn't a
    tile → no partner, with a warning.
  - Only a Chained tile's link means anything. A link on another tile is ignored, with a validation warning.
- **Starting state -- Sync powers settle the seeded board** (approved, owner 2026-10-22):
  - When the board is seeded, the GameMode runs one Sync pass on it (the rule set's chain with an empty change), before
    it is committed.
  - So a Chained tile starts locked unless its partner starts with a piece, without the designer setting it.
  - The pass's events (Power Triggered, Tile Lock Changed) follow Board Seeded in the seed's event list, and the
    display board replays them.
  - The alternative is the builder tile's own "starts locked" setting plus a validation check. Simpler, but wrong when
    the partner starts with a piece.
- **No partner is valid:** the tile stays locked; its visual shows a "no partner" icon. Several Chained tiles may name
  the same partner. A partner removed by Remove Tiles / Change Layout leaves its chained tiles unpartnered: the link
  points at an id no tile has, which reads as none.
- **Set Chain Partner** operation {Position, bHasPartner, PartnerPosition} → event **Chain Partner Changed** {Position,
  partner id and position, or none}, applying itself.
  - Works on locked tiles (it's the chain's own setup).
  - Refused if the tile isn't Chained (its `TilePowerTag`) or the partner position isn't a tile.
  - For the Level director, e.g. Change Layout to bring partners in, then Set Chain Partner.
- **Visuals know a tile is a partner without stored state:** `GetChainedTilesFor(Board, Position)` (Blueprint) lists the
  Chained tiles naming it. The partner's look updates on board seed, tiles added and Chain Partner Changed.
- **Validation** (map check / layout save):
  - a Chained builder tile or cell naming a position that isn't a tile (informational: it starts unpartnered);
  - naming another Chained tile (warning until a chain of chains is tested);
  - a partner set on a tile that isn't Chained (warning: ignored).
- **Tests:**
  - place on the partner → unlocks;
  - partner emptied (removed, swapped away, cleared by scoring) → piece destroyed, then locked;
  - capture on the partner → nothing;
  - no partner → stays locked;
  - two Chained tiles on one partner open and close together;
  - Set Chain Partner links mid-match (and unlinks with none);
  - a shift keeps the link;
  - removing the partner leaves the chained tile unpartnered;
  - the seeded board settles: locked, or unlocked when the partner starts with a piece;
  - before + events = after;
  - Simulate Operation and the AI see the same.
- *As built (2026-10-23; 58/58 tests; Throughput not measured -- the CPU was capped at 30% of its maximum frequency
  throughout, ~68k nodes/s; no search hot-path code changed, one int32 per tile):*
  - **The power:** `FConnectItTilePowerEffect_Chained` (new `Public/Board/Powers/ConnectIt_TilePowerEffects.h`, for the
    effects ConnectIt ships) and native tag `ConnectIt.TilePower.Chained`.
    - The effect returns Set Tile Locked (false) when the partner has a piece, or Remove Piece (if occupied) then Set
      Tile Locked (true) when it doesn't.
    - Set Tile Locked refusing a no-op keeps it quiet when the board already agrees.
  - **The link:**
    - **Settings:** `bHasChainPartner` + `ChainPartner` on `FConnectItTileInitialData`.
    - **Board:** `ChainPartnerId` on `FConnectItTileData` (compared, travels with shifts).
    - **Helpers:** `FConnectItBoardState::LinkChainPartner` and `FindTileIndexById`.
    - **Resolved:** in `InitialiseBoardState` once every tile has its id (unresolved: warning), and in Add Tiles after
      all cells are added; Tiles Added's `ApplyTo` does the same.
  - **Builder tile:**
    - `Chain Partner Tile` picker (`EditInstanceOnly`); `GetInitialTileData` turns it into the partner's grid
      position through the level config's grid definition.
    - Load Layout Into Level sets the pickers back from the cells' partner positions.
    - Validation: a partner on a non-Chained tile (warning: ignored), itself (error), a Chained partner (warning).
  - **Seed pass:** `InitialiseBoardState` takes the match's rules (the GameMode passes them). Previous = the board as
    built; Current = it after one `ResolveBoardChange` with an empty change. The settle's events follow Board Seeded
    in the seed's list, and the display board starts from Previous.
  - **Set Chain Partner** (`ConnectIt.Game.State.SetChainPartner`; the operation catalog is now 15):
    - may act on locked tiles;
    - refused if the tile isn't Chained or the partner isn't another tile;
    - reports a Power tile change.
    - Event **Chain Partner Changed** (`ConnectIt.Event.ChainPartnerChanged`, applies itself; Blueprint getter).
  - **Blueprint:** `Get Chained Tiles For` and `Get Chain Partner` (not in the plan -- the "no partner" icon needs
    it).
  - **Layout validation:** a partner on a non-Chained cell, a partner position that isn't another cell of the layout
    (it will have no partner), a Chained partner -- all warnings.
  - **Not done:** the plan's "informational" message for a partner that isn't on the board is a warning instead, kept
    simple; and "swapped away" isn't tested, since a swap always leaves both tiles occupied.
  - **Tests:** new `ConnectIt.Board.ChainedTiles`:
    - the seed pass, both ways;
    - place on the partner opens it; emptying the partner destroys the piece then locks (event order checked);
    - scoring clears the partner;
    - a capture does nothing;
    - no partner;
    - two chained tiles on one partner;
    - Set Chain Partner, linking, unlinking and refused;
    - a shift keeps the link, removing the partner's tile unlinks;
    - Add Tiles with a partner added alongside;
    - Simulate Operation and the AI's ApplyMove;
    - replay checks throughout.

## Stage 3: visuals and the tutorial (owner, editor)

*Left to the owner (2026-10-23): not tracked by this plan any more; the recipe `code/recipes/lock-a-tile-or-chain-tiles.md`
lists what the editor side needs.*

- The catalog entry for `ConnectIt.TilePower.Chained` (the Chained tile class, the power with the Chained effect).
- Tile visuals: a locked look; a chained look (chains) that snaps on Tile Lock Changed; a "no partner" icon; the
  partner's look (from `GetChainedTilesFor`).
- Builder tiles in the level: the Chained tag and its partner picked.
- Refused-click message from `GetPlacementBlocker`.
- Level 1 "Placing pieces": the phase that teaches locked / chained tiles.

## Stage 4: vault

Decision notes (the tile lock with per-operation rules; chained tiles as a Sync tile power with a per-tile link);
`code/` recipe "lock a tile / chain two tiles"; the tile-powers recipe and flow (the lock-aware check); status lines in
`tile-lock.md`, `chained-tiles.md`; plan properties; log.

- *As built (2026-10-23):*
  - **Decisions:** [tile lock per-operation rule](../_decisions/2026-10-23-tile-lock-per-operation-rule.md) and
    [chained tiles as a Sync power](../_decisions/2026-10-23-chained-tiles-as-a-sync-power.md), both indexed.
  - **New recipe:** `code/recipes/lock-a-tile-or-chain-tiles.md`, indexed.
  - **Tile-power pages:** `add-a-tile-power.md` and the `tile-power-chain.md` flow now name the lock-aware check;
    anchored at `2226fcf`.
  - **Status lines** in `tile-lock.md` and `chained-tiles.md`.
  - **Plan:** complete.

## Not in this plan

- Several reasons to lock one tile at once (a lock-reason container) -- one owner per tile for now.
- Several partners for one chained tile (several chained tiles for one partner **is** in).
- Tile powers themselves (built: the tile-powers plan); the first real powers (Push, Repel, Vacuum) are their own plan,
  after this one.

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

## Revision for the tile catalog (2026-10-22, evening; approved the same evening)

Since approval, tile powers were built with a **tile catalog and builder tiles**
([decision](../_decisions/2026-10-22-tile-catalog-and-builder-tiles.md)): power values live once per tag in the catalog,
designers place builder tiles naming a tag, and the board carries only the tag. This revision follows from that:

- **The Chained tile is a catalog entry**, not a Blueprint subclass with a defaults-only tag: a C++ Sync effect and a
  native tag, with the owner's catalog entry giving its tile class.
- **The partner is a per-tile link** in the tile settings (builder tile picker, layout cell position) and on the board
  (`ChainPartnerId`), not "a power's tunable value": the catalog holds no per-tile values.
- **New (approved):** Sync powers settle the seeded board, so a chained tile starts in the right state; and a link on
  a non-Chained tile is ignored with a warning.
- **Stage 1:** tile powers' effects switch from `CanApply` to the lock-aware `CanCarryOut` (tile powers are built now);
  `bIsLocked` joins the board comparison and the self-applying events.
