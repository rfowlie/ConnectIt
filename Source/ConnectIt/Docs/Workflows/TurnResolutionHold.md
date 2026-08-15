# Pattern: Resolution Hold Counter

## Problem

A turn-based (or phase-based) game often needs to delay advancing to the next turn/phase until some asynchronous, variable-duration work finishes — visual effects, animations, a summary screen. The naive fix is a fixed-duration timer ("wait 2 seconds, that's probably long enough") which is really a guess dressed up as a signal: too short and the next turn starts while something is still visibly resolving; too long and every turn wastes idle time waiting out a timer nothing is actually using anymore.

## Pattern

Give the turn/phase system a **generic hold counter** — `BeginHold()`/`EndHold()` — with *zero knowledge* of what's holding it or why. Anything that needs to delay advancement increments the counter when it starts caring and decrements it when it's done; advancement is gated purely on the count being back to zero. The turn system stays completely decoupled from whatever visual/logic systems are the actual reason for the delay — it doesn't import their types, doesn't know their names, and doesn't need updating when a new reason to hold is added later.

```
Generic turn/phase system                  Project-specific orchestrator
──────────────────────────                 ───────────────────────────
CurrentPhase = Resolution
  → broadcast "resolution started"    →    sees the broadcast, decides what
                                            (if anything) needs to finish first
  (blocked while count > 0)                → BeginHold()
                                            → ...waits for its own conditions...
                                            → EndHold()
  count == 0 → advance to next phase  ←
```

## Participants / Classes Involved

| Role | ConnectIt class | Note |
|---|---|---|
| Generic hold counter, owned by the turn/phase state machine | `UTurnBasedParticipantManagerComponent::BeginResolutionHold()` / `EndResolutionHold()` (`UnrealTurnBasedMechanics`) | A plain integer count, no knowledge of tags, board state, or ConnectIt at all. `AdvanceToNextParticipant` only runs once the count is back to zero. See [UnrealTurnBasedMechanics docs](../../../Plugins/UnrealTurnBasedMechanics/Docs/README.md). |
| The signal that resolution has started | `OnTurnResolutionStarted` (broadcast from `EndTurn()`) | Fired *after* the counter is reset to `0` for the new turn, so a bug that leaves a previous turn's hold unreleased can never permanently stall every future turn. |
| The project-specific orchestrator | `AConnectIt_BoardManager::HandleTurnResolutionStarted` | Bound to the broadcast above. Combines two independently-scoped facts — "is the board visually quiet" and "should a turn-end-specific reaction run" — to decide when to release the hold. See [ConnectItModule.md](../ConnectItModule.md). |
| "Is the board visually quiet" fact | `UConnectIt_BoardSequencerComponent::IsIdle()` / `OnSequenceIdle` | Knows nothing about turns — a pure board-domain fact, reused from [GameplayTag-EventSequencing.md](GameplayTag-EventSequencing.md). |
| The turn-end-specific gated reaction | A one-step `QueueTagSequence({ ConnectIt_Event_TurnEnd })` | Reuses the exact same gated-sequencing primitive as board-change visuals, letting any external system register a gated turn-end reaction (summary screen, camera reset) using an identical pattern. |

## Sequence

1. A turn ends (timeout, explicit end-turn submission, etc.) → the turn/phase state machine resets its hold counter to zero and broadcasts "resolution started."
2. The project-specific orchestrator, bound to that broadcast, immediately calls `BeginHold()` — this is what actually stops the state machine's own post-broadcast advance check from firing right away.
3. It checks whether the thing it actually cares about (in ConnectIt: board-change visuals) is already settled. If so, proceed immediately to step 4. If not, it binds that system's own "now idle" signal and waits.
4. Once settled, the orchestrator runs its own gated reaction for "the turn is ending" — reusing whatever generic sequencing primitive the project already has (in ConnectIt: one more `QueueTagSequence` call, see [GameplayTag-EventSequencing.md](GameplayTag-EventSequencing.md)) — deliberately kept as a *separate* concern from the "is the board quiet" check in step 3, rather than folded into it.
5. Once that reaction completes, the orchestrator calls `EndHold()`. If nothing else is holding, the count returns to zero and the turn/phase state machine advances immediately.

## Why It's Reusable

The counter itself — `BeginHold()`/`EndHold()`, gating an "advance" call on count-zero — is a trivial, fully engine-agnostic primitive, and that's exactly the point: its value is in **where it's allowed to know things and where it isn't**. Any turn-based/phase-based system (not just Unreal, not just games) that needs "wait for variable-duration work before advancing" benefits from this shape:

- **The generic system exposes only the counter, never a callback into project-specific types.** This is what keeps a reusable plugin/library reusable — it would be easy (and wrong) to instead have the turn system directly await a `BoardSequencer` or a specific visual system; that couples generic infrastructure to one project's concerns and breaks the moment a second project wants different things to hold on.
- **A counter, not a boolean flag**, so more than one independent system can hold resolution back without needing to coordinate with each other or know about each other. ConnectIt only ever places one hold today, but nothing in the design assumes that stays true.
- **Reset-before-broadcast is a small but important robustness property**: resetting the counter to zero *before* announcing "resolution has started" (rather than trusting every previous holder to have correctly released) bounds the blast radius of a bug to one turn, not a permanently stuck game.
- **Keep the orchestrator's own concerns separate even from each other** — "is the board quiet" and "should a turn-end-specific reaction run" are two different questions answered by two different mechanisms, deliberately not merged into one check, so each stays reusable/testable on its own.

## Gotchas

- Forgetting to call `EndHold()` on every exit path (including error/early-return paths in whatever async work you're gating on) will stall that turn — since the counter resets at the start of the *next* turn's resolution broadcast, a single missed release costs at most one turn, but it's still worth auditing every code path that calls `BeginHold()` for a matching `EndHold()`.
- This pattern assumes the generic system's "advance" step is the *only* place that needs to be held back. If there are multiple advancement points in your state machine, either all of them need to check the same counter, or you need multiple independently-scoped counters — don't assume one counter automatically covers every place your project might want to delay something.
- The orchestrator binding the broadcast must do so early enough (before the turn system could possibly advance on its own) — in ConnectIt this works because `BeginHold()` is called synchronously, inline, during the broadcast itself, not deferred to next tick.

## Source

Portable rewrite of `Source/ConnectIt/Workflows/TurnResolutionHold_Workflow.txt`, which remains the ConnectIt-specific implementation reference (exact function names, exact call order, file paths).
