---
Date: 2026-10-28
status: Active
superseded by:
tags:
  - architecture
  - tile-powers
  - tile-catalog
---
## Decision

**A tile power's effect is its identity.** Each C++ effect (`FConnectItTilePowerEffect`) names its power tag
(`GetPowerTag`, a native `ConnectIt.TilePower.*` tag) and carries only the values it uses. **The catalog keys by the
effect.**

- `FConnectItTilePower` holds `Triggers` (exposed for playtesting), `CooldownTurns`, the tile type's `MultiplierFloor` /
  `MultiplierCeiling` (read for every tile type by `ClampMultiplier`) and `Effect`. It has no `Name`: `GetTag()` is the
  effect's tag.
- **The gate is a value on the effects that want one:** `FConnectItMultiplierGate Gate` on Push, Vacuum and Repel. The
  chain asks `PassesGate` (default: always).
- **`UConnectIt_TileCatalogDataAsset::Entries`** is an array (tile class + power). The tag → entry lookup (`FindEntry`)
  is built from each entry's effect tag; a repeated tag is a validation error and the first entry wins. Builders keep
  their power tag; callers (`GetTileClassFor`, `Defines`, the rules' `FindTilePower`) are unchanged.

## Why

- **One source for the tag:** a map key, a `Name` field and an effect could disagree. Now the effect's tag is the only
  one, so it can't.
- **Only relevant values:** every catalog entry used to show every field (a gate for a Chained tile, for example). A value
  on an effect shows only for that effect.

## What Would Change It

- Tying a tile class to one power the same way (the open question
  [tile-catalog-actor-matching](../_questions/tile-catalog-actor-matching.md)).
- Hiding the trigger mask once playtesting settles each power's triggers (it would move onto the effect).
- A designer-made power with no C++ effect (a data-only power would need a tag again).

Plan: [tile-catalog-effects-plan](../design/tile-catalog-effects-plan.md). Design:
[editor-notes-2026-10-28](../design/editor-notes-2026-10-28.md), item 5.
