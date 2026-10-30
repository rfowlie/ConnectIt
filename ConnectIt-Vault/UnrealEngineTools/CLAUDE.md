# CLAUDE.md — UnrealEngineTools

Notes on **outside editor tools**: Unreal tools built in other projects (Blueprint editor utilities, Content Browser helpers,
and similar) that we are investigating to see whether they should come into our framework. Each tool gets a note here, named
after the tool. The tools are reviewed from their assets and notes first; nothing here is copied game code.

## What's here

- [`AssetNamingKeeper.md`](AssetNamingKeeper.md) — review of the Asset Naming Keeper, an Editor Utility Widget that applies
  prefixes and suffixes to selected assets. Origin: the Spell Thief project.
- [`PathTracer.md`](PathTracer.md) — the Path Tracer demo. Its Blueprints and assets are in
  [UnrealPathMechanics](../UnrealPathMechanics/CLAUDE.md); the goal is to convert them to C++ and find each part a home.

## Domain

- **Source:** each tool's assets live where the tool lives, not here. The Path Tracer is in `../Plugins/UnrealPathMechanics/`.
  The Asset Naming Keeper's assets are **not** in this repo (no `Content/AssetNamingKeeper/` in ConnectIt), so its note
  reviews what was available; they must be brought in or confirmed before it is worked on.
- **Status:** early. Both notes are reviews and goals, from the assets and notes only. Neither tool has been run in the editor.
- **Type:** notes about external tools, not a plugin and not game code. Nothing here is compiled or enabled.
- **Consumed by:** nothing. A tool that is brought in becomes a plugin or game code, with its own domain.

## Sections

None: this domain is notes only. Create `code/` (per [`_core/_schema/_code.md`](../_core/_schema/_code.md)) only if a tool
becomes code here, which it should not do.

## Rules

- A tool's source assets stay in the game repo or the plugin that owns them. This domain holds notes, not copies.
- Each tool gets its own note, named after the tool.
- A tool becomes code in its own domain (a plugin with its own `CLAUDE.md`) once it is converted and placed. Until then it stays
  here as notes.
- Editor-only tools (Editor Utility Widgets, Content Browser helpers) go to [UnrealEditorMechanics](../UnrealEditorMechanics/CLAUDE.md) once they are ready to move.

## Next steps

- Bring the Asset Naming Keeper assets into the repo, or confirm where they live, then run the tests listed in its note.
- Work the Path Tracer goals in [`PathTracer.md`](PathTracer.md), phase 1 first.
