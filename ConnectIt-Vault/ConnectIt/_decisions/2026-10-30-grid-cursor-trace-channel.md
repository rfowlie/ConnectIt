---
Date: 2026-10-30
status: Active
superseded by:
tags:
  - architecture
  - hover
  - collision
summary: 'Grid hover and clicks trace one project setting (UGridMechanicsSettings::CursorTraceChannel); the plugin ships the "GridCursor" channel (slot 18) and GridTile / GridPiece profiles in its own Config/Engine.ini; tile and piece base classes set their root primitive''s response in C++; the ConnectIt player controller traces it'
---
## Decision

**The cursor traces one channel for the grid, named by one plugin setting. The plugin defines that channel itself.**

- **`UGridMechanicsSettings`** (Project Settings > Plugins > Grid Mechanics) has `CursorTraceChannel`. Its default is
  Visibility, so nothing changes until the game sets it. The per-component and per-piece `CursorTraceChannel`
  properties are gone.
- **The plugin carries the channel.** `Plugins/UnrealGridMechanics/Config/Engine.ini` defines **GridCursor** on
  `GameTraceChannel18` (default response Ignore) and the profiles **GridTile** (blocks it) and **GridPiece** (ignores
  it). UE merges a plugin's `Config/<Branch>.ini` into that project branch (the `{PLUGIN}/Config/{TYPE}.ini` layer).
  A test confirms the engine loads it.
- **The base classes set their own collision:**
  - `AGridTileBase` (block) and `AGridPieceBase` (ignore) set their root primitive's response to the setting's channel
    in `OnConstruction`, so a Blueprint child needs no collision work. In the C++ constructor no root exists yet.
  - A non-primitive root is logged once per class.
  - **Map nodes too:** `UConnectIt_MapNodeComponent::OnRegister` sets its actor's root primitive to block the channel,
    so any actor dressed as a hub node is clickable (owner, 2026-10-30: consistent with tiles).
  - Pieces still ignore the channel on every primitive when play starts.
- **The hover map check** names the channel. It also says when an owner blocks Visibility but not the cursor channel,
  the common mistake after the switch.
- **The ConnectIt player controller** sets `CurrentClickTraceChannel` from the setting at begin play.

## Why

- Anything that blocks Visibility (decals, effects, a backdrop, hub props) could take the cursor. A channel only tiles
  block can't be stolen.
- **One setting rather than a property per component:** this stops components drifting apart.
- **The plugin carries the ini:** the channel arrives with the plugin. Slot 18 keeps clear of the game's own channels,
  which count up from 1.
- **The base classes set collision:** designers and artists get a working tile or piece without touching collision.

## Gotchas

- A collision preset set by hand on a Blueprint's root wins over the C++ default, so clear it.
- A hub node actor whose root isn't a primitive (a scene root with a mesh child) is logged, and its mesh must block
  GridCursor by hand.
- If another plugin or the game uses `GameTraceChannel18`, move one of them.

## Links

- Plan: [grid-cursor-channel-plan](../design/grid-cursor-channel-plan.md); design: [hover-component](../design/hover-component.md).
- Related: [hover through a component](2026-10-28-hover-through-a-component.md),
  [each action is its own cursor manager](2026-10-30-each-action-is-its-own-cursor-manager.md).
