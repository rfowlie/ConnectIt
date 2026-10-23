---
Date: 2026-10-21
status: Partly superseded
superseded by: "[2026-10-22-tile-catalog-and-builder-tiles](2026-10-22-tile-catalog-and-builder-tiles.md) (a ConnectIt project settings class returns for the default tile catalog and the builder tile class; maps still name their board, preset and catalog in World Settings)"
tags:
  - architecture
  - data
---
## Decision

The ConnectIt project settings (`UConnectIt_LevelConfigSettings`) are deleted. A map's board and default preset come only from
its World Settings (`AConnectIt_WorldSettings::FindLevelConfig` / `FindDefaultMatchPreset`); there are no project-wide
fallbacks. A map with no board is allowed (a menu map): the board-registry setup warns, validation warns, and the match
setup warns when there is no preset. The main menu will reference its level catalog asset directly
(`UConnectIt_MatchSetupSubsystem::GetLevelCatalog` removed).

## Why

Once each map names its own board and preset, the settings held only fallbacks and an unused catalog pointer. The fallback
existed (2026-09-14) because a duplicated level that nobody registered left the board silently unset; duplicating a map now
copies its World Settings, so that case works by itself, and a genuinely empty map gets a warning naming the field to set.
Fewer places to configure, and no asset paths left as strings in config.

## What Would Change It

A need for a project-wide default (e.g. many throwaway test maps sharing one board) -- a fallback could return on the World
Settings class's defaults rather than in project settings; or a menu that must find the catalog without a reference (then a
settings entry, or an asset-manager primary asset type).

Supersedes [2026-09-14-level-config-default-fallback](2026-09-14-level-config-default-fallback.md).
