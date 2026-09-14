---
Date: 2026-09-14
status: Active
superseded by:
tags:
  - action-state
  - playerstate
  - actionscomponent
  - architecture
---

## Decision

Action state (type — permanent/temporary, uses-per-turn, etc.) will live on `PlayerState`
as pure replicated data with no behavior. `ActionsComponent` becomes **the builder**: it
listens for the UI's gameplay-tag broadcast (routed through the PC), queries `PlayerState`
for that action's state, configures/instantiates the action instance from it, and runs it.

Critical constraint agreed alongside this: **`ActionsComponent` must never write back to
`PlayerState` directly.** The only legitimate write path stays the existing pipeline —
client sends a request → server verifies → `UConnectIt_BoardRequestMediator` processes it
→ Mediator writes `PlayerState` → replication → UI reads. This is exactly the pattern
already built for SWAP (`ConsumeSwapUse`-style guarded write, Mediator-only caller).
`PlayerState` also needs a real per-action lookup structure (not yet designed) rather than
one field per action.

## Why

Two earlier shapes were considered and rejected in conversation:

- **Builder living on `PlayerState`** (`PlayerState::GetAction(Tag)` delegating to some
  action-builder) — rejected because it risks the exact replication issues `PlayerState`
  exists to avoid: instantiating/configuring stateful action objects from replicated data
  invites two sources of truth (the instance vs. the state that produced it) and would
  make `PlayerState` responsible for behavior, not just data.
- **Two sources of truth split across `ActionsComponent`-managed instances and
  `PlayerState`** — rejected because whichever side isn't the single writer will
  eventually drift from the other (this was the original worry that started the
  conversation).

The converged shape keeps `PlayerState` as pure, replicated, server-authoritative data
(its actual job), and keeps `ActionsComponent` as the only thing that turns that data into
a running action — but only ever *reads* `PlayerState`, never writes it, so there is
exactly one write path in the whole system and it's the one already proven out by SWAP.
This was reached specifically because the owner was about to start a second action (Board
Shift) and wanted the shape settled before duplicating whatever pattern SWAP happened to
land on ad hoc — see [2026-09-13 — Action state architecture vs. demo progress](../../Development/optimal-co-developer/_questions/2026-09-13-action-state-architecture-vs-demo-progress.md),
which recommended waiting for this second data point before generalizing; Board Shift is
that second data point.

## What Would Change It

- Not yet implemented — SWAP's `ConsumeSwapUse` pattern is the only concrete precedent;
  the per-action lookup structure on `PlayerState` (replacing one field per action) still
  needs designing.
- If a future action's state genuinely can't be expressed as pure data (needs its own
  behavior beyond configuration), this shape would need revisiting — no such case has
  come up yet.
- Building the Board Shift action against this shape is the next real test of it.
