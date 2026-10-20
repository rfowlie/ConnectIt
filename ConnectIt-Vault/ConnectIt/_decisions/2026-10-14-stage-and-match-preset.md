---
Date: 2026-10-14
status: Active
superseded by:
tags:
  - architecture
  - level-design
  - match-setup
---
## Decision

A level config is **the stage** only: tile and piece registries, grid definition, piece class, pool size. **How a
match is played** -- rules, player loadout, AI profile -- is a separate asset, `UConnectIt_MatchPreset`.

- A level config names the preset it plays as by default (`DefaultMatchPreset`): its Adventure setup, and what
  opening the level directly plays.
- The main menu can choose another preset for the same level (`FConnectItMatchSettings::MatchPreset`), offered from
  `UConnectIt_LevelCatalog::MatchPresets`. The menu's AI profile and target score stay as tweaks on top.
- `AConnectIt_GameMode::EnsureMatchSetupResolved` uses the menu's preset if there is one, else the level's default,
  then applies the tweaks. Everything after that (the live copy, replication to clients) is unchanged.
- Any level can be played with any preset; nothing restricts or validates the pairing yet.

## Why

The owner wants Smash-style reuse: a level designed for an Adventure stage, also playable in multiplayer with rules
chosen in the menu. The level config mixed the place with the way it is played, so reusing the place meant
duplicating it. Chosen over (2) letting the menu send whole override rules (the UI would have to build valid rules
from nothing), (3) one config per level per mode (duplicated board data that drifts) and (4) shared stage sub-levels
(three maps per stage). The owner's reasoning: easier to split now than to find out later it is needed.

## What Would Change It

Stages that only make sense with some presets (then the catalog or the stage lists what it allows, and the GameMode
validates the pairing). Clients choosing settings in a lobby, or a dedicated server: the choice would have to be sent
to the server, since menu settings live on the machine that made them.
