---
schema: systems
reconciled: 2026-09-07
commit: 668872e
---

# game (ConnectIt module) — systems index

Per-flow narratives with diagrams. Governed by [[_schema/systems|_schema/systems.md]].
An overlay on the module's own `old/Source/ConnectIt/Docs/Workflows/` (which these link to).

| Flow | What it does | Status |
|---|---|---|
| [[game/systems/place-piece-request\|place-piece-request]] | Player picks a tile → server validates → board state commits → gated visual sequence → outcome back to client | current |
| [[game/systems/board-state-single-source-of-truth\|board-state-single-source-of-truth]] | One replicated snapshot (prev + current + change event) drives every visual system on server and client | current |
| [[game/systems/game-state-machine\|game-state-machine]] | The per-turn phase progression: select tile → place piece → update board → next turn | current |

Candidate flows not yet written (`stub`): AI turn (blackboard modifier → MinMax → request),
match start / board initialisation, game-over lockout, the (in-progress) game-event queue.

## Related

- Types: [[game/code/index|code/index.md]]
- Recipes: [[game/recipes/index|recipes/index.md]]
