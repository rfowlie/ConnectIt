---
Date: 2026-09-18
status: Active
superseded by: partially — the definition layer (skin/preset asset shapes) is now project-owned, see [2026-09-18-skin-plugin-defines-no-row-shapes-project-owns-them](2026-09-18-skin-plugin-defines-no-row-shapes-project-owns-them.md)
tags:
  - ui
  - visuals
  - skins
  - architecture
---
## Decision

Player-selectable visuals ("skins") are a layered data flow, generic enough to reuse in
future projects. Full walkthrough: [`design/skin-system.md`](../design/skin-system.md).

1. **Definition (developer-authored).** Skin *categories* are `FGameplayTag`s (e.g.
   `Visual.Piece`, `Visual.Tile`, `Visual.ActionFX`, `Visual.UI`) — a new project adds
   categories without new code. A **skin** is a data asset (one base class, typed
   subclasses per category). A **preset** ("universal config") is a data asset mapping
   category → skin.
2. **Selection (player-authored).** A small struct: a preset ID plus a map of
   category → skin ID for overrides ("Preset A, but Preset B's pieces"). Stored as
   **IDs, not asset references**, so a renamed or removed skin falls back instead of
   breaking a save. Persisted in a `USaveGame` slot.
3. **Resolution.** A `UGameInstanceSubsystem` owns the selection, loads/saves it, and
   resolves `ResolveSkin(Category)` in this order: **level-forced > player override >
   player preset > project default preset**. Async-loads soft references, caches, and
   broadcasts `OnSkinChanged(Category)`.
4. **Consumption.** Nothing flows "into" a level — actors and widgets pull from the
   subsystem on construct and bind `OnSkinChanged` for live swaps. C++ exposes data and
   tools only; Blueprint applies them (per the
   [visual-reactions-in-Blueprint convention](2026-09-18-visual-reactions-in-blueprint-convention.md)).
5. **Options menu.** One Preset dropdown, then one dropdown per category with a
   "(preset default)" entry; pending selection with Apply/Revert, live preview via
   `OnSkinChanged`.

## Why

Nothing skin-shaped exists today: visuals are entirely Blueprint
(`AConnectIt_GridPiece::OnFactionVisualUpdate(int32)` has an empty native body,
`PieceActorClass` on the level config is the only C++-visible selector), and there is no
`USaveGame`/`UGameUserSettings`, no options menu, no C++ `UGameInstance`, no Asset Manager
use. So the shape had to be chosen deliberately rather than grown ad hoc per category.

`UGameInstanceSubsystem` specifically: it outlives level travel, exists on the main-menu
level (where the options live), is client-local, and can skip dedicated servers via
`ShouldCreateSubsystem` — the same reasoning the retired
[faction-visuals subsystem](2026-09-09-faction-visuals-subsystem-built-then-removed.md)
recorded. Storing IDs rather than asset refs is what makes "preset plus per-category
override" survive content changes. Categories-as-tags is what makes it reusable across
projects.

## What Would Change It

Split-screen or multiple local players with different skin choices would move the owner
from `UGameInstanceSubsystem` to `ULocalPlayerSubsystem` (the API can take an optional
local-player/owner context to make that a small change). CommonUI/MVVM being enabled later
would change how UI tokens are delivered, not the layers above.
