# CLAUDE.md — UnrealSkinMechanics

A generic, player-selectable **skin** system: the player picks a preset (a "universal
config" — one skin per category) and can override individual categories ("Preset A, but
Preset B's pieces"). Categories are gameplay tags (`Visual.Piece`, `Visual.Tile`,
`Visual.ActionFX`, `Visual.UI`, …) so a new project adds categories without new code.
Skins are data applied to a fixed actor/widget class — never a class swap — and cosmetics
are local-only (not replicated). Built for reuse across projects; nothing game-specific
belongs here.

Design and decisions: [`ConnectIt/design/skin-system.md`](../ConnectIt/design/skin-system.md),
[layered data & flow](../ConnectIt/_decisions/2026-09-18-skin-system-layered-data-and-flow.md),
[plugin name + DataTable catalogs](../ConnectIt/_decisions/2026-09-18-skin-catalogs-as-datatable-arrays-and-plugin-name.md),
[plugin defines no row shapes — project owns them](../ConnectIt/_decisions/2026-09-18-skin-plugin-defines-no-row-shapes-project-owns-them.md).

## Domain

- **Source:** `../Plugins/UnrealSkinMechanics/Source/` — one Runtime module. Deps:
  `Core`, `CoreUObject`, `Engine`, `GameplayTags`, `DeveloperSettings` (+ `Slate`/`SlateCore`/`UMG`
  private). No plugin-to-plugin dependencies.
- **Status:** **slice 3** — settings, subsystem + save slot, preset base row, debug panel + validation. Enabled in
  `ConnectIt.uproject`; not yet consumed by the game module. Not compiled/PIE-tested by the
  author (see `_logs`).
- **Type:** game-agnostic plugin, independently extractable.
- **Class prefix:** feature-named `USkin*` / `FSkin*`; log `LogSkinMechanics`.
- **Core principle:** the plugin does not know what a skin *is*. Skin table rows are
  project-defined structs; the plugin only uses **row names as stable IDs** and hands the
  project a `FDataTableRowHandle`. See
  [skin-plugin-defines-no-row-shapes-project-owns-them](../ConnectIt/_decisions/2026-09-18-skin-plugin-defines-no-row-shapes-project-owns-them.md).

## What exists

- `USkinSettings` — `SkinTables` (`TMap<CategoryTag, TSoftObjectPtr<UDataTable>>`, one table per
  category; extend via `UCompositeDataTable`), `PresetTable`, `DefaultPresetId`, save slot
  name/user index. Stored in `DefaultGame.ini`.
- `FSkinPresetRowBase` — the only plugin row struct (`TMap<Tag,FName> SkinIds`); projects derive
  from it. Row name = preset ID.
- `USkinSaveGame` — saved preset ID + per-category override IDs.
- `USkinSubsystem` (`UGameInstanceSubsystem`, skipped on dedicated servers) — resolves
  *level-forced > player override > player preset > default preset* per category (a candidate
  counts only if its ID exists in that category's table); `GetSkinRowHandle` / `FindSkinRow<T>`;
  `SetPreset` / `SetOverride` / `ClearOverride` (apply immediately, broadcast
  `OnSkinChanged(Category)` on changes); `SetLevelForced` / `ClearLevelForced` (runtime-only, never
  saved); `SaveSelection` / `ReloadSelection` (Apply / Revert).

- `GetSkinResolution` (winning ID + `ESkinSource`) and `GetConfigurationIssues` (catalog
  validation: missing/empty tables, bad preset table, unknown IDs/categories in presets).
- `USkinDebugWidget` / `USkinDebugRow` — code-built, unstyled panel: every category with a
  skin-ID dropdown + resolved ID/source, preset dropdown, Save/Revert, issue list. Toggle with
  console `SkinMechanics.ToggleDebug` or `ToggleDebugWidget`. See
  [decision](../ConnectIt/_decisions/2026-09-18-skin-plugin-ships-a-generic-debug-panel.md).

## Not built yet

Project-side row structs and consumers (ConnectIt); themed-widget base; real options menu;
editor-only preview tab.

## Sections

- **`code/`** — inventory only (`code/__INDEX.md`), all rows `stub`. Governed by
  [`_core/_schema/_code.md`](../_core/_schema/_code.md).
- `logs/` — created on first use.

## Start here

[code/__INDEX.md](code/__INDEX.md), then the design note above.
