---
Date: 2026-09-18
status: Active
superseded by:
tags:
  - action-state
  - loadout
  - numbered-actions
---
## Decision

Two changes to the action-config/runtime-state shape from
[2026-09-18 — Generic turn-end requirement system](2026-09-18-generic-turn-end-requirement-system.md),
both made by the owner directly in `Action/ActionConfig.h`:

1. **Numbered actions can be granted mid-match.** `FNumberedActionConfig::StartingMatchUses`
   now defaults to `0` (was `1`, `ClampMin=1`) — a player may not start with the action at
   all. New `MaxHeldUses` (`0` = unlimited) caps how many uses a player can hold at any one
   time, so grants clamp to it. Intended as a gameplay-complexity lever (hoarding vs. using).
2. **Runtime state is split by lifecycle, like the config.** `FActionRuntimeState` is
   replaced by `FPermanentActionRuntimeState` (`UsesThisTurn`, `CooldownTurnsRemaining`) and
   `FNumberedActionRuntimeState` (`UsesRemaining`, `UsesThisTurn`, `CooldownTurnsRemaining`).

## Why

The original decision kept runtime state as one struct on the argument that a permanent
action's unread `MatchUsesRemaining` was "inert, not a mismatched-value bug." The owner
prefers no field to be redundant depending on which kind of action the state describes —
the same principle that already justified splitting the config into two typed arrays. With
mid-match grants and a held-uses cap the two kinds also diverge further (only numbered
actions have a hold/grant lifecycle), so a shared struct would carry more dead weight, not
less.

## What it implies for the still-unbuilt validator

- `StartingMatchUses` must be ≤ `MaxHeldUses` when `MaxHeldUses` is non-zero.
- Grant logic (not yet designed) clamps to `MaxHeldUses` rather than exceeding it.
- Existing rules unchanged: no tag in both/neither config array; each turn-end leaf's
  `RequiredUsesThisTurn` ≤ its action's `MaxUsesPerTurn` when capped.
- A numbered action with `StartingMatchUses = 0` and no grant source is unusable — worth a
  validator warning once grants exist.

## What Would Change It

If per-turn counters end up needing identical handling for both kinds (e.g. one loop over
all runtime states), the split makes that slightly clumsier; a shared base struct holding
`UsesThisTurn`/`CooldownTurnsRemaining` would be the fix, not re-merging the lifecycle-
specific fields.
