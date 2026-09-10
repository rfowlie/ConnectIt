---
schema: recipes
reconciled: 2026-09-07
commit: 668872e
---

# game (ConnectIt module) — recipes index

Step-by-step task procedures. Governed by [[_schema/recipes|_schema/recipes.md]].

| Task | When you'd do it | Status |
|---|---|---|
| [[add-a-scoring-rule\|add-a-scoring-rule]] | Change how a placement scores (a new `IConnectIt_ScoringRule`) | current |
| [[add-a-board-request-type\|add-a-board-request-type]] | Add a new server-validated board mutation (destroy multiplier, swap, capture, …) | current |

Candidate recipes not yet written (`stub`): add a game state-machine phase, add a win
condition, wire a new debug widget, add an influence map for the AI, finish the
game-event queue.

## Related

- Types: [[ConnectIt/code/index|code/index.md]]
- Flows: [[ConnectIt/systems/index|systems/index.md]]
