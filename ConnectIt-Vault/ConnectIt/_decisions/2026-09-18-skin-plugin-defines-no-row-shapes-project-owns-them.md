---
Date: 2026-09-18
status: Active
superseded by:
tags:
  - visuals
  - skins
  - plugins
  - architecture
---
## Decision

The skin plugin (`UnrealSkinMechanics`) is **project-agnostic about what a skin is**. It
does not define skin data types; the game project defines the row structs of its own skin
tables, and does so when it starts consuming the plugin (ConnectIt's rows are designed then,
not now).

What the plugin owns instead:

- **Row name = stable skin ID.** `USkinSettings::SkinTables` is a
  `TMap<CategoryTag, TSoftObjectPtr<UDataTable>>` — **one table per category**, any row
  struct. Content packs extend a category through a `UCompositeDataTable` rather than an
  array of tables per category. The map keys double as the category list.
- **One tiny base row: `FSkinPresetRowBase { TMap<Tag, FName> SkinIds }`.** The only row
  struct the plugin defines, because it must read a preset to resolve a selection. Projects
  derive from it to add display fields. `USkinSettings::PresetTable` must use a
  derived struct (checked at runtime).
- **`USkinSubsystem` (`UGameInstanceSubsystem`) + `USkinSaveGame`.** Resolves per-category
  precedence — *level-forced > player override > player preset > default preset*, where a
  candidate only counts if its ID exists as a row in that category's table — and hands the
  project a `FDataTableRowHandle` (table + row name) to read with its own struct.
  Player mutators apply immediately and broadcast `OnSkinChanged(Category)` for categories
  whose resolved ID changed; `SaveSelection()` persists preset + overrides,
  `ReloadSelection()` reverts to the saved slot (an Apply/Revert options menu without the
  plugin knowing any UI). **Level-forced skins are runtime-only and never saved**; the project
  calls `SetLevelForced` from its own level config.
- **Removed:** `USkinDataAsset`, `FSkinCatalogRow`, `FSkinCategoryDefinition`,
  `FSkinSelection`, and the optional owner/faction context on the resolver API (per-faction
  data lives inside the project's own row).

## Why

The plugin can't know what "changing a skin" means for a given project's actors — that's
mesh/material/VFX swaps in one game and UI tokens in another. Defining catalog rows,
categories or a skin data-asset base in the plugin was guessing at project territory
(over-engineering, per the owner). The plugin's real, reusable job is selection: which ID
is active, saved, overridden, forced, and announced. Everything shape-specific belongs to the
consumer.

This partially supersedes
[skin-catalogs-as-datatable-arrays-and-plugin-name](2026-09-18-skin-catalogs-as-datatable-arrays-and-plugin-name.md)
(arrays of catalog tables with plugin-defined rows → one project-defined table per category;
the plugin name and "row name = ID" stand) and the "skin assets / payload" detail of
[skin-system-layered-data-and-flow](2026-09-18-skin-system-layered-data-and-flow.md) (the
definition layer is now project-owned; the selection, resolution and consumption layers
stand). It also drops the owner-context note in
[skins-local-only-and-data-on-fixed-actor-class](2026-09-18-skins-local-only-and-data-on-fixed-actor-class.md)
(local-only and fixed-class rules stand).

## What Would Change It

If several projects converge on the same row fields, a shared optional base row (display
name, icon) could be added — but only from evidence, not in advance. A need for more than one
table per category *without* composite tables would bring back a small pairing struct.
