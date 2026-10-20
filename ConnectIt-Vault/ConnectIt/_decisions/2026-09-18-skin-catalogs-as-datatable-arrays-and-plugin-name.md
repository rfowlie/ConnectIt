---
Date: 2026-09-18
status: Active
superseded by: partially — arrays of plugin-defined catalog tables became one project-defined table per category, see [2026-09-18-skin-plugin-defines-no-row-shapes-project-owns-them](2026-09-18-skin-plugin-defines-no-row-shapes-project-owns-them.md)
tags:
  - visuals
  - skins
  - plugins
  - developer-settings
---
## Decision

- The skin plugin is named **`UnrealSkinMechanics`** (class prefix `USkin*`/`FSkin*`, log
  `LogSkinMechanics`). Considered: `UnrealVisualCustomization` (breaks the suite's
  `...Mechanics` pattern), `UnrealCosmeticsMechanics`, `UnrealThemeMechanics` (reads as
  UI-only).
- Which skins and presets exist is declared as **arrays of DataTables** in `USkinSettings`
  (`UDeveloperSettings`, stored in `DefaultGame.ini`): `SkinCatalogs` (row struct
  `FSkinCatalogRow`: category tag, display name, icon, soft ref to the skin asset, sort
  order) and `PresetCatalogs` (row struct `FSkinPresetRow`: display name, icon, category →
  skin-ID map, sort order). **The row name is the stable ID** the player's saved selection
  stores.
- Per-category skin *payloads* stay **typed data assets** (`USkinDataAsset` and subclasses),
  referenced from a catalog row — not DataTable rows.
- `USkinSettings` also holds the category list (`FSkinCategoryDefinition`), a default preset
  ID (fallback of last resort), and the save slot name/user index.

## Why

A DataTable has exactly one row struct, so it fits a flat catalog (an index of skins/
presets) but not payloads whose shape differs per category (piece skin vs action-FX skin vs
UI tokens) — those need typed, validatable data assets. An *array* of tables lets a content
pack or another project add its own table without editing a shared list, which is what
makes the plugin reusable. Using the row name as the ID gives a stable key for saves for
free and keeps IDs out of the assets themselves.

This refines, rather than contradicts,
[skin-system-new-plugin-and-explicit-settings-lists](2026-09-18-skin-system-new-plugin-and-explicit-settings-lists.md):
the lists are still explicit and live in Developer Settings — they are now lists of
tables — and the plugin got its name.

## What Would Change It

Enough skins that hand-maintaining tables is tedious, or content-only contributors adding
skins without touching settings — the trigger to adopt Asset Manager scanning. A category
whose skins turn out to be genuinely flat data (e.g. pure UI tokens) could live in a
DataTable directly instead of behind a data asset.
