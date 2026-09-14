---
Date: 2026-09-14
status: Active
superseded by:
tags:
  - level-config
  - robustness
---

## Decision

`UConnectIt_GameUtilityLibrary::GetLevelConfig` now falls back to a new
`UConnectIt_LevelConfigSettings::DefaultLevelConfig` (a `TSoftObjectPtr`, set once in
Project Settings) when the current level's name has no entry in `LevelConfigs`, logging a
`Warning` naming the level and telling you to register it — instead of logging an `Error`
and returning null.

Prompted by a real, hard-to-diagnose bug: a duplicated sandbox map (irregular 8x8 grid,
random tiles removed to pressure-test SWAP against a non-standard layout) was never added
to `ConnectIt_LevelConfigSettings`. The match still started, turn flow still worked, and
nothing crashed — but `GetLevelConfig()` returned null, which cascaded silently into a
null `TileRegistry` (so `PlacePieceAction`'s hover/selection rejected every tile) and an
uninitialised action loadout (so the player had no available actions at all). Diagnosing
it required reading `GetLevelConfig`/`GetTileRegistry`/`PlacePieceAction::IsValidHoverTile`
end to end from the symptom back to the cause.

## Why

Registering a level in `ConnectIt_LevelConfigSettings` is a manual, easy-to-forget step
that's disconnected from the actual level-creation action in the editor — nothing prompts
for it, and the old failure mode gave no indication that *this* was the missing piece
("everything loads, turn starts" is exactly what you'd also see from a dozen unrelated
bugs). Two options were on the table: (a) just document the step as a level-creation
checklist item, or (b) make the missing-registration case degrade gracefully with a loud,
specific warning. (a) relies on memory for something that's easy to forget precisely
*because* it isn't part of the visible level-creation workflow; (b) fixes the failure
mode itself — a new/duplicated level now works out of the box with sane defaults, and if
you actually wanted level-specific rules/loadout, the `Warning` log tells you exactly
what's missing and how to fix it. Chose (b); ended up needing no checklist at all, since
the system now points at its own gap instead of failing silently.

## What Would Change It

- `DefaultLevelConfig` needs to be set once in Project Settings (ConnectIt Level Config)
  before this fallback does anything useful — until then, a level with no `LevelConfigs`
  entry hits the second `Error` branch (`DefaultLevelConfig` also unset) and behaves
  exactly as before. Not yet done as of this note.
- If level-specific config ever becomes load-bearing for correctness (not just tuning —
  e.g. a level that *must* reject the default ruleset outright), a silent fallback would
  be the wrong behavior and this should become a hard failure again, or add a
  `bRequiresExplicitLevelConfig` opt-out per level.
