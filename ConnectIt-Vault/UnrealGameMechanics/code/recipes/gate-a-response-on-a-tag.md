---
schema: recipes
task: Make a gameplay event (e.g. a turn advance) wait until a visual/async reaction has finished, by gating it on a gameplay tag.
touches:
  - a new UGameEventTask_Async instance (owned by your reacting system)
  - your reacting system's setup code (RegisterAsyncTask)
  - the firing system's code (QueueTagContainer) — often already exists
  - a project gameplay tag for the event
reconciled: 2026-09-10
commit: ec992a2
---

# Recipe: gate a response on a tag

## Goal

A system that reacts to a gameplay event (playing an animation, a board shift, a score
popup) registers itself so that whoever fires the event's tag does not proceed until the
reaction reports done.

## Prerequisites

- A gameplay tag naming the event (the firing side already calls, or will call,
  `UGameEventTaskSubsystem::QueueTagContainer` with it — e.g.
  `UnrealTurnBasedMechanics`' `TurnEndEventTag`).
- Access to the world's
  [[UnrealGameMechanics/code/UGameEventTaskSubsystem|UGameEventTaskSubsystem]]
  (`GetWorld()->GetSubsystem<UGameEventTaskSubsystem>()`).

## Steps

1. **Create the task.** In your reacting system's init (BeginPlay / component setup):
   `UGameEventTask_Async* Task = NewObject<UGameEventTask_Async>(this);`
2. **Bind the work.** Bind `Task->OnExecuteDelegate` to a function that starts your
   reaction. It must be bound **before** step 3.
3. **Register.** `Subsystem->RegisterAsyncTask(EventTag, Task, Phase)`. Use `Phase` to
   order against other reactions (all phase 0 finish before phase 1 starts).
4. **Signal completion.** When your reaction finishes — including on failure/abort — call
   `Task->CallOnComplete()`. For a one-shot reaction, leave `bIsPersistentTask = false`;
   for a permanent reaction that should run every time the tag fires, set it true.
5. **(firing side, if not already done)** `Subsystem->QueueTagContainer(
   FGameplayTagContainer(EventTag));` and, if you need to know when the whole sequence is
   done, `Subsystem->BindOnTagComplete(EventTag, this, FName("OnEventResolved"))`.

## Verify

- Fire the tag with your reaction registered: the reaction's execute function runs, and
  whatever binds `BindOnTagComplete` (or the turn advance in
  [[UnrealTurnBasedMechanics/code/systems/turn-end-tag-gate|turn-end-tag-gate]]) does **not**
  run until `CallOnComplete()`.
- Temporarily *not* calling `CallOnComplete()` should visibly stall the downstream event —
  confirming the gate is real.
- `UGameEventTaskSubsystem::GetTagsInQueue()` / `UDWidget_GameEventTaskSubsystem` show the
  tag active while your reaction runs.

## Pitfalls

- **Registering before binding `OnExecuteDelegate`** → registration silently refused.
- **Registering while the tag's sequence is already running** → refused; do it at setup.
- **Never calling `CallOnComplete()`** → permanent stall, no timeout.
- Setting `bIsPersistentTask` when you meant a one-shot → the reaction re-fires forever.
- Assuming a frame of delay when no tasks are registered — the tag completes synchronously.

## See also

- [[UnrealGameMechanics/code/UGameEventTaskSubsystem|UGameEventTaskSubsystem]] ·
  [[UnrealGameMechanics/code/UGameEventTask_Async|UGameEventTask_Async]] ·
  [[UnrealGameMechanics/code/UGameEventTaskManager|UGameEventTaskManager]]
- [[UnrealGameMechanics/code/systems/gated-event-tag-queue|systems/gated-event-tag-queue]]
