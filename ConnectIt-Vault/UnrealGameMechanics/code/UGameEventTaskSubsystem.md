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
  a `UFUNCTION() void Handler(FGameplayTag EventTag)`. `BindOnTagComplete` is how a
  caller learns "the whole sequence for *this* tag is done" — reflection-based, one bind
  per tag a listener cares about.
- `OnAnyTagComplete` (`BlueprintAssignable`, added 2026-09-18) — fires for **every** tag's
  completion, not just one. For a listener that reacts the same way (or dispatches
  internally by tag) regardless of which tag fired, this is one `AddDynamic` instead of a
  separate `BindOnTagComplete` + handler per tag. `BindOnTagComplete` stays the right tool
  for a listener that only ever cares about one specific tag (e.g.
  [[AConnectIt_GameMode|AConnectIt_GameMode]]'s `PlayerWin` binding,
  `UTurnBasedParticipantManagerComponent`'s turn-end binding — neither migrated, both
  still tag-scoped on purpose).
- `GetTagsInQueue()`, `OnActiveManagerTagsChanged` (BlueprintAssignable).

## Collaborators

- Owns `TMap<FGameplayTag, UGameEventTaskManager*>`; `GetOrCreateManager` stamps the
  manager's `EventTag`.
- `HandleOnManagerComplete` advances `ContainerQueue` via `TryExecuteNextContainer`, and
  (since 2026-09-18) broadcasts `OnAnyTagComplete` — one line, additive, doesn't touch the
  per-tag `OnManagerComplete` delegate `BindOnTagComplete` binds to.
- Consumers: `UnrealTurnBasedMechanics` (`UTurnBasedParticipantManagerComponent`, tag-
  scoped `BindOnTagComplete`), project board managers, `UConnectIt_PieceRegistry` (via
  `OnAnyTagComplete` — see
  [[ConnectIt/code/UConnectIt_BoardRegistrySubsystem|ConnectIt_BoardRegistrySubsystem]]),
  any visual system.

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

- 2026-09-18 — **`OnAnyTagComplete` added** (`FOnAnyTagComplete`, `BlueprintAssignable`) —
  a generic "any tag just completed" signal, broadcast from inside the existing
  `HandleOnManagerComplete`, so a listener that reacts the same way to any event
  finishing doesn't need a separate `UFUNCTION` + `BindOnTagComplete` call per tag. First
  (and so far only) consumer: `UConnectIt_PieceRegistry`, see
  [[ConnectIt/_decisions/2026-09-18-generic-game-event-any-tag-complete-delegate|ConnectIt/_decisions/2026-09-18-generic-game-event-any-tag-complete-delegate]].
  Reflects a local, uncommitted change at write time — `commit:` above still anchors to
  the last real commit; re-anchor once this lands in a commit.
- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[UnrealGameMechanics/CLAUDE|UnrealGameMechanics overview]] → *GameEvent*.
- [[UnrealGameMechanics/code/systems/gated-event-tag-queue|systems/gated-event-tag-queue]]
- [[UnrealGameMechanics/code/recipes/gate-a-response-on-a-tag|recipes/gate-a-response-on-a-tag]]
- Decision: [[ConnectIt/_decisions/2026-09-18-generic-game-event-any-tag-complete-delegate|ConnectIt/_decisions/2026-09-18-generic-game-event-any-tag-complete-delegate]]
