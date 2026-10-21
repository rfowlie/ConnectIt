---
schema: recipes
reconciled: 2026-10-19
commit: fd369a4
---

# game (ConnectIt module) — recipes index

Step-by-step task procedures. Governed by [[_core/_schema/_code|_core/_schema/_code.md]].

| Task | When you'd do it | Status |
|---|---|---|
| [[add-a-scoring-rule\|add-a-scoring-rule]] | Change how a placement scores (a new `IConnectIt_ScoringRule`) | current |
| [[set-up-a-custom-ai-opponent\|set-up-a-custom-ai-opponent]] | Designer-facing: the AI opponent through the match preset's AI director -- tune the built-in one, make a Blueprint strategy, or a Blueprint AI controller | current |
| [[combine-ai-strategies\|combine-ai-strategies]] | Choose an AI's strategies and selector: several strategies at once, the stock selectors (First Available, Top N, Greedy Scorer, Weighted by Confidence), your own selector | current |
| [[direct-the-ai-during-a-match\|direct-the-ai-during-a-match]] | Change the AI's loadout / strategies / selector during a match (per turn, at checkpoints, on anything), incl. the tell flow | current |
| [[add-a-win-condition\|add-a-win-condition]] | Set up match conditions: goals as condition assets (in order per side), the condition types (incl. Not Enough Pieces To Score, No Legal Move), reacting to checkpoints, a new condition type or legal-move support in C++ | current |
| [[add-a-board-request-type\|add-a-board-request-type]] | Add a new server-validated board mutation (destroy multiplier, swap, capture, …) | current |

Candidate recipes not yet written (`stub`): add a game state-machine phase, wire a new
debug widget, add an influence map for the AI, finish the
game-event queue.

## Related

- Types: [[ConnectIt/code/__INDEX|code/__INDEX.md]]
- Flows: [[ConnectIt/code/systems/__INDEX|systems/__INDEX.md]]
