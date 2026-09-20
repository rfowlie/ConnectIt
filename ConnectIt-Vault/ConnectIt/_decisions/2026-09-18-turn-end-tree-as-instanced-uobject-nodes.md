---
Date: 2026-09-18
status: Active
superseded by:
tags:
  - turn-end
  - loadout
  - unreal
---
## Decision

The turn-end requirement tree is built from **Instanced `UObject` nodes**, not the
recursive `FTurnEndRequirementNode` struct written into
[2026-09-18 — Generic turn-end requirement system](2026-09-18-generic-turn-end-requirement-system.md).
That note's semantics are unchanged; only the node's *representation* is replaced:

- `UTurnEndRequirementNode` — abstract base; carries `bIsEnabled` (disabled = skipped, as
  if absent) and a virtual `Evaluate`.
- `UTurnEndRequirementLeaf` ("Action Used") — `ActionTag` + `RequiredUsesThisTurn`.
- `UTurnEndRequirementGroup` ("Group") — `Mode` (`Any`/`All`) + `Instanced` `Children`.
- `UActionLoadoutDataAsset::TurnEndRequirements` is an `Instanced` root node pointer.
  Unset = the component's legacy `bIsRequired` / `RequiredActionTagA/B` behaviour.

Lives in `UnrealTurnBasedMechanics` (`Action/TurnEndRequirement.h`), alongside the
unchanged `FPermanentActionConfig` / `FNumberedActionConfig` / `FActionRuntimeState`
structs in `Action/ActionConfig.h`.

## Why

Unreal Header Tool rejects the struct as written: a `USTRUCT` holding
`TArray<FTurnEndRequirementNode> ChildGroups` fails with "'Struct' recursion via arrays is
unsupported for properties." Found when the first build of the implementation ran; the
design conversation had assumed it would work. Options considered: fixed-depth structs
(caps nesting, duplicates fields per level), a flat array with parent indices (awkward to
author), and Instanced UObjects — chosen because it gives unlimited nesting and is the same
pattern the loadout's `Actions` array already uses.

Evaluation semantics carried over exactly: a leaf is satisfied when its action's uses this
turn ≥ `RequiredUsesThisTurn`; a group combines its *enabled* children by `Any`/`All`; a
disabled node, an unconfigured leaf, or a group with nothing enabled beneath it is
*skipped* rather than failed. A root that resolves to skipped is not satisfied.

## What Would Change It

If the config needs to be edited by non-editor tooling, or replicated, an object tree is
heavier than structs; a fixed-depth struct form would then be worth revisiting. Not a
current need — the loadout is a static data asset read the same way on every machine.
