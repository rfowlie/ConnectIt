# CLAUDE.md — UnrealEditorMechanics

Editor-only tools: Editor Utility Widgets, Content Browser helpers and other authoring tools that are not tied to one gameplay
plugin. Its module is **Editor** type, so it is built and loaded only in the editor and never ships in a packaged game.

## Domain

- **Source:** `../Plugins/UnrealEditorMechanics/` — one Editor module (`UnrealEditorMechanics`), with an empty source skeleton
  (`FUnrealEditorMechanicsModule`, empty `StartupModule`/`ShutdownModule`). The `.Build.cs` depends on `Core` (public) and
  `CoreUObject`, `Engine`, `UnrealEd` (private). Created 2026-10-30.
- **Status:** skeleton. Nothing moved in yet. The Asset Naming Keeper is still a content asset under `Content/AssetNamingKeeper/`,
  and will move here when it is ready (see [AssetNamingKeeper](../UnrealEngineTools/AssetNamingKeeper.md)).
- **Type:** editor-only plugin. Not game code, and game-agnostic.
- **Consumed by:** nothing. It is not enabled in `ConnectIt.uproject`, so it does not build with the game.

## Rules

- **Editor only.** Nothing at runtime may depend on this plugin or include from it. Runtime code that needs an editor helper
  must not reach into here.
- **What belongs here:** editor tools not owned by one gameplay plugin (the naming tool, Content Browser helpers, editor utility
  widgets). A tool that belongs to one plugin stays with that plugin: the grid validator is in `UnrealGridMechanics`, for example.
- **Content is allowed** (`CanContainContent` is true): Editor Utility Widgets and their enums live in this plugin's `Content/`
  once moved.
- **Tools from other projects** are reviewed in [UnrealEngineTools](../UnrealEngineTools/CLAUDE.md) first, then moved here when
  they are ready.

## Sections

None yet. Create `code/` (per [`_core/_schema/_code.md`](../_core/_schema/_code.md)) once there is C++ to document.

## Next steps

- Move the Asset Naming Keeper into `Plugins/UnrealEditorMechanics/Content/` once its enum is fixed and it has been tested.
  Set up redirects for the old asset path.
- Review `ConnectIt_LayoutToolLibrary` (currently in the game module, editor-only) for a move here.
- Enable the plugin in `ConnectIt.uproject` only when it has content or code that the editor needs.
