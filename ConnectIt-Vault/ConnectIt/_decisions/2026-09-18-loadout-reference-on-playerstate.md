---
Date: 2026-09-18
status: Active
superseded by:
tags:
  - action-state
  - playerstate
  - loadout
---
## Decision

`DefaultLevelConfig` (or the active level config generally) holds the canonical `Loadout`
asset. `PlayerState` holds **one shared reference** to that same `Loadout` — not a
per-player duplicated copy — plus per-action runtime-state structs (uses/cooldowns; see
[2026-09-18 — Generic turn-end requirement system](2026-09-18-generic-turn-end-requirement-system.md)
for their shape). The Actions UI reads available actions from `PlayerState`'s loadout
reference instead of the current hardcoded Place/Swap buttons. `ActionsComponent` remains
the **only** thing that ever instantiates/configures an actual action instance — it
queries `PlayerState` for the reference and the relevant runtime state, then builds.
`PlayerState` never owns a built action instance itself.

## Why

Reuses two patterns already proven rather than inventing a third: the
registries'-canonical-template-in-level-config shape
([2026-09-09 — Board registries to world subsystem](2026-09-09-board-registries-to-world-subsystem.md))
and `PlayerState`-as-the-one-thing-both-UI-and-gameplay-logic-read-from
([2026-09-14 — Action state on PlayerState, ActionsComponent as builder](2026-09-14-action-state-on-playerstate-actionscomponent-as-builder.md)).
Every board rule discussed to date is symmetric — no per-faction loadout variation has
come up as an actual need — so a single shared reference avoids the per-player
`DuplicateObject` machinery the registries need for a different reason (independent
per-machine mutable state, which a static loadout reference doesn't have). Keeping the
instantiation boundary on `ActionsComponent` preserves the 09-14 decision's core
guarantee: `PlayerState` stays pure, replicated data, never behavior.

**Explicitly still open, not settled by this decision:** whether `PlayerState` should
expose a "factory method" (pass a tag, get back a fully parameterized instance) was
raised and parked mid-conversation — floated as possibly blurring the instantiation
boundary this decision just drew. Needs its own pass before being built either way.

## What Would Change It

If asymmetric per-faction loadouts become a real requirement, this needs to move to the
registries' duplicated-per-player pattern instead of a single shared reference.
