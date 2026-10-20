# Stage and match preset

Design + plan note (2026-10-14). Option 1 of the "Smash-style level reuse" discussion, chosen by the owner: split
"the place" from "the match played there", so a level built for Adventure can be played in multiplayer with
different rules chosen in the menu.

## Context

`UConnectIt_LevelConfigDataAsset` holds two different things:

- **The stage** -- tile registry, piece registry, grid definition, piece class, pool size. Read by the board-registry
  bootstrap and the utility library.
- **The match** -- `Rules`, `PlayerLoadout`, `AIProfile`. Read only by `AConnectIt_GameMode::EnsureMatchSetupResolved`.

The main menu can already override the AI profile and target score for a level (`FConnectItMatchSettings`), and the
GameMode owns and replicates the live values. This plan widens that override from two values to the whole match.

## Plan

1. **New asset `UConnectIt_MatchPreset`** (`Framework/Data/`): `DisplayName`, `Description`, `Rules`,
   `PlayerLoadout`, `AIProfile`. A designer-authored "way to play" ("Adventure: Puzzle Swap", "Versus Classic"...).
2. **Level config keeps the stage** and gains `DefaultMatchPreset`: what the level plays as when the menu chooses
   nothing (Adventure, or opening the level directly in the editor). `Rules` / `PlayerLoadout` / `AIProfile` leave the
   level config (migration below).
3. **Menu chooses a preset.** `FConnectItMatchSettings` gains `MatchPreset` (null = the level's default). The existing
   `AIProfile` and `TargetScore` stay as tweaks applied on top. `UConnectIt_LevelCatalog` gains a `MatchPresets` list
   for the menu to offer, next to its `AIProfiles`.
4. **GameMode** (`EnsureMatchSetupResolved`): preset = the menu's (if set for this level) else the level config's
   default; copy rules / loadout / AI profile from it; then apply the tweaks. No preset at all = empty rule set plus a
   warning, as today with no level config.
5. **Not in this change:** restricting which presets a stage offers, and validating that a preset's rules suit the
   stage (e.g. connect length vs board width). Both are easy to add once a real case shows up.

## Migration of the two existing level configs

Owner's choice: **by hand**. The old fields are removed from the level config in code; the owner copies the values
out with the current build first (right-click the property in the Details panel → Copy), then builds, creates the
presets and pastes them in. Until a level config is re-saved its old values are still on disk, and git has them.

## Things to watch (from the discussion)

- Adventure-only content in a level (starting pieces, blockers, level scripting such as a rotating scoring direction)
  must check the match type and stay out of a versus match.
- A versus preset has no AI goals; the adventure preset's AI setup must not leak into it.
- Menu choices live on the machine that made them: fine on a listen server (the host runs the GameMode); a lobby where
  clients choose, or a dedicated server, would need them sent to the server.

## Status (2026-10-14): written, NOT yet built (on purpose)

- All code is in the working tree: `UConnectIt_MatchPreset` (with the rules / loadout / AI validation that used to be
  on the level config), the level config's `DefaultMatchPreset`, `FConnectItMatchSettings::MatchPreset`,
  `UConnectIt_LevelCatalog::MatchPresets`, and the GameMode resolving the preset.
- Not built yet, so the owner can copy the two configs' old values out with the current build first.
- Decision: [stage and match preset](../_decisions/2026-10-14-stage-and-match-preset.md).
