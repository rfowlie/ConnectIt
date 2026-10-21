---
schema: systems
reconciled: 2026-09-10
commit: 6477d5d
---

# game (ConnectIt module) — systems index

Per-flow narrative pages, each with a Mermaid diagram. Governed by [[_core/_schema/_code|_core/_schema/_code.md]].

| Flow | What it does | Status |
|---|---|---|
| [[place-piece-request\|place-piece-request]] | Board-change request: client busy check → server turn / settled / loadout gates → commit → gated visual sequence → outcome back to client; one board change at a time (lock until its visuals finish) | current |
| [[board-state-single-source-of-truth\|board-state-single-source-of-truth]] | One replicated snapshot (prev + current + change event) drives every visual system on server and client | current |
| [[ai-turn\|ai-turn]] | How the AI opponent is assembled (AI director: loadout, strategies, selector) and takes a turn: waits for a settled board → coordinator → selector → Mediator gate → end turn | current |
| [[game-state-machine\|game-state-machine]] | **Retired legacy** — the old non-networked per-turn phase machine; documented for contrast, not live | legacy |

Candidate flows not yet written (`stub`): match start / board initialisation, game-over lockout, the (in-progress) game-event queue.

## Related

- Types: [[ConnectIt/code/__INDEX|code/__INDEX.md]]
- Recipes: [[ConnectIt/code/recipes/__INDEX|recipes/__INDEX.md]]
