---
Date: 2026-10-21
status: Active
superseded by:
tags:
  - architecture
  - data
summary: 'Each map names its board (level config) and default match preset in its World Settings; the level config is a reusable board; the project''s name-keyed LevelConfigs registry is gone (defaults remain as fallbacks)'
---
## Decision

Each map names its **board** (level config) and its **default match preset** in its World Settings
(`AConnectIt_WorldSettings`, the project's World Settings class). The level config is now only the board -- reusable by any
number of maps -- and the match preset is how a level plays (one per adventure level, or a shared mode the menu offers). The
project settings' name-keyed `LevelConfigs` registry is removed; `DefaultLevelConfig` and a new `DefaultMatchPreset` remain as
fallbacks for a map that names none. The main menu still picks any preset for any map.

## Why

With `DefaultMatchPreset` on the level config, the reusable board values were welded to a per-level choice, forcing a level
config per level. Reversing the dependency (the preset naming its board) would break shared modes (a Versus preset playable on
any board) and still leave the map needing to find its preset. The map is the one thing that is truly per level, so the pairing
lives there -- as real asset references, which also removes the ini path strings that didn't follow folder moves (and a
registry keyed by the wrong name that sent the swap level to the test board).

## What Would Change It

Needing a level's setup without loading its map (e.g. a menu showing each level's default mode from the catalog alone) -- then
the catalog entry would carry the default preset too; or World Partition / level instances where a map's World Settings don't
describe the playable level.

Partly supersedes [2026-10-14-stage-and-match-preset](2026-10-14-stage-and-match-preset.md) (where the default preset is named).
