---
type: plan
status: complete
approved: true
approved-on: 2026-10-21
completed-on: 2026-10-21
playtested: false
design:
question:
decision:
---

# Plan: each map names its board and default preset in World Settings

Written 2026-10-21 from the owner's notes; approved and **built** the same day (47/47 tests); PIE pending. Log:
[_logs/2026-10-21-0145](../_logs/2026-10-21-0145.md).

**2026-10-28:** World Settings validation only warns about missing match data (level config, tile catalog, default
preset) on a **match map** -- one whose game mode (its override, else the project default) is `AConnectIt_GameMode` or a
subclass; the hub and menu maps get an info line instead. [world-settings-validation-plan](world-settings-validation-plan.md).

## Context

The level config holds the board (registries, grid geometry, piece class / pool) -- values likely to be reused across many
levels -- **and** `Default Match Preset`, which is per level. Having both on one asset forces a new level config per level even
when the board is identical. Reversing the dependency (the preset names its board) would break menu modes ("Versus Classic"
on any board would need a preset per board) and a map would still need to find its preset.

Settled (owner, 2026-10-21): the thing that is one-per-level is **the map**, so the pairing lives on the map, in a custom
**World Settings** class: `Level Config` (the board) and `Default Match Preset` (how the level plays unless the menu picks
another). Both assets become reusable; the level config is the reusable board asset from the wishlist; the match preset is the
per-adventure-level asset (or a shared mode). The name-keyed `LevelConfigs` registry in project settings (paths as strings in
`DefaultGame.ini`, which don't follow folder moves) retires.

Found while planning: `DefaultGame.ini`'s `LevelConfigs` is keyed `CILC_Swap1A`, but the lookup keys by **map** name
(`CIL_Puzzle_Swap_01`), so the swap level currently falls back to `DefaultLevelConfig` (`CILC_Test`). This plan removes that
registry, which fixes it.

The editor must be closed for builds; the owner commits; nothing is committed by me.

## Stage 1: World Settings pairing (code + config)

- **New `AConnectIt_WorldSettings`** (`Public/Framework/Data/ConnectIt_WorldSettings.h` + cpp), deriving `AWorldSettings`:
  - `Level Config` (`TObjectPtr<UConnectIt_LevelConfigDataAsset>`, category "ConnectIt") -- the board this map plays on.
  - `Default Match Preset` (`TObjectPtr<UConnectIt_MatchPreset>`) -- how it plays unless the menu chooses another.
  - `IsDataValid`: a warning for each unset (the project defaults are used).
- **`Config/DefaultEngine.ini`**: `[/Script/Engine.Engine] WorldSettingsClassName=/Script/ConnectIt.ConnectIt_WorldSettings`.
- **Lookups** (`UConnectIt_LevelConfigSettings`):
  - `FindLevelConfig(WorldContext)`: the world's `AConnectIt_WorldSettings::LevelConfig`; else `DefaultLevelConfig` (warning,
    as today); else null (error).
  - New `FindDefaultMatchPreset(WorldContext)`: the world's `DefaultMatchPreset`; else a new settings fallback
    `DefaultMatchPreset` (warning); else null.
  - `LevelConfigs` (the map-name registry) removed; its `DefaultGame.ini` line removed.
- **`UConnectIt_LevelConfigDataAsset`**: `DefaultMatchPreset` and its validation removed; header comment: the reusable board.
- **`AConnectIt_GameMode::EnsureMatchSetupResolved`**: preset = the menu's choice, else `FindDefaultMatchPreset`.
- Unchanged readers, now through World Settings: `UConnectIt_BoardRegistrySubsystem` (at world begin play -- World Settings
  exist then), `UConnectIt_GameUtilityLibrary::GetPieceActorClass` / `GetPiecePoolInitialSize` (also on clients: the map
  carries its World Settings everywhere).
- Comments: `FConnectItLevelCatalogEntry::Level`, `FConnectItMatchSettings::MatchPreset`, `UConnectIt_MatchPreset`.
- Tests: the existing suite still passes; World Settings lookups need a loaded map -- PIE (below). (If cheap, a test that a
  world with no ConnectIt World Settings falls back to the defaults.)

## Stage 2: vault

- `design/stage-and-match-preset.md`: update (the pairing moved to World Settings; the wishlist's reusable board is the level
  config); `design/content-folders.md` (the `DefaultGame.ini` path step retires; only the fallbacks remain there).
- Recipes `set-up-a-custom-ai-opponent.md` (step "confirm the level config's Default Match Preset" -> the map's World Settings),
  `code/systems/ai-turn.md` setup step.
- Decision note (partly supersedes `2026-10-14-stage-and-match-preset`), index line; log.

## Owner-side (after Stage 1)

- Open each map (`CIL_Test`, `CIL_Puzzle_Swap_01`): **World Settings > ConnectIt**: set `Level Config` and `Default Match
  Preset`; save. If a map still shows the engine's World Settings class, check Project Settings > Engine > General > World
  Settings Class, then reopen the map (Unreal replaces a map's World Settings of the wrong class on load -- verify the values
  stuck after saving).
- The level configs lose `Default Match Preset` (dropped on load): note them first.
- Optional: project settings > ConnectIt Level Config > `Default Match Preset` for maps with no World Settings entry (e.g. a
  sandbox).

## Verification

- Build (editor closed) and `Automation RunTests ConnectIt`: all pass.
- PIE each map, log filtered to `ConnectIt_`: `match setup resolved -- preset '<the map's preset>'`, and no
  `falling back to DefaultLevelConfig` warning; the board initialises (registry subsystem has its config); the swap level now
  uses `CILC_Swap1A`.

## Risks / notes

- Changing the World Settings class touches every map; Unreal's handling of existing maps' old-class World Settings should be
  checked on the first map opened (values may need re-entering).
- Main menu: the catalog's entry is the map; its default preset is now read when the map loads (no menu change needed).

## Follow-up (2026-10-21): the ConnectIt project settings are deleted

Asked by the owner once the World Settings were in. `UConnectIt_LevelConfigSettings` held only the two fallbacks and the
menu's catalog pointer (unused: `None`, no Blueprint called `Get Level Catalog`), so it went:

- The lookups moved to `AConnectIt_WorldSettings::FindLevelConfig` / `FindDefaultMatchPreset` (callers: the GameMode, the
  board-registry subsystem, the piece-pool accessors).
- No fallbacks. Duplicating a map copies its World Settings, which covers the case the 2026-09-14 fallback existed for. A map
  with no board is fine for a menu map: the board-registry setup logs a warning naming the World Settings field, and
  validation warns; the lookup itself is silent.
- `UConnectIt_MatchSetupSubsystem::GetLevelCatalog` removed: the main menu will reference its catalog asset directly.
- `DefaultGame.ini`'s ConnectIt section removed.

Decision: [2026-10-21-no-connectit-project-settings](../_decisions/2026-10-21-no-connectit-project-settings.md).
