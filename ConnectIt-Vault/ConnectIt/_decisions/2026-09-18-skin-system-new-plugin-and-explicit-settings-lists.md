---
Date: 2026-09-18
status: Active
superseded by: partially — the lists became arrays of DataTables, and the plugin is named, see [2026-09-18-skin-catalogs-as-datatable-arrays-and-plugin-name](2026-09-18-skin-catalogs-as-datatable-arrays-and-plugin-name.md)
tags:
  - visuals
  - skins
  - plugins
  - developer-settings
---
## Decision

- The reusable parts of the skin system (tag categories, base skin/preset data assets,
  the `UGameInstanceSubsystem` resolver, the save slot, a themed-widget base) live in a
  **new dedicated plugin** — not the empty `UnrealUIMechanics` stub. The game project
  supplies its own categories, skins and consumers.
- Which skins and presets exist is declared as **explicit lists in a `UDeveloperSettings`
  class** (default preset + available presets/skins), mirroring
  `UConnectIt_LevelConfigSettings` (soft-ref assets, `GetDefault<>()`, load with
  fallback-and-warn).
- Asset Manager primary-asset scanning ("drop an asset in a folder and it appears") is a
  later upgrade, not v1.

## Why

Skins cover pieces, tiles, VFX and UI look — it isn't only UI, so framing it inside a UI
plugin would mislead. A separate plugin stays independently extractable like the rest of
the suite, which is the whole point of building it generically for future projects.

Explicit lists reuse a pattern already proven in this codebase, and the project has no
Asset Manager configuration today (the level-config settings header notes none was needed).
Adding it now would be setup cost without a current benefit.

## What Would Change It

Enough skins that maintaining the lists by hand becomes tedious, or content-only
contributors adding skins without touching Developer Settings — that's the trigger to
adopt Asset Manager scanning.
