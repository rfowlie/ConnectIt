---
Date: 2026-09-09
status: Active
superseded by:
tags:
  - ui
  - scope
  - white-box
---

## Decision

A full faction-visuals implementation — `UConnectIt_FactionVisualsSubsystem`
(`UGameInstanceSubsystem`) + `UConnectIt_FactionVisualsDataAsset` (`UDataAsset` palette
keyed by faction slot) + a `UDeveloperSettings` Project Settings page, presentation-only,
skipping dedicated servers — was **built and then deliberately removed** at the project
owner's request. The project is in a white-box phase; keeping the codebase lean took
priority over having the colour/icon/label accessor ready early.

Recorded design intent for when it's picked back up:

- **Not** replicated, **not** on `PlayerState` — a faction's colour/icon is a local
  presentation choice, not game state.
- `UGameInstanceSubsystem` specifically — not `UWorldSubsystem` (the palette is immutable
  config, not per-world data), not `ULocalPlayerSubsystem` (unreachable from a
  board/piece actor, which has no local player).
- `UDataAsset` palette keyed by faction slot index; `UDeveloperSettings` config page;
  `ShouldCreateSubsystem` skips dedicated servers.
- Do **not** migrate the legacy `PDA_PlayerColours` / `ConnectIt_PlayerColours` assets —
  they belong to the dead MVVM branch
  ([[ConnectIt/_decisions/2026-09-08-retire-legacy-mvvm-pipeline|retire-legacy-mvvm-pipeline]]).
  Copy colour values by hand if matching the old look is wanted.

## Why

The accessor has no consumer yet — no scoreboard, results screen, or board overlay
currently needs faction colour on the networked path. Carrying a built-but-unused
subsystem + data asset + settings page ahead of need is exactly the kind of weight the
owner wants to avoid during white-boxing. The design is cheap to rebuild from this note
when a real UI consumer arrives.

## What Would Change It

- The first production UI element that needs faction colour / icon / label (scoreboard,
  results screen, turn HUD, board overlay) — build the subsystem per the intent above at
  that point.
