---
Date: 2026-09-18
status: Active
superseded by:
tags:
  - visuals
  - skins
  - networking
  - pooling
---
## Decision

Two rules for how skins meet the running game:

1. **Cosmetics are local presentation, not replicated.** Each client renders with its own
   selection; nothing about a player's skin choice is replicated or stored on
   `PlayerState`. The resolver API takes an optional owner/faction context so
   "show the opponent's skin" can be added later by replicating a tiny skin ID on
   `PlayerState` without changing the data model.
2. **A skin is data applied to one fixed actor class — never a class swap.** A piece or
   tile skin supplies meshes, materials and VFX that are applied to the existing
   piece/tile actor class. Applying happens on pool activation and on
   `OnSkinChanged` for already-spawned actors.

## Why

(1) matches the standing precedent in the retired
[faction-visuals decision](2026-09-09-faction-visuals-subsystem-built-then-removed.md)
("not replicated, not on `PlayerState`") and the project's symmetric-resolution
philosophy for level-authored data: each machine resolves what it needs locally.
Replicating cosmetics adds a network surface for no gameplay value in v1.

(2) exists because pieces are pooled per client (`UActorPoolSubsystem` keys pools by
`TSubclassOf`). A skin that swapped the actor class would force draining and rebuilding
pools on every change. Data on a fixed class keeps pools valid and swaps cheap. It also
fits the existing hook: `OnFactionVisualUpdate` is already where a Blueprint applies
mesh/material/VFX to a fixed `AConnectIt_GridPiece`.

## What Would Change It

A requirement for opponent-visible skins (rule 1) — a small, additive change. A skin
concept that genuinely needs a different actor class (e.g. a differently-rigged piece)
would force revisiting rule 2 and a pool-drain strategy.
