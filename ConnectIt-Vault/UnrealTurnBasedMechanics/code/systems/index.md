---
schema: systems
reconciled: 2026-09-10
commit: 3086271
---

# UnrealTurnBasedMechanics — systems index

Per-flow narrative pages, each with a Mermaid diagram. Governed by [[_core/_schema/_code|_core/_schema/_code.md]].

| Flow | What it does | Status |
|---|---|---|
| [[UnrealTurnBasedMechanics/code/systems/turn-end-tag-gate\|turn-end-tag-gate]] | How a turn ends without advancing until every gated async task finishes | current |
| [[UnrealTurnBasedMechanics/code/systems/action-stack-lifecycle\|action-stack-lifecycle]] | How the action stack is reshaped across turn lifecycle + the board-change confirmation freeze | current |

Candidate flows not yet written (`stub`): ready-check & match start, participant
disconnect → reconnect / forfeit, board-change request routing end-to-end (action →
server → `NotifyBoardChangeOutcome`), opponent-turn spectator view.

## Related

- Types: [[UnrealTurnBasedMechanics/code/index|code/index.md]]
- Recipes: [[UnrealTurnBasedMechanics/code/recipes/index|recipes/index.md]]
