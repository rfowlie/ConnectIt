---
Date: 2026-10-02
status: Active
superseded by:
tags:
  - ai
  - data
---
## Decision

An AI opponent is one **`UConnectIt_AIProfile`** data asset: display name, description, the AI's **loadout**, and an
`Instanced` **strategy** (configured inline). One asset type serves every strategy -- no paired data-asset class per
strategy. The level config references a default `AIProfile` (replacing its inline `AIStrategy` and `EnemyLoadout`);
the main menu's match setup can override it. Each AI controller duplicates the profile's strategy for its own match.

## Why

Presets (Classic Easy / Hard, later Adventure opponents) become saved, swappable assets the menu can list. A strategy's
own properties already are its configuration, so a per-strategy data-asset class would only re-create a parallel
hierarchy. Moving the loadout onto the profile makes one asset fully describe an opponent.

## What Would Change It

A need to share one strategy configuration across many profiles with different loadouts (then strategies themselves
might become assets referenced by profiles).
