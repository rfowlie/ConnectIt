---
schema: systems
spans:
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/GameEvent/GameEventTaskSubsystem.h
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Private/GameEvent/GameEventTaskSubsystem.cpp
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/GameEvent/GameEventTaskManager.h
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Private/GameEvent/GameEventTaskManager.cpp
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/GameEvent/GameEventTask_Async.h
reconciled: 2026-09-06
commit: ec992a2
---

# Gated event tag queue

## What happens

Any system fires a gameplay tag by calling
[[UnrealGameMechanics/code/UGameEventTaskSubsystem|UGameEventTaskSubsystem]]`::QueueTagContainer`.
The subsystem keeps a FIFO of tag containers; it runs one container at a time. For the
active container, every tag runs **in parallel**, each driven by its own lazily-created
[[UnrealGameMechanics/code/UGameEventTaskManager|UGameEventTaskManager]]. A manager runs
its [[UnrealGameMechanics/code/UGameEventTask_Async|UGameEventTask_Async]] tasks in
**phase order** — phase 0 must fully complete before phase 1 begins — then fires
`OnManagerComplete`. Only when *every* tag in the active container has completed does the
subsystem dequeue the next container. This is a phase-barrier: whoever bound
`BindOnTagComplete` (e.g. a turn-advance) proceeds only after all reactions finish.

## Diagram

```mermaid
sequenceDiagram
    participant Caller as Any system
    participant S as UGameEventTaskSubsystem
    participant M as UGameEventTaskManager (per tag)
    participant T as UGameEventTask_Async (per phase)
    participant L as Listener (BindOnTagComplete)

    Note over Caller,S: setup: Caller/others RegisterAsyncTask(tag, task, phase)
    Caller->>S: QueueTagContainer({TagA, TagB})
    S->>S: enqueue; if idle, TryExecuteNextContainer()
    S->>M: TriggerTag(TagA)  %% + TagB in parallel
    M->>M: InitiateAllTasks() → ExecuteNextPhase()
    loop each phase in order
        M->>T: invoke OnExecuteDelegate
        T-->>M: CallOnComplete() → CheckPhaseComplete
    end
    M-->>S: OnManagerComplete(TagA)
    S->>S: HandleOnManagerComplete — all tags in container done?
    S-->>L: (tag's) OnTagComplete fires
    S->>S: TryExecuteNextContainer()  %% dequeue next
```

## Steps

1. **Setup:** systems call `UGameEventTaskSubsystem::RegisterAsyncTask(Tag, Task, Phase)`
   with each `UGameEventTask_Async`'s `OnExecuteDelegate` already bound.
2. **Fire:** a caller calls `QueueTagContainer(Tags)`. Enqueued; if nothing is running,
   execution starts immediately.
3. **Per tag:** `TriggerTag` (private) drives that tag's `UGameEventTaskManager` —
   `InitiateAllTasks` → `ExecuteNextPhase`.
4. **Per phase:** every task's `OnExecuteDelegate` is invoked; the manager waits for each
   task's `OnComplete` (`CallOnComplete`) via `CheckPhaseComplete`, then advances.
5. **Manager done:** `OnManagerComplete(Tag)` fires → subsystem's
   `HandleOnManagerComplete`.
6. **Container done:** once all tags in the active container have completed, listeners on
   those tags (`BindOnTagComplete`) fire and `TryExecuteNextContainer` dequeues the next.

## Gotchas

- **A task that never calls `CallOnComplete()` stalls everything** downstream — no
  timeout.
- `RegisterAsyncTask` is refused while a tag's sequence is running, or if the task's
  `OnExecuteDelegate` is unbound — register during setup.
- No registered tasks ⇒ the tag completes effectively synchronously; callers must not
  assume a frame gap.
- The deprecated `QueueTagSequence` path keeps parallel bookkeeping on the same managers —
  don't mix it with `QueueTagContainer`.
- `bIsPersistentTask` tasks re-run on every firing of their tag.

## Cross-impact

Changing queue/phase semantics touches all three GameEvent types plus every consumer:
`UnrealTurnBasedMechanics` (turn-end gate — see
[[UnrealTurnBasedMechanics/systems/turn-end-tag-gate|turn-end-tag-gate]]),
`UGameTurnHandler`, project board managers, `UDWidget_GameEventTaskSubsystem`.

## See also

- In-repo: `old/Plugins/UnrealGameMechanics/Docs/Systems.md` → gated sequencing.
- [[UnrealGameMechanics/recipes/gate-a-response-on-a-tag|recipes/gate-a-response-on-a-tag]]
