---
Date: 2026-09-18
status: Active
superseded by:
tags:
  - action-state
  - loadout
  - turn-end
---
## Decision

Action config and turn-end leaves reference their action by **`TSubclassOf<UTurnBasedAction>`**,
not by a typed `FGameplayTag`:

- `FPermanentActionConfig::ActionClass` and `FNumberedActionConfig::ActionClass` replace
  `ActionTag`.
- `UTurnEndRequirementLeaf::ActionClass` replaces `ActionTag`.
- The tag is **derived** from the class default object via the new static
  `UTurnBasedAction::GetTagForClass(TSubclassOf<UTurnBasedAction>)` (null-safe; invalid tag if
  the class is null or doesn't implement `GetActionTag`). Runtime lookup
  (`FindActionByTag`, the tree's uses lookup) stays tag-based.
- **Tags are not removed.** They remain the identity for requests (`Request.RequestType`),
  history, debug widgets, input and UI — they just stop being *authored* in config.

Supersedes the tag-keyed shape in
[2026-09-18 — Generic turn-end requirement system](2026-09-18-generic-turn-end-requirement-system.md)
(config structs and leaf).

## Why

Proposed by the owner: a tag in config is a second, hand-typed link to something the action
class already declares, open to mismatch, and something would have to map tag → class to
instantiate. Checked against the code: no such map exists today —
`CloneActionsFromLoadout` duplicates the loadout's Instanced `Actions` entries and
`FindActionByTag` scans them — and `GetActionTag()` is already a per-class
`BlueprintNativeEvent`. So config's tag was purely redundant.

The tamper worry doesn't attach to where the class↔tag relation lives: the loadout is a
static data asset identical on every machine, and enforcement is server-side (the Mediator
dispatches on `RequestType` and reads budgets from `PlayerState`). A relation stored on the
action class is no more exposed than the current `Actions` array.

## Consequences

- **Board Shift (and any parameterised action) needs one Blueprint subclass per variant.**
  `ShiftDirection` is a per-instance property; class-as-identity means `BoardShift_Up`,
  `BoardShift_Left`, … each carrying their own defaults. Previously two same-class entries
  also shared one tag, so `FindActionByTag` only ever found the first — this makes the
  constraint explicit rather than newly imposed.
- Leaves/config already authored with a tag are dropped by the type change. None had been
  authored at the time (tree never tested in PIE).
- The config arrays are still unread at runtime. Step 2 (component builds instances from
  them with `NewObject`, replacing the Instanced `Actions` array, moving
  `MaxCompletionsPerTurn`/`CooldownTurns`/`bIsRequired` into config) is not done.
- Validator (still unbuilt) gains: duplicate class within a loadout; leaf class absent from
  the loadout; class whose `GetActionTag` is unimplemented.

## What Would Change It

If an action's tag ever needs to differ per *instance* of one class (rather than per
subclass), tag-from-CDO breaks; the fix would be an explicit per-entry tag override, not
reverting to tag-as-key.
