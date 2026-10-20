---
Date: 2026-10-07
status: Active
superseded by:
tags:
  - architecture
  - replication
  - match-setup
---
## Decision

The level config asset is only a level's **starting template**. The server's `AConnectIt_GameMode` reads it once
(`EnsureMatchSetupResolved`, with main-menu choices applied), then **owns the live match setup** and is the only reader
of the config's rules / player loadout / AI profile. Because a GameMode doesn't exist on clients, the live values are
published where clients can reach them:

- **Match-wide** (rules, AI opponent) → a read-only replicated mirror on `AConnectIt_GameState`
  (`GetMatchRules`, `GetOpponentProfile`, `OnMatchRulesChanged`). `AConnectIt_GameMode::ModifyRules` is the one way to
  change rules after setup; it refreshes the mirror.
- **Per-player** (loadout, action state) → the player's `ATurnBasedPlayerState` (already replicated). Each local
  `AConnectIt_PlayerController` builds its action stack from its PlayerState's loadout (`OnLoadoutChanged`), not from
  the config. The GameMode is the only seeder of a human's action state; `UTurnBasedActionsComponent` no longer seeds.
- **Fixed level setup** (registry / grid templates, piece class, pool size) → still read straight from the asset, via
  narrow accessors.

`UConnectIt_GameUtilityLibrary::GetLevelConfig` (public, Blueprint-callable, whole asset) is removed; the lookup is
`UConnectIt_LevelConfigSettings::FindLevelConfig` (C++ only), intended for the GameMode and the board-registry
bootstrap.

## Why

Anything could read the starting template and get out-of-date values -- already true for the menu's target score and
AI profile, and it would get worse with rules that change mid-level or a player's actions changing. Each client was
building its action stack straight from the config's `PlayerLoadout`. One authority plus replicated mirrors keeps every
reader (UI, clients, AI) on the same live values. The owner's original idea was an interface on the GameMode; it became
GameState + PlayerState because clients cannot reach a GameMode.

## What Would Change It

Per-player loadouts that differ at setup (the GameMode would resolve one per player instead of one for all); or a need
for clients to read fixed setup before the asset is loaded locally.
