---
schema: systems
reconciled: 2026-09-06
commit: 3086271
---

# UnrealGridMechanics — systems index

Per-flow narratives with diagrams. Governed by [[_schema/systems|_schema/systems.md]].
An overlay on `Docs/Systems.md`.

| Flow | What it does | Status |
|---|---|---|
| [[UnrealGridMechanics/systems/board-shift\|board-shift]] | Compute the wraparound position remap, commit state, animate tiles, finalise | current |
| [[UnrealGridMechanics/systems/hover-relay\|hover-relay]] | Registry registers tiles/pieces → subsystem chains per-actor cursor events into one delegate | current |

Candidate flows not yet written (`stub`): tile discovery & board-dimension derivation,
piece spawn/despawn through the pool, cursor manager → selection.

## Related

- Types: [[UnrealGridMechanics/code/index|code/index.md]]
- Recipes: [[UnrealGridMechanics/recipes/index|recipes/index.md]]
