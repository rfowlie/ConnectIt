---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/GameEvent/GameEventTaskManager.h
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Private/GameEvent/GameEventTaskManager.cpp
reconciled: 2026-09-06
commit: 99cdd6a
---

# UGameEventTaskManager

`UObject`. The per-tag engine behind
[[UnrealGameMechanics/code/UGameEventTaskSubsystem|UGameEventTaskSubsystem]]: it holds a
set of tasks that must all complete before it fires `OnManagerComplete`, and runs
async tasks in **phase order** (phase 0 fully completes before phase 1 starts, etc.).

## When you touch this

- Almost never directly — you interact via the subsystem. Touch this when changing how
  phases advance or how completion is detected.

## Entry points

- `static Create()`; `EventTag` (set by the subsystem after construction).
- `RegisterTask(UObject*)` / `UnregisterTask(UObject*)` — the simple "these objects must
  all report done" set (`TaskSet`).
- `RegisterAsyncTask(UGameEventTask_Async*, int32 Phase = 0)` — phased async tasks
  (`AsyncTaskMap` + a persistent variant `AsyncTaskMapPersistent`).
- `InitiateAllTasks()` → `InitiateAsyncTasks()` → `ExecuteNextPhase()`.
- `OnManagerBegin` / `OnManagerComplete` (BlueprintAssignable, carry the `FGameplayTag`).
- `bIsActive` (BlueprintReadOnly).

## Collaborators

- Runs [[UnrealGameMechanics/code/UGameEventTask_Async|UGameEventTask_Async]] instances;
  binds each task's completion to `CheckPhaseComplete`.
- Created & owned by the subsystem; its `OnManagerComplete` is what the subsystem's
  `HandleOnManagerComplete` and callers' `BindOnTagComplete` ultimately hang off.

## Gotchas

- Two completion mechanisms coexist: the plain `TaskSet` (objects call
  `UnregisterTask`) and the phased async map. `CheckTasksComplete` covers the set;
  `CheckPhaseComplete` advances phases.
- `AsyncTaskMapPersistent` tasks are *not* cleared after a run (`bIsPersistentTask` on the
  task) — they re-fire every sequence. Easy to leak behaviour if set unintentionally.
- `PhaseIndex` starts at `-1`; `bAsyncTasksInitiated` guards re-entry.
- Empty manager ⇒ `OnManagerComplete` fires essentially synchronously from
  `InitiateAllTasks`.

## Cross-impact

Change phase/completion logic and also update:
[[UnrealGameMechanics/code/UGameEventTaskSubsystem|UGameEventTaskSubsystem]]
(`TriggerTag`, `HandleOnManagerComplete`),
[[UnrealGameMechanics/code/UGameEventTask_Async|UGameEventTask_Async]], `UGameTurnHandler`
(registers turn-start/end async tasks against its own managers).

## See also

- In-repo: `old/Plugins/UnrealGameMechanics/Docs/README.md` → *GameEvent*; `old/Plugins/UnrealGameMechanics/Docs/Systems.md` → gated sequencing.
- [[UnrealGameMechanics/systems/gated-event-tag-queue|systems/gated-event-tag-queue]]
