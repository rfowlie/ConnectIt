---
type: plan
status: not-started
approved: true
approved_on: 2026-10-30
started:
stages: 4
current_stage: not started
owner_done: false
playtested: false
design: "[hover-component](hover-component.md)"
question:
decision:
requires: "[hover-component-plan](hover-component-plan.md)"
---

# Plan: a dedicated Grid cursor trace channel

Design: [hover-component](hover-component.md), "Open: a dedicated Grid cursor trace channel?" (owner's question and lean,
2026-10-30). Written 2026-10-30 in a design-only session; **approved by the owner 2026-10-30, with one addition: the base classes set their collision in C++** (Stage 2). Best built together with the
editor work in [action-cursors-plan](action-cursors-plan.md) so the collision presets are touched once.

## Context

- Hover rides **Visibility** today: `UGridHoverComponent::CursorTraceChannel` and `AGridPieceBase::CursorTraceChannel`
  both default to `ECC_Visibility`, and the player controller's click trace channel is the default. Pieces set themselves
  to Ignore it (so the tile under a piece takes the hover); the hover component warns if its owner blocks nothing on it.
- Anything else that blocks Visibility (decals, effects, a backdrop, the hub's node actors) can take the cursor.
- A trace channel is a **project** setting, so the plugin reads the channel from one setting and the game defines it.
- **Can a plugin carry the ini?** Checked in UE 5.5 (`FConfigCacheIni::AddPluginToBranches`): files in a plugin's `Config/`
  folder are merged into the matching project config branch (the branch is the file's base name with the plugin's name
  taken off, e.g. `<PluginName>Engine.ini` -> Engine). So `[/Script/Engine.CollisionProfile]` channel and profile entries
  *can* ship in the plugin. Not yet verified: that a project-folder plugin's exact filename is picked up (a Stage 1 test
  settles it). The catch is the numbering: custom channels are slots `GameTraceChannel1..18`, and a plugin claiming a slot
  can clash with one the game already uses. Lean: **the plugin ships the ini only if the test works, on a high slot
  (e.g. 18) the game does not use; otherwise the game's `DefaultEngine.ini` defines it** and the plugin only reads the
  setting.

Each stage builds and passes the tests before the next; the editor must be closed for builds; the owner commits between
stages.

## Stage 1: one setting, and the channel definition

- **`UGridMechanicsSettings`** (`UDeveloperSettings`, plugin, Project Settings > Plugins > Grid Mechanics) with
  `CursorTraceChannel` (default `ECC_Visibility`, so nothing changes until the game sets it) and a static
  `GetCursorTraceChannel()`.
- Define the channel **"GridCursor"** and two collision profiles, **GridTile** (blocks GridCursor, Ignores the rest of
  what tiles should ignore) and **GridPiece** (ignores GridCursor, keeps its other responses): in the plugin's
  `Config` ini if the filename test passes, else in the game's `Config/DefaultEngine.ini`. Test: launch the editor and
  read the channel name in Project Settings > Collision.
- Tests: the setting defaults to Visibility; `GetCursorTraceChannel()` returns what is set.

## Stage 2: the plugin reads the setting

- `UGridHoverComponent` and `AGridPieceBase` drop their own `CursorTraceChannel` property and read
  `UGridMechanicsSettings::GetCursorTraceChannel()`. (Class redirects not needed: a removed property is dropped at load; a
  Blueprint that set it is flagged in the owner checks.)
- **The base classes set their own collision in C++, so a new tile or piece is set up right away for designers and
  artists:** `AGridTileBase` and `AGridPieceBase` apply the **GridTile** / **GridPiece** behaviour to their root
  primitive (block / ignore the Grid cursor channel from the setting, other responses as the profiles say) when they are
  constructed and again at `OnConstruction`, so a Blueprint child inherits it with no collision work. The responses are
  set per channel from the setting (not by profile name), so they work even if the profile ini did not ship; the named
  profiles remain for components a Blueprint adds. A Blueprint can still override a response deliberately (the hover
  validation warns if the result blocks nothing). If the root is not a primitive, the class logs once and leaves it.
- The hover component's validation warning names the channel from the setting, and also warns when its owner blocks
  Visibility **but not** the cursor channel (the common mistake after the switch).
- Tests: with the channel set to GridCursor, a primitive that blocks it passes validation and one that only blocks
  Visibility warns; a spawned tile blocks the cursor channel and a spawned piece ignores it with no Blueprint changes.

## Stage 3: the controller traces on it

- The ConnectIt player controller sets `CurrentClickTraceChannel` from the setting at begin play, so cursor-over and click
  events use the Grid channel. The hub's node clicks keep working because node actors block the Grid channel too (or get
  their own channel later -- out of scope).
- The hover subsystem's tile lookups (if any trace manually) use the same channel.
- Tests: none without a world; covered by the owner checks.

## Owner checks: content migration

- In Project Settings > Collision, confirm the "GridCursor" channel and the GridTile / GridPiece profiles exist (or add
  them if the ini did not ship).
- Set **Cursor Trace Channel** to GridCursor in Project Settings > Plugins > Grid Mechanics.
- Open the tile and piece Blueprints once: they inherit the C++ collision now, so **clear any collision preset or
  response you set by hand** that overrides it (a preset on the root component wins over the default). Anything
  clickable in the hub (node actors) must block GridCursor.
- Remove any Blueprint override of the old per-component `Cursor Trace Channel`.
- PIE: hover and click tiles on a match board with pieces on them; hover and click a hub node; put a large mesh that
  blocks Visibility over the board and confirm it no longer steals the cursor.

## Stage 4: vault

Decision note (the Grid cursor channel and the one setting; whether the plugin carries the ini); update
[hover-component](hover-component.md), the hover-relay system page and the cursor recipe; log.

## Not in this plan

- A separate channel for hub nodes (they can share Grid cursor for now).
- Mouse picking for anything outside the grid framework.

## Verification

- Each stage: build (editor closed); `Automation RunTests` passes.
- Owner, PIE: hover and click work on the board and the hub with the Grid channel set; reverting the setting to Visibility
  restores the old behaviour.
