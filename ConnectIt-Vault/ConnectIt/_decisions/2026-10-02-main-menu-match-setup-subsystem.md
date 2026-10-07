---
Date: 2026-10-02
status: Active
superseded by:
tags:
  - ui
  - match-setup
  - architecture
---
## Decision

Main-menu match choices travel in **`UConnectIt_MatchSetupSubsystem`** (a `UGameInstanceSubsystem`) as
`FConnectItMatchSettings` (level, AI profile, target score). The menu reads options from a **`UConnectIt_LevelCatalog`**
data asset (levels + AI profiles, set in project settings) and calls `StartMatch`. In the match, the GameMode applies the
target score to the per-match win-condition copy and the AI controller prefers the chosen profile -- but only when the
settings' level is the one loaded. No new GameMode class: Adventure `MatchType` is the vs-AI mode. Prerequisite: the
GameMode now duplicates the level config's rule objects per match instead of using the asset's instances.

## Why

A GameInstance subsystem survives `OpenLevel`, needs no custom GameInstance class, and is easy to call from UMG. Matching
on the level keeps stale choices from leaking into another map or a PIE session started straight into a level. Using the
asset's rule instances live meant any runtime override (like a target score) would have modified the asset itself.

## What Would Change It

Online matches: the same settings would need to reach a remote server, i.e. URL options parsed in `InitGame`.
