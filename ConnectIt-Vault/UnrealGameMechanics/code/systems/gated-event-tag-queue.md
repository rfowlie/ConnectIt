---
schema: systems
spans:
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/GameEvent/GameEventTaskSubsystem.h
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Private/GameEvent/GameEventTaskSubsystem.cpp
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/GameEvent/GameEventTaskManager.h
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Private/GameEvent/GameEventTaskManager.cpp
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/GameEvent/GameEventTask_Async.h
reconciled: 2026-10-22
commit: faab026
---

# Gated event tag queue

## What happens

Any system fires a gameplay tag by calling
[[UnrealGameMechanics/code/UGameEventTaskSubsystem|UGameEventTaskSubsystem]]`::QueueTagContainer`
(or `QueueTagContainerWithPayload`, which attaches a struct of data to the entry).
The subsystem keeps a FIFO of entries (tags + optional payload); it runs one entry at a time,
and a listener reads the running entry's data with `GetActivePayload`. For the
active container, every tag runs **in parallel**, each driven by its own lazily-created
[[UnrealGameMechanics/code/UGameEventTaskManager|UGameEventTaskManager]]. A manager runs
its [[UnrealGameMechanics/code/UGameEventTask_Async|UGameEventTask_Async]] tasks in
**phase order** — phase 0 must fully complete before phase 1 begins — then fires
`OnManagerComplete`. Every tag completing also broadcasts `OnAnyTagComplete`. Only when
*every* tag in the active container has completed does the subsystem clear the payload and
dequeue the next entry. This is a phase-barrier: whoever bound
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
    Caller->>S: QueueTagContainer({TagA, TagB}) / ...WithPayload(Tags, Payload)
    S->>S: enqueue entry; if idle, TryExecuteNextContainer() -- sets ActivePayload first
    S->>M: TriggerTag(TagA)  %% + TagB in parallel
    M->>M: InitiateAllTasks() → ExecuteNextPhase()
    loop each phase in order
        M->>T: invoke OnExecuteDelegate
        T-->>M: CallOnComplete() → CheckPhaseComplete
    end
    M-->>S: OnManagerComplete(TagA)
    S->>S: HandleOnManagerComplete — all tags in container done?
    S-->>L: (tag's) OnTagComplete fires; OnAnyTagComplete(TagA)
    S->>S: TryExecuteNextContainer()  %% dequeue next
```

## Steps

1. **Setup:** systems call `UGameEventTaskSubsystem::RegisterAsyncTask(Tag, Task, Phase)`
   with each `UGameEventTask_Async`'s `OnExecuteDelegate` already bound.
2. **Fire:** a caller calls `QueueTagContainer(Tags)` or `QueueTagContainerWithPayload(Tags,
   Payload)`. Enqueued; if nothing is running, execution starts immediately -- the payload is
   in place (`GetActivePayload`) before the first tag triggers.
3. **Per tag:** `TriggerTag` (private) drives that tag's `UGameEventTaskManager` —
   `InitiateAllTasks` → `ExecuteNextPhase`.
4. **Per phase:** every task's `OnExecuteDelegate` is invoked; the manager waits for each
   task's `OnComplete` (`CallOnComplete`) via `CheckPhaseComplete`, then advances.
5. **Manager done:** `OnManagerComplete(Tag)` fires → subsystem's
   `HandleOnManagerComplete`.
6. **Container done:** each tag's completion fires its `BindOnTagComplete` listeners and
   `OnAnyTagComplete`; once all tags in the active entry have completed, the payload is cleared
   and `TryExecuteNextContainer` dequeues the next.

## Gotchas

- **A task that never calls `CallOnComplete()` stalls everything** downstream — no
  timeout.
- `RegisterAsyncTask` is refused while a tag's sequence is running, or if the task's
  `OnExecuteDelegate` is unbound — register during setup.
- No registered tasks ⇒ the tag completes effectively synchronously; callers must not
  assume a frame gap. (ConnectIt's board-change lock relies on this being handled: its
  completion marker can complete inside the call that queued it.)
- The payload lives only while its entry is active -- copy what a delayed reaction needs.
- The deprecated `QueueTagSequence` path keeps parallel bookkeeping on the same managers —
  don't mix it with `QueueTagContainer`.
- `bIsPersistentTask` tasks re-run on every firing of their tag.

## Cross-impact

Changing queue/phase semantics touches all three GameEvent types plus every consumer:
`UnrealTurnBasedMechanics` (turn-end gate — see
[[UnrealTurnBasedMechanics/code/systems/turn-end-tag-gate|turn-end-tag-gate]]),
`UGameTurnHandler`, project board managers, `UDWidget_GameEventTaskSubsystem`.

## See also

- [[UnrealGameMechanics/code/recipes/gate-a-response-on-a-tag|recipes/gate-a-response-on-a-tag]]
