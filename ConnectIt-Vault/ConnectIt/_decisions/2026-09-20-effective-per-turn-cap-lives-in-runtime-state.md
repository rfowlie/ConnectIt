---
Date: 2026-09-20
status: Active
superseded by:
tags:
  - action-state
  - playerstate
  - ui
---
## Decision

An action's **effective per-turn use cap lives in the runtime state**, not only in the loadout
config. `MaxUsesThisTurn` is a field on both `FPermanentActionRuntimeState` and
`FNumberedActionRuntimeState`:

- Seeded **once** in `ATurnBasedPlayerState::InitialiseActionState` from the config's
  `MaxUsesPerTurn` (0 = unlimited), and **not** reset each turn — only `UsesThisTurn` resets.
- `CanUseAction` (and so the Mediator gate and `UTurnBasedAction::CanActivate`) reads the runtime
  value; the config lookup only answers "is this action in the loadout".
- `GetActionMaxUsesPerTurn(class)` (BlueprintPure) exposes it to the Actions UI.

The config stays the *base*; the runtime field is the *current effective* value.

## Why

Prompted by the Actions UI: the cap was only on the config asset, which UI reading `PlayerState`
entries can't reach. Putting it in state also allows a temporary change (e.g. 2 -> 1 for a few
turns) that a per-turn reset from config would have silently undone — the first version of the
field did exactly that, being re-copied from config at every turn start. Not a duplicate of the
config value, for the same reason `UsesThisTurn` isn't a duplicate of anything: it's live state
that starts from config.

The temporary-modifier machinery itself is deliberately **not built** — there is no concrete action
or event that needs it yet. It is tracked as the deferred task "Support temporary changes to an
action's max uses per turn" in [`_tasks/active.md`](../_tasks/active.md).

## What Would Change It

If several independent effects need to stack on one action's cap, a single overwritten field stops
being enough and a modifier list (base + effects, recomputed) would replace it; that is part of the
deferred task.
