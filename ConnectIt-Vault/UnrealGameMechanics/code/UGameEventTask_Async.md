---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/GameEvent/GameEventTask_Async.h
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Private/GameEvent/GameEventTask_Async.cpp
reconciled: 2026-09-06
commit: 99cdd6a
---

# UGameEventTask_Async

`UObject`. One gated unit of work inside a
[[UnrealGameMechanics/code/UGameEventTaskManager|UGameEventTaskManager]] phase. You give
it an `OnExecuteDelegate` (what to do) and it calls back `OnComplete` when the work is
finished; the manager won't advance its phase until every task in that phase has.

## When you touch this

- Wrapping a piece of visual/async work (an animation, a timeline, a delayed effect) so a
  tag sequence waits for it.

## Entry points

- `OnExecuteDelegate` (`FGameEventTaskExecute`, `BlueprintReadWrite`) — bind before
  registering; the manager invokes it when the phase runs.
- `OnComplete` (`FGameEventTask_Async`, multicast, carries `this`) — broadcast when done.
- `CallOnComplete()` (BlueprintCallable) — convenience: broadcasts `OnComplete` passing
  self (nicer in Blueprint).
- `bIsPersistentTask` (`BlueprintReadWrite`) — if true the owning manager keeps it across
  runs instead of clearing it.

## Collaborators

- Registered via
  [[UnrealGameMechanics/code/UGameEventTaskSubsystem|UGameEventTaskSubsystem]]`::RegisterAsyncTask`
  or `UGameEventTaskManager::RegisterAsyncTask`.
- `UGameTurnHandler::RegisterAsyncTaskTurnStart` / `…TurnEnd` funnel tasks into its
  turn managers.

## Gotchas

- **Registration is refused if `OnExecuteDelegate` is unbound** — bind first, register
  second.
- You must eventually call `CallOnComplete()` (or broadcast `OnComplete`) or the phase —
  and the whole tag sequence, and anything gating on it (e.g. a turn advance) — stalls
  forever. Complete on failure too.
- `bIsPersistentTask = true` means it fires on *every* sequence for that tag; only set it
  for genuinely permanent reactions.

## Cross-impact

Change the completion signature and also update `UGameEventTaskManager`
(`CheckPhaseComplete` bind), `UGameEventTaskSubsystem`, `UGameTurnHandler`.

## See also

- In-repo: `Docs/README.md` → *GameEvent*.
- [[UnrealGameMechanics/systems/gated-event-tag-queue|systems/gated-event-tag-queue]] ·
  [[UnrealGameMechanics/recipes/gate-a-response-on-a-tag|recipes/gate-a-response-on-a-tag]]
