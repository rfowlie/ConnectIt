---
Date: 2026-10-22
status: Active
superseded by:
tags:
  - architecture
  - board
  - tile-powers
  - level-design
---
## Decision

A tile's power is **defined once, by tag, in a tile catalog**, and designers block levels out with **builder tiles**:

- **Tile catalog** (`UConnectIt_TileCatalog`, a data asset):
  - a map from power tag to the **tile class** that shows it and the **power** (trigger, gate, cooldown, effect and
    its values);
  - a plain tile class for tiles with no power.
  - A level uses its level config's catalog, else the project default. Another skin or look is another catalog.
- **Builder tiles** (`AConnectIt_BuilderTile`) are the only tiles placed in a level.
  - Each names a tag and its per-tile settings (starting piece, multiplier, active, shiftable). Never a power's values.
  - An editor preview shows a catalog over them as transient actors, never saved.
  - When the match starts, each becomes its catalog class.
- **The board carries only the tag** on each tile, plus its id and cooldown. The rules get the catalog's powers when
  the match starts.
- **No level suffix** on tags for now (`ConnectIt.TilePower.Push`); tiles growing in power is unexplored.

## Why

- **Per-tile values made every board copy slower:** a power with its values on every tile cost ~17% AI search speed,
  even with no powered tiles (303,709 against 364,987 nodes/s). Tag only: 392,028.
- **Values set per placed tile invite errors:** anyone can put bad values on a tile. One definition per tag, in one
  place, is tuned in one place.
- **Swapping looks is cheap:** the catalog decides which actor shows a tag, so a new look is a new catalog, not
  edited levels.
- **Builder tiles keep levels independent of the art:** a level only says what goes where.

## What Would Change It

- **Tiles growing in power** (levels): a tag per level, or a level stored on the tile.
- **A power needing per-tile data that isn't a link:** the Chained tile's partner is the first per-tile link, and
  rides with the tile's settings.
- **Open, owner to confirm:** the project-default catalog and the builder tile class live in a ConnectIt project
  settings class (`UConnectIt_ProjectSettings`). That reverses part of
  [no ConnectIt project settings](2026-10-21-no-connectit-project-settings.md); that note's own "What Would Change It"
  suggested World Settings class defaults instead.

Plan: [tile-powers-plan](../design/tile-powers-plan.md) Stage 3. Design: [tile-power-setup](../design/tile-power-setup.md)
("Revised: the tile catalog and builder tiles"). Question:
[tile-powers-in-the-board-pipeline](../_questions/tile-powers-in-the-board-pipeline.md).
