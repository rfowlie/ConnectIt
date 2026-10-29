---
Date: 2026-10-28
status: Active
superseded by:
tags:
  - naming
  - conventions
  - data-assets
summary: 'Every C++ data asset class ends in DataAsset (files to match); six ConnectIt classes renamed with class redirects; property names unchanged'
---
## Decision

**Every C++ data asset class ends in `DataAsset`** (owner, 2026-10-28), and new ones follow it. Files are named to match.

Renamed 2026-10-28, with `+ClassRedirects` in `Config/DefaultEngine.ini` so saved assets and Blueprints still load:

| Was | Now |
| --- | --- |
| `UConnectIt_ActionShardTable` | `UConnectIt_ActionShardTableDataAsset` |
| `UConnectIt_TileCatalog` | `UConnectIt_TileCatalogDataAsset` |
| `UConnectIt_BoardLayout` | `UConnectIt_BoardLayoutDataAsset` |
| `UConnectIt_LevelCatalog` | `UConnectIt_LevelCatalogDataAsset` |
| `UConnectIt_MatchPreset` | `UConnectIt_MatchPresetDataAsset` |
| `UConnectIt_MatchConditionAsset` | `UConnectIt_MatchConditionDataAsset` |

Property names that embed a class's old name (`TileCatalog`, `DefaultMatchPreset`, ...) stay: only class names changed.

## Why

- **Clarity:** a type name says whether it is an asset designers make in the Content Browser or a value / struct. "Table",
  "Catalog", "Preset", "Layout" didn't.
- Some already followed it (`UConnectIt_MapNodeDataAsset`, `UConnectIt_LevelConfigDataAsset`, `UActionLoadoutDataAsset`,
  `UItemActionInventoryDataAsset`), so it was a convention half-followed.

## What Would Change It

- Nothing expected. The redirects can be dropped once every affected asset has been resaved.

Plan: [data-asset-naming-plan](../design/data-asset-naming-plan.md). Design:
[editor-notes-2026-10-28](../design/editor-notes-2026-10-28.md), item 4.
