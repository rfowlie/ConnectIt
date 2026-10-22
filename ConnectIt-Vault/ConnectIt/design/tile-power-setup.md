# Tile power setup: tile classes as identity

Design note (2026-10-22). **Under discussion; nothing decided beyond the owner's direction; nothing built.** Builds on
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

## Open

- Shift: which option for powered tiles?
- Power definitions: class-held, Level-director list (approved), or a project catalog with per-level overrides?
- Confirm dropping `_SetTilePower` (tile replacement instead).
