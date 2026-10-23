---
Date: 2026-10-23
status: Active
superseded by:
tags:
  - tile-powers
  - visuals
---
## Decision

Tile Blueprints react to the board through **one hook per kind of event** (`OnPowerTriggered`, `OnPieceEnter`,
`OnPieceExit`, `OnMultiplierChanged`, `OnLockChanged`, `OnActiveChanged`, `OnChainPartnerChanged` on `AConnectIt_TileBase`),
not one generic `HandleTilePower(FGameplayTag)`. Tile actors are for visuals only; the logic that resolves the board stays in
C++ in the rules.

## Why

- Each hook has typed arguments and a fixed meaning, so a Blueprint implements only what it shows and the tag lookup is not
  its job.
- The hooks take a reaction handle the Blueprint must finish, so the event queue waits for the animation: one shape for every
  event.
- The logic stays plain C++ so the AI's search and `Simulate Operation` can run it (an actor hierarchy cannot).

## What Would Change It

Powers multiplying faster than hooks (many powers needing hooks of their own): the `PowerTriggered` event already carries the
power, so a tile can branch on it inside `OnPowerTriggered`.

Design: [first-tile-powers-plan](../design/first-tile-powers-plan.md) Stage 4.
