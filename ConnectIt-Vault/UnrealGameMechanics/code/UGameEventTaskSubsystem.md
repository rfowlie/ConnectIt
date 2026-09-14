---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/GameEvent/GameEventTaskSubsystem.h
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Private/GameEvent/GameEventTaskSubsystem.cpp
reconciled: 2026-09-10
commit: ec992a2
---

# UGameEventTaskSubsystem

`UWorldSubsystem`. A per-world registry of tag-keyed
[[UnrealGameMechanics/code/UGameEventTaskManager|UGameEventTaskManager]] instances. Any
system can register a gated/ordered response against a gameplay tag and fire that tag's
sequence **without ever holding a manager pointer** — all access is through this
subsystem. Managers are created lazily per tag and live for the world's lifetime.

## When you touch this

- Firing a phase-barrier: "do X, and don't let anything downstream proceed until every
  registered reaction to X has finished."
- Wiring a visual/UI system to a gameplay event it doesn't own (it binds against a tag,
  not an actor).
- This is the mechanism `UnrealTurnBasedMechanics`' turn-end gate runs on — see
  [[UnrealTurnBasedMechanics/code/systems/turn-end-tag-gate|that flow]].

## Entry points

- `QueueTagContainer(const FGameplayTagContainer&)` — the **only** public way to fire.
  Enqueues the container; every tag in it runs in parallel; the queue does not advance to
  the next container until every tag's sequence fully completes.
- `RegisterAsyncTask(FGameplayTag, UGameEventTask_Async*, int32 Phase = 0)` — add a gated
  task at a phase. Refused if that tag's sequence is currently running or the task's
  `OnExecuteDelegate` is unbound.
- `BindOnTagBegin` / `BindOnTagComplete` (+ `Unbind…`) — `(Object, UFUNCTION name)` pairs,
  zero-param UFUNCTION. `BindOnTagComplete` is how a caller learns "the whole sequence
  for this tag is done".
- `GetTagsInQueue()`, `OnActiveManagerTagsChanged` (BlueprintAssignable).

## Collaborators

- Owns `TMap<FGameplayTag, UGameEventTaskManager*>`; `GetOrCreateManager` stamps the
  manager's `EventTag`.
- `HandleOnManagerComplete` advances `ContainerQueue` via `TryExecuteNextContainer`.
- Consumers: `UnrealTurnBasedMechanics` (`UTurnBasedParticipantManagerComponent`),
  project board managers, any visual system.

## Gotchas

- `TriggerTag` is **private** — always go through `QueueTagContainer` so firings are
  properly serialized. A deprecated `QueueTagSequence` path keeps *separate* bookkeeping
  on the same per-tag managers; don't mix the two.
- `RegisterAsyncTask` silently fails (logs) if the sequence is mid-run or the task isn't
  ready — register during setup, not reactively.
- If no tasks are registered for a queued tag, its sequence completes effectively
  immediately — callers must not assume a frame of delay.
- Managers never free until world teardown.

## Cross-impact

Change the queue/phase semantics and also update:
[[UnrealGameMechanics/code/UGameEventTaskManager|UGameEventTaskManager]] (phase execution),
[[UnrealGameMechanics/code/UGameEventTask_Async|UGameEventTask_Async]] (completion
contract), `UDWidget_GameEventTaskSubsystem`, and every consumer that calls
`QueueTagContainer` / `BindOnTagComplete` (notably `UTurnBasedParticipantManagerComponent`).

## Changes

- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[UnrealGameMechanics/CLAUDE|UnrealGameMechanics overview]] → *GameEvent*.
- [[UnrealGameMechanics/code/systems/gated-event-tag-queue|systems/gated-event-tag-queue]]
- [[UnrealGameMechanics/code/recipes/gate-a-response-on-a-tag|recipes/gate-a-response-on-a-tag]]
