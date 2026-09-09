---
schema: systems
reconciled: 2026-09-06
commit: 3086271
---

# UnrealTurnBasedMechanics — systems index

Per-flow narratives with diagrams. Governed by [[_schema/systems|_schema/systems.md]].
An overlay on `old/Plugins/UnrealTurnBasedMechanics/Docs/Systems.md` (authoritative narrative) that adds the figure, the
named-hop walkthrough, and the cross-impact list.

| Flow | What it does | Status |
|---|---|---|
| [[UnrealTurnBasedMechanics/systems/turn-end-tag-gate\|turn-end-tag-gate]] | How a turn ends without advancing until every gated async task finishes | current |
| [[UnrealTurnBasedMechanics/systems/action-stack-lifecycle\|action-stack-lifecycle]] | How the action stack is reshaped across turn lifecycle + the board-change confirmation freeze | current |

Candidate flows not yet written (`stub`): ready-check & match start, participant
disconnect → reconnect / forfeit, board-change request routing end-to-end (action →
server → `NotifyBoardChangeOutcome`), opponent-turn spectator view.

## Related

- Types: [[UnrealTurnBasedMechanics/code/index|code/index.md]]
- Recipes: [[UnrealTurnBasedMechanics/recipes/index|recipes/index.md]]
