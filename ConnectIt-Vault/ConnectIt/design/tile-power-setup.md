# Tile power setup: tile classes as identity

Design note (2026-10-22). **Decided with the owner the same day (below), then revised the same day after the first
build: see "Revised: the tile catalog and builder tiles" at the end -- that section is current; "Decided" is kept as
the history.** Folded into the tile-powers plan's Stage 3.
Builds on
[tile-power-triggers](tile-power-triggers.md) and the approved [tile-powers-plan](tile-powers-plan.md) (Stage 3: power
data and authoring) and [tile-lock-and-chained-tiles-plan](tile-lock-and-chained-tiles-plan.md).

## Owner's direction

For art and design, **a tile's power is part of what the tile *is***:

- Make **subclasses of the base tile class**, one per kind of powered tile (a Push tile, a chained tile...), each with
  its own visuals.
- **Assign the single power in the class defaults**, and **don't expose it on instances** -- a level designer can't
  give a placed Push tile a different power.
- **Don't change a tile's power at runtime** either: a tile set up to look chained that behaves like something else
  would confuse players.

## How that fits what's approved (Claude)

- **The board state stays the truth during a match; the class is the authoring source.** When the board is built from
  placed tiles (`InitialiseBoardState`) or tiles are added (Add Tiles / Change Layout, from a layout cell's
  `TileClass`), the tile's power tag is **read from its class's defaults** and copied into the tile data
  (`PowerTag`). The AI, scoring and firing keep reading tile data, as planned.
- **So the tile-powers plan's Stage 3 changes slightly:** `PowerTag` comes off `FConnectItTileInitialData` (it is
  instance data, which is exactly what the owner doesn't want); the tile class carries it as a defaults-only property
  (`EditDefaultsOnly`, not `EditAnywhere`). Layout cells name a `TileClass`, so they get the power through the class.
- **`_SetTilePower` (the deferred runtime operation) conflicts with this** and should be dropped. If a level ever
  needs a tile to "become" another kind, that is **replacing the tile**: Remove Tiles + Add Tiles with a different class
  -- the phased-levels path, which swaps the actor and its visuals together.
- **Identity vs configuration.** Not everything can be class-only:
  - *Identity* (class defaults, not per instance): the power, the chain role (Partner / Chained), the look.
  - *Configuration* (per instance or per layout cell): starting piece, starting multiplier, active, shiftable,
    starting lock, **the chain link tag** (which partner a chained tile belongs to -- necessarily per placed tile).
  `FConnectItTileInitialData` keeps the configuration; the class holds the identity.
- **Level-wide powers (Crumble) aren't on any class** -- they stay rules authored on the Level director.

## Pitfalls

1. **Board Shift moves tile data but not tile actors.** Today a shift rotates whole tile data along the line (power,
   cooldown, lock, chain role and tag travel together) while the tile *actors* stay where they are -- only pieces are
   re-keyed (`UConnectIt_PieceRegistry::HandleBoardShifted`). With the class as identity, a shift would leave a
   chained-tile actor showing over data that is now a plain tile. Options:
   - **powered tiles don't shift** (`bCanShift = false` set by the class) -- simplest, and they become anchors;
   - **the visuals move the tile actors with the data** on Board Shifted, the way pieces are re-keyed;
   - visuals follow data only (one tile class that changes look by `PowerTag`) -- contradicts the direction.
2. **Where the power's numbers live.** If the class holds the whole power (effect, cooldown, gate), tuning Poison
   differently in one level needs another subclass. If the class holds only the **power tag** and the definitions live
   in one place (the approved plan: a list on the Level director, copied into the rules), each level can tune its
   powers but must list every power its tiles use (validation catches a missing one). A middle option: a project-wide
   power catalog (data asset) with optional per-level overrides.
3. **Two sources that must agree.** The class says "Push"; the board's tile data says what actually fires. They are
   copied once (at build / add) and nothing changes them afterwards -- keep it that way (no runtime power changes) and
   they can't drift.
4. **Class explosion.** One subclass per power × visual variants multiplies quickly. Keep the class for gameplay
   identity plus its base look; per-tile cosmetic variation goes through the layout cell's variant tag or the skin
   system, not more subclasses.
5. **Exposure in Blueprint.** A property declared in C++ as `EditDefaultsOnly` stays hidden on placed instances, but a
   Blueprint subclass can still change it in its own defaults (intended). Watch for a Blueprint adding its own
   instance-editable "power" variable -- the validator should read only the C++ property.
6. **Validation** (editor, on save / map check): a placed tile or layout cell whose class's power the level doesn't
   define; a chained tile with no partner; two powered classes with the same tag but different looks.
7. **The tile registry and replacement.** Replacing a tile (Remove + Add) gives it a new actor; anything holding the old
   actor (UI highlights, hover) must look tiles up by position each time, not cache actors.

## Decided (owner, 2026-10-22) -- partly superseded the same day

*Superseded by the revision below: the defaults-only `TilePowerTag` per subclass (Pitfall 2's values from each
Blueprint) and Pitfall 3's one subclass per power. Still standing: `_SetTilePower` dropped, per-tile configuration on
instances, tile actors following the board (Pitfall 1), validation, and visualizers handling replaced tiles.*

- **`_SetTilePower` dropped.** A tile becomes another kind by Remove Tiles + Add Tiles with a different class.
- **`TilePowerTag`** (renamed from `PowerTag`) lives on the new C++ **`AConnectIt_TileBase`** as **defaults-only**.
- **Per-tile configuration stays exposed** on instances (starting piece, multiplier, active, shiftable, lock, chain
  link).
- **Pitfall 1 isn't one:** keeping tile actors where the board says is the visualizers' job -- on a shift they move the
  tile actors with their data (as the piece registry does for pieces), so class and data never disagree on screen.
- **Pitfall 2 -- values come from the Blueprint subclass:** a powered tile's Blueprint exposes its tunable values and
  hands them over through `GetTilePower()`, so values are tuned per placed tile and per layout cell. The class decides
  *what* the power is, instances tune *how much*. The values are copied onto the tile's data (and travel with it); the
  Level director's list keeps only level-wide powers.
- **Pitfall 3 isn't one:** one `AConnectIt_TileBase` subclass per power; visual variants as Blueprint subclasses of
  that, or handled inside the one power class -- that's what Blueprints are for.
- **Pitfall 4 (validation):** agreed.
- **Pitfall 5:** the visualizers handle a tile being replaced at runtime.

Folded into [tile-powers-plan](tile-powers-plan.md) Stage 3 (and a tile-actor bullet in Stage 2).

## Revised (owner, 2026-10-22): the tile catalog and builder tiles

**Why:** the first Stage 3 build carried each tile's power, with its values, on the board's tile data (each placed
tile could tune its own values, so each board tile needed its own copy). That made every board copy slower: ~17% AI
search speed (303,709 against 364,987 nodes/s), even with no powered tiles. The owner also judged per-tile editable
values a bad setup: anyone can put bad values on a placed tile, and errors spread quickly. Discussion:
[tile-powers-in-the-board-pipeline](../_questions/tile-powers-in-the-board-pipeline.md) (Stage 3 build round).

- **A power is defined once, by tag.** A level is part of the tag (`ConnectIt.TilePower.Poison.1`, `.2`); the tag
  names one exact definition. Nothing about a power's values is set on a placed tile.
- **The tile catalog** (a data asset): a **map from tag to an entry** -- the **tile class** that shows it and the
  **power** (trigger, gate, cooldown, effect and its values). One entry per tag by construction. Its "no power" entry
  (the plain tile class) replaces the level config's default tile class. **Trying another skin or look** = a copy of
  the catalog with other classes slotted in.
- **Chosen on the level config**, with a **project default** (ConnectIt project settings).
- **Builder tiles:** the only tile designers place to block out a level. A builder tile names a **tag** (picked from
  `ConnectIt.TilePower.*`, editable on the placed tile) and carries the per-tile settings (starting piece, multiplier,
  active, shiftable; later lock and chain partner). A small visual shows designers what is where. Layout cells name a
  tag the same way; the layout tool captures builder tiles and places them back.
- **Editor preview, never saved:** a tool takes a catalog and shows each builder tile as the catalog's class. It
  spawns those actors as **transient** (never saved) and hides the builders with the editor's **temporary hide** (not
  saved either); clearing the preview undoes both. Nothing it does can reach the saved level -- the owner's
  requirement that levels never save the temporary swaps.
- **At match start the swap is real:** the board is built from the builder tiles, then the tile registry spawns the
  catalog's class for each one (every machine) and retires the builder. Tiles added mid-match (a layout's cells) get
  their class from the catalog too.
- **At runtime, plain data only:** a board tile carries its tag (plus its ready turn and tile id); the rules get the
  catalog's powers at match start, and firing looks the tag up there. The tile classes stay in the catalog -- never in
  the rules or the board, which the AI copies on a worker thread.
- **Level-wide powers** (Crumble): the Level director lists their **tags**; the definitions come from the catalog.
- **Pitfall 2 answered:** one place to tune (the catalog), one definition per tag. **Pitfall 3:** classes are looks,
  slotted per tag in the catalog, not one subclass per power the designer must pick. **Pitfall 6:** validation checks
  each builder tile's and layout cell's tag against the catalog, and each catalog entry for a class and an effect.
- **Follows for the lock plan** ([tile-lock-and-chained-tiles-plan](tile-lock-and-chained-tiles-plan.md), not yet
  updated): the Chained tile is a catalog entry (`ConnectIt.TilePower.Chained`) rather than a Blueprint subclass, and
  its partner picker sits on the builder tile.

Folded into [tile-powers-plan](tile-powers-plan.md) Stage 3 (revised 2026-10-22).
