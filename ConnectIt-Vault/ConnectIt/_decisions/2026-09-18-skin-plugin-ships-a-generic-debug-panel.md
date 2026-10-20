---
Date: 2026-09-18
status: Active
superseded by:
tags:
  - visuals
  - skins
  - plugins
  - debugging
---
## Decision

`UnrealSkinMechanics` ships a **generic, unstyled debug panel** for verifying a skin setup —
`USkinDebugWidget` (+ one `USkinDebugRow` per category), built entirely in C++ (no asset), toggled
with the console command `SkinMechanics.ToggleDebug` or the Blueprint-callable
`USkinDebugWidget::ToggleDebugWidget`. It lists every configured category with a dropdown of its
skin IDs (`(preset default)` clears the override) and the resolved ID + which layer produced it,
plus a preset dropdown, Save / Revert, and a red list of configuration issues.

To feed it, `USkinSubsystem` gained:

- `GetSkinResolution(Category)` → `FSkinResolution { SkinId, ESkinSource }` (source = none /
  level-forced / override / preset / default preset). `ResolveSkinId` is now a thin wrapper over
  the same single precedence implementation.
- `GetConfigurationIssues()` → human-readable problems: empty/missing/row-less tables, bad or
  unset preset table, missing/unknown default preset, presets naming unknown categories or skin
  IDs, categories that resolve to nothing. **This delivers the previously deferred catalog
  validation.**

The panel drives the same `Set*` / `Save` / `Reload` API a real options menu will, and knows
nothing about what a skin is — it stays inside the plugin/project boundary.

## Why

Setup is data spread across settings, several DataTables and a preset table; mistakes are
silent (a bad ID just falls through precedence). A visualization the owner can open in PIE shows
what each category actually resolves to and why. Code-built and runtime-only keeps it zero-asset
and usable in any project, and doubles as a smoke test of the subsystem.

## What Would Change It

An editor-only tab (no PIE needed, static resolution from settings) is the deferred alternative;
it could reuse the issue-list logic. A real, styled options menu is project work and does not
replace this.
