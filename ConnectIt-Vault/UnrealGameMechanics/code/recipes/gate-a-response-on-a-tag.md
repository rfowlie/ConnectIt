---
schema: recipes
task: Make a gameplay event (e.g. a turn advance) wait until a visual/async reaction has finished, by gating it on a gameplay tag.
touches:
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/GameEvent/GameEventTask_Async.h (a new UGameEventTask_Async instance, owned by your reacting system)
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/GameEvent/GameEventTaskSubsystem.h (your reacting system's setup code: RegisterAsyncTask; the firing system's QueueTagContainer -- often already exists)
  - a project gameplay tag for the event
reconciled: 2026-10-23
commit: faab026
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
4. **Read the event's data (optional).** If the firing side queued it with a payload
   (`QueueTagContainerWithPayload`), read it in your execute function with
   `Subsystem->GetActivePayload()` -- it is that entry's own data, however many entries are
   waiting.
5. **Signal completion.** When your reaction finishes — including on failure/abort — call
   `Task->CallOnComplete()`. For a one-shot reaction, leave `bIsPersistentTask = false`;
   for a permanent reaction that should run every time the tag fires, set it true.
6. **(firing side, if not already done)** `Subsystem->QueueTagContainer(
   FGameplayTagContainer(EventTag));` (or `QueueTagContainerWithPayload(Tags, Payload)` to
   send data with it) and, if you need to know when the whole sequence is done,
   `Subsystem->BindOnTagComplete(EventTag, this, FName("OnEventResolved"))` -- or
   `OnAnyTagComplete` to hear every tag finish with one bind.

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
- **Calling `CallOnComplete()` from inside your execute function** (a reaction with nothing
  to wait for): the queue calls each phase's tasks from a loop over the very array that
  completing removes from. Defer the completion to the next tick (ConnectIt's
  `AConnectIt_BoardVisualizer` does).
- **The payload is only readable while its entry is active** -- copy what you need in the
  execute function, not when the reaction finishes.

## See also

- [[UnrealGameMechanics/code/UGameEventTaskSubsystem|UGameEventTaskSubsystem]] ·
  [[UnrealGameMechanics/code/UGameEventTask_Async|UGameEventTask_Async]] ·
  [[UnrealGameMechanics/code/UGameEventTaskManager|UGameEventTaskManager]]
- [[UnrealGameMechanics/code/systems/gated-event-tag-queue|systems/gated-event-tag-queue]]
