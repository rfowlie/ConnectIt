---
type: plan
status: complete
approved: true
approved-on: 2026-10-28
completed-on: 2026-10-28
playtested: false
design: "[editor-notes-2026-10-28](editor-notes-2026-10-28.md)"
question:
decision:
---

# Plan: World Settings validation by game mode

From [editor-notes-2026-10-28](editor-notes-2026-10-28.md), item 2 (decided 2026-10-28). Written 2026-10-28 in a
design-only session; **approved by the owner 2026-10-28**. **Built 2026-10-28**; awaiting the owner's editor check.

## Context

`AConnectIt_WorldSettings::IsDataValid` warns on every map without a Level Config, a Tile Catalog (when it has a Level
Config) or a Default Match Preset -- noise for the hub and menu maps, which have no match. Decided: **check match data
only when the map's game mode is a match game mode.**

## Stage 1: the check

- Resolve the map's game mode: World Settings' **Default Game Mode override**, else the project's default game mode.
- **Match map** = that class is `AConnectIt_GameMode` or a subclass. Only then run today's three warnings.
- Otherwise: no match-data warnings. (If a non-match map *does* set match data, say nothing -- harmless.)
- A note in the validation output naming which game mode was used ("checked as a match map: BP_ConnectIt_GameMode").
- Tests: none (editor validation); checked by running Data Validation on a match map, the hub and the main menu.

## Stage 2: vault

A line in `level-world-settings.md`'s status and the World Settings code page; log.

## Verification

- Build (editor closed); tests pass.
- Owner, editor: Validate Data on the main menu and hub maps → no match warnings; on a match map missing its preset →
  the warning.
