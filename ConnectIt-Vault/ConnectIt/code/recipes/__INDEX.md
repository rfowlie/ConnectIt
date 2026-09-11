---
schema: recipes
reconciled: 2026-09-10
commit: 6477d5d
---

# game (ConnectIt module) — recipes index

Step-by-step task procedures. Governed by [[_core/_schema/_code|_core/_schema/_code.md]].

| Task | When you'd do it | Status |
|---|---|---|
| [[add-a-scoring-rule\|add-a-scoring-rule]] | Change how a placement scores (a new `IConnectIt_ScoringRule`) | current |
| [[add-a-board-request-type\|add-a-board-request-type]] | Add a new server-validated board mutation (destroy multiplier, swap, capture, …) | current |

Candidate recipes not yet written (`stub`): add a game state-machine phase, add a win
condition, wire a new debug widget, add an influence map for the AI, finish the
game-event queue.

## Related

- Types: [[ConnectIt/code/__INDEX|code/__INDEX.md]]
- Flows: [[ConnectIt/code/systems/__INDEX|systems/__INDEX.md]]
