---
schema: systems
reconciled: 2026-09-10
commit: ec992a2
---

# UnrealGameMechanics — systems index

Per-flow narrative pages, each with a Mermaid diagram. Governed by [[_core/_schema/_code|_core/_schema/_code.md]].

| Flow | What it does | Status |
|---|---|---|
| [[UnrealGameMechanics/code/systems/gated-event-tag-queue\|gated-event-tag-queue]] | Fire a gameplay tag, run every registered reaction in phase order, and don't let downstream proceed until all are done | current |
| [[UnrealGameMechanics/code/systems/actor-pooling-lifecycle\|actor-pooling-lifecycle]] | Get → activate → use → deactivate → release, and why get/activate are separate calls | current |

Candidate flows not yet written (`stub`): turn start/end sequencing via `UGameTurnHandler`,
the single-state `UGameMechanicsStateBase` machine, debug-widget seed-then-react
(`UDWidgetBase`).

## Related

- Types: [[UnrealGameMechanics/code/index|code/index.md]]
- Recipes: [[UnrealGameMechanics/code/recipes/index|recipes/index.md]]
