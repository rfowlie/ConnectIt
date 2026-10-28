---
type: plan
status: complete
approved: true
approved-on: 2026-10-28
completed-on: 2026-10-28
playtested: false
design: "[editor-notes-2026-10-28](editor-notes-2026-10-28.md)"
question:
decision: "[2026-10-28-data-asset-classes-end-in-dataasset](../_decisions/2026-10-28-data-asset-classes-end-in-dataasset.md)"
---

# Plan: data asset classes end in DataAsset

From [editor-notes-2026-10-28](editor-notes-2026-10-28.md), item 4 (decided 2026-10-28: rename for clarity). Written
2026-10-28 in a design-only session; **approved by the owner 2026-10-28**. **Built 2026-10-28**; awaiting the owner's resave.

## Context

Convention (owner): **every C++ data asset class ends in `DataAsset`.** Not following it today:

| Today | Renamed |
| --- | --- |
| `UConnectIt_ActionShardTable` | `UConnectIt_ActionShardTableDataAsset` |
| `UConnectIt_TileCatalog` | `UConnectIt_TileCatalogDataAsset` |
| `UConnectIt_BoardLayout` | `UConnectIt_BoardLayoutDataAsset` |
| `UConnectIt_LevelCatalog` | `UConnectIt_LevelCatalogDataAsset` |
| `UConnectIt_MatchPreset` | `UConnectIt_MatchPresetDataAsset` |
| `UConnectIt_MatchConditionAsset` | `UConnectIt_MatchConditionDataAsset` |

Already following it: `UConnectIt_MapNodeDataAsset`, `UConnectIt_LevelConfigDataAsset`, `UActionLoadoutDataAsset`,
`UItemActionInventoryDataAsset`.

## Stage 1: rename with redirects

- Rename each class, its files (`ConnectIt_TileCatalogDataAsset.h/.cpp`...) and its uses in C++.
- **`+ClassRedirects`** in `Config/DefaultEngine.ini` (`[CoreRedirects]`, next to the existing ones) from each old
  `/Script/ConnectIt.<OldName>` to the new name, so saved assets and Blueprints load.
- Property names that embed the old class name (e.g. `TileCatalog`, `DefaultMatchPreset`) **stay** -- only class names
  change, so no property redirects and no asset data changes.
- Tests: all existing pass (they reference classes by type).

## Stage 2: the convention and the vault

- A `_decisions` note: data asset classes end in `DataAsset` (owner, 2026-10-28), and new ones follow it.
- Code pages and recipes that name the old classes; log.

## Verification

- Build (editor closed); `Automation RunTests ConnectIt` passes.
- Owner, editor: open a map, a preset, a tile catalog, a layout and a condition asset -- they load; resave them (so the
  redirects can later be dropped); Data Validation clean.
