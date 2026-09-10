---
schema: systems
reconciled: 2026-09-10
commit: 6477d5d
---

# game (ConnectIt module) — systems index

Per-flow narrative pages, each with a Mermaid diagram. Governed by [[_core/_schema/_code|_core/_schema/_code.md]].

| Flow | What it does | Status |
|---|---|---|
| [[place-piece-request\|place-piece-request]] | Player picks a tile → server validates → board state commits → gated visual sequence → outcome back to client | current |
| [[board-state-single-source-of-truth\|board-state-single-source-of-truth]] | One replicated snapshot (prev + current + change event) drives every visual system on server and client | current |
| [[game-state-machine\|game-state-machine]] | **Retired legacy** — the old non-networked per-turn phase machine; documented for contrast, not live | legacy |

Candidate flows not yet written (`stub`): AI turn (blackboard modifier → MinMax → request),
match start / board initialisation, game-over lockout, the (in-progress) game-event queue.

## Related

- Types: [[ConnectIt/code/index|code/index.md]]
- Recipes: [[ConnectIt/code/recipes/index|recipes/index.md]]
