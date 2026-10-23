# CLAUDE.md — UnrealUnsorted

A holding area plugin: somewhere to drop bits and pieces of code from other plugins or other projects so they can be
digested and moved to their real home later.

## Domain

- **Source:** `../Plugins/UnrealUnsorted/Source/UnrealUnsorted/` — an empty module skeleton (`FUnrealUnsortedModule` with empty
  `StartupModule`/`ShutdownModule`; `.Build.cs` depends only on `Core`, `CoreUObject`, `Engine`). Created 2026-10-23.
- **Status:** **empty on purpose.** `EnabledByDefault` is false and it is not listed in `ConnectIt.uproject`.
- **Type:** game-agnostic scratch plugin.
- **Consumed by:** nothing, and that is the rule (below).

## Purpose

- Migrate code in from other projects or plugins **without deciding yet where it belongs**.
- Each piece is later digested: read, cleaned up, and moved into the plugin or the game module it really belongs to (or
  deleted). The goal is for this plugin to be empty again.

## Limits (the rules)

- **Nothing depends on anything in here.** No other plugin's or module's `.Build.cs` may list `UnrealUnsorted`, no source
  file outside the plugin may `#include` from it, and no game content may reference its classes. If something needs it, it
  is no longer unsorted: move it first.
- **Not shipped, not documented.** No `code/` pages for what is in it; the vault records only this note. Code here may be
  broken, duplicated or half-understood; it may not compile cleanly against the current engine version.
- **No new features are built here.** It is a waiting room for imported code, not a place to start writing.
- If a piece sits here uncompiled and unneeded, keep it out of the build rather than letting it break the project (a
  disabled plugin does not compile).
