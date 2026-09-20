# Skin system — player-selectable visuals

Design note (no schema yet, like the rest of `design/`). The plugin core (settings, subsystem, save slot) is built as of slice 2, uncompiled; project
consumers are not. The decisions behind it: [layered data & flow](../_decisions/2026-09-18-skin-system-layered-data-and-flow.md),
[local-only + fixed actor class](../_decisions/2026-09-18-skins-local-only-and-data-on-fixed-actor-class.md),
[new plugin + explicit lists](../_decisions/2026-09-18-skin-system-new-plugin-and-explicit-settings-lists.md),
[plugin defines no row shapes](../_decisions/2026-09-18-skin-plugin-defines-no-row-shapes-project-owns-them.md),
[faction visuals as a curated skin group](../_decisions/2026-09-18-faction-visuals-are-a-curated-skin-group.md).

## Goal

Let a player choose how the game looks — pieces, tiles, action effects, UI — through a
Graphics-style options menu: pick a **universal preset**, then optionally **override
individual categories** ("Preset A, but Preset B's pieces"). Generic enough to reuse in
future projects, so nothing ConnectIt-specific belongs in the core.

## Layers (top-down)

```
Developer Settings (category -> skin table map, preset table, default preset, save slot)
        │
        ▼
Skin tables (per category, PROJECT-defined rows) ──► Preset table (category → skin ID)
        │                                   │
        └───────────────┬───────────────────┘
                        ▼
   Player selection (preset ID + per-category skin IDs)  ⇄  USaveGame slot
                        │
                        ▼
   UGameInstanceSubsystem resolver ── ResolveSkin(Category)
        precedence: level-forced > player override > player preset > project default
                        │  OnSkinChanged(Category)
                        ▼
   Consumers pull on construct + rebind on change:
     pieces/tiles · action effects · UI widgets
```

**Categories** are gameplay tags (`Visual.Piece`, `Visual.Tile`, `Visual.ActionFX`,
`Visual.UI`, …). **Skins** are rows in a **project-defined DataTable**, one table per
category — the plugin never sees inside a row; it only uses the row **name as the stable ID**
and returns a `FDataTableRowHandle` for the project to read with its own struct. **Presets**
are rows of a table whose struct derives from the plugin's one base row
(`FSkinPresetRowBase { category → skin ID }`). **Selection** is stored as IDs, never asset refs,
so a renamed/removed skin falls back to the next precedence level instead of breaking.

> **Plugin/project boundary** ([decision](../_decisions/2026-09-18-skin-plugin-defines-no-row-shapes-project-owns-them.md)):
> the plugin can't know what "changing a skin" means for a project's actors. It owns
> selection, precedence, save/load, `OnSkinChanged`, and level-forced (runtime-only) skins.
> The project owns row shapes, categories' meaning, and how a consumer applies a skin.
> ConnectIt's row structs are designed when it starts consuming the plugin.

## Data flow

1. Boot: the subsystem loads the save slot, loads the skin/preset tables, and
   resolves each category. Any assets a row references are the project's to load.
2. A level starts: it may declare a policy — `AllowPlayer` (default) or `ForceTheme`
   (name a skin group). Adventure levels use `ForceTheme`; online play uses `AllowPlayer`.
3. Actors/widgets construct: each asks the subsystem for its category's row (`GetSkinRowHandle` /
   `FindSkinRow<T>`) and applies it. They bind `OnSkinChanged` for live swaps.
4. Player opens the options menu: changes a dropdown → pending selection → live preview
   via `OnSkinChanged` → Apply (save) or Revert.

Nothing "flows into" a level; the subsystem outlives level travel and everything pulls.

## Consumption per category

- **Pieces / tiles.** A skin is *data on a fixed actor class* (meshes, materials, VFX),
  applied in the existing Blueprint hook (`OnFactionVisualUpdate` becomes a skin lookup
  keyed by faction). Re-apply on pool activation and on `OnSkinChanged` so
  `UActorPoolSubsystem` pools stay valid.
- **Action effects.** Blueprint reacts to the GameEvent tag (existing convention), then
  asks the action-FX skin for the effect keyed by action tag.
- **UI.** Design tokens (colours, fonts, brushes, a `UMaterialParameterCollection`) —
  works with plain UMG since CommonUI/MVVM aren't enabled. A themed-widget base carries a
  category tag and an `OnSkinApplied` Blueprint event; token getters live in a function
  library.

## Faction visuals = a curated skin group

Faction colour/mesh/icon/label per faction slot is just one skin category whose payload is
keyed by faction slot — not its own subsystem. A **skin group** is a curated bundle of
skins that belong together. Adventure mode is the main consumer: a level forces a skin
group so the designer controls the look; the player's own choice applies in online play.

## Networking

Cosmetics are local-only (no replication, nothing on `PlayerState`). Opponent-visible skins
could be added later by replicating a small skin ID; per-faction data lives inside the
project's own row, so the resolver takes no owner/faction context.

## Open items

- Exact `USaveGame` shape and slot naming; whether `UGameUserSettings` is worth involving.
- UI token shape (data table vs data asset vs material parameter collection only).
- The per-faction payload of a piece skin (how many faction slots, what happens with more
  factions than the skin defines).
- Precisely what a "skin group" is as an asset (a preset variant? a separate asset type?)
  and how adventure levels reference one.
- ~~Plugin name / boundary~~ — resolved: `UnrealSkinMechanics`; plugin = selection + save +
  notification, project = row shapes and application (slice 2 built: settings, subsystem, save
  slot; see [decision](../_decisions/2026-09-18-skin-plugin-defines-no-row-shapes-project-owns-them.md)).
- ConnectIt's own row structs per category (designed when the game starts consuming the plugin).
- ~~Catalog validation~~ — built: `USkinSubsystem::GetConfigurationIssues`, surfaced by the
  generic debug panel (`SkinMechanics.ToggleDebug`; `USkinDebugWidget`, code-built, shows each
  category's dropdown + resolved ID/source). See
  [decision](../_decisions/2026-09-18-skin-plugin-ships-a-generic-debug-panel.md). An editor-only
  preview tab is deferred.
- Where the options menu lives relative to the existing main-menu level/widget.
