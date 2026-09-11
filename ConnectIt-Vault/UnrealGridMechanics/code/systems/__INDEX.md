---
schema: systems
reconciled: 2026-09-10
commit: 6477d5d
---

# UnrealGridMechanics — systems index

Per-flow narrative pages, each with a Mermaid diagram. Governed by [[_core/_schema/_code|_core/_schema/_code.md]].

| Flow | What it does | Status |
|---|---|---|
| [[UnrealGridMechanics/code/systems/board-shift\|board-shift]] | Compute the wraparound position remap, commit state, animate tiles, finalise | current |
| [[UnrealGridMechanics/code/systems/hover-relay\|hover-relay]] | Registry registers tiles/pieces → subsystem chains per-actor cursor events into one delegate | current |

Candidate flows not yet written (`stub`): tile discovery & board-dimension derivation,
piece spawn/despawn through the pool, cursor manager → selection.

## Related

- Types: [[UnrealGridMechanics/code/__INDEX|code/__INDEX.md]]
- Recipes: [[UnrealGridMechanics/code/recipes/__INDEX|recipes/__INDEX.md]]
