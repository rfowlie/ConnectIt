# Pattern: Gated Event Sequencing via Tags

## Problem

Once game-state changes reliably reach every client (see [SingleSourceOfTruth-Replication.md](SingleSourceOfTruth-Replication.md)), a new problem appears: several unrelated systems want to *react visually* to the same change (a shift animation, a piece-drop animation, a score VFX, a win banner), and if they all just fire the instant the state change is observed, they fire **simultaneously** — which looks wrong when the change is actually a sequence (place → maybe score → maybe win). There's also often a second, harder problem hiding in the first: some reactions take real time (an animation, a widget transition) and something downstream (the next event, the next turn) needs to wait for that reaction to *finish*, not just *start*.

## Pattern

Represent each event as a **tag**, not a typed delegate signature, and register reactions against a **generic per-world task-sequencing subsystem** rather than binding directly to whatever object caused the event. The subsystem offers two independent primitives on top of tags: "tell me the instant this tag fires" (no gating) and "run my work and hold the next thing until I say I'm done" (gated). On top of both, the subsystem is itself a genuine **FIFO queue**: a caller enqueues one tag-group at a time, and the queue only starts the next group once every tag in the current one has fully completed — a caller with several events to fire in order just calls the enqueue primitive once per event, back to back, and the queue's own ordering does the rest. No caller-side chaining or second queue is needed on top.

```
Caller (knows WHICH tags fire WHEN)
  → QueueTagContainer({PiecePlaced})
  → QueueTagContainer({LineScored})
  → QueueTagContainer({PlayerWin})
        │
        ▼
Subsystem (generic, tag-keyed, serialized FIFO queue)
  Dequeue {PiecePlaced}: trigger it
             ├─ instant listeners fire immediately (BindOnTagBegin)
             └─ gated listeners run, subsystem waits for all to report done
  Complete → dequeue {LineScored}: trigger it  (same shape)
  Complete → dequeue {PlayerWin}: trigger it    (same shape)
  Complete → queue empty, nothing left pending
```

## Participants / Classes Involved

| Role | Class | Note |
|---|---|---|
| Generic per-world registry + FIFO queue | `UGameEventTaskSubsystem` (`UnrealGameMechanics`, `UWorldSubsystem`) | Has no dependency on anything project-specific — a caller is any consumer, ConnectIt included. See [UnrealGameMechanics docs](../../../Plugins/UnrealGameMechanics/Docs/README.md). `QueueTagContainer(Tags)` is the primary enqueue entry point; the underlying `TriggerTag` is private, reachable only internally. |
| Phase-barrier executor per tag | `UGameEventTaskManager` | Tracks a set of registered tasks; only reports "done" once every one of them has actually finished, not just started. |
| Gated task payload | `UGameEventTask_Async` | `OnExecuteDelegate` (your work) + `OnComplete` (you broadcast when genuinely finished) + `bIsPersistentTask` (auto-re-register for every future trigger, e.g. a permanent VFX manager bound once at startup). |
| Project-specific tag vocabulary | `ConnectIt.Event.Shift/PiecePlaced/LineScored/PlayerWin/TurnEnd` (`Source/ConnectIt/ConnectIt_GameplayTags.h`) | The tags themselves are ConnectIt-specific; the subsystem that keys off them is not. |
| The caller that decides enqueue order | `UConnectIt_BoardStateComponent::EnqueueBoardEventTags` | Reads `FConnectItBoardChangeEvent`'s flags and calls `QueueTagContainer` once per event, in order — see [ConnectItModule.md](../ConnectItModule.md). Called from the same component that owns the data being read, not a separate listener. |
| Data for "what actually happened" | `UConnectIt_BoardStateComponent::GetChangeEvent()` | The tag signal itself carries no parameters — payload data is read separately from the same replicated snapshot driving the queue (see [SingleSourceOfTruth-Replication.md](SingleSourceOfTruth-Replication.md)), available identically on server and client. |
| A downstream consumer of queue ordering, not a caller | `UTurnBasedParticipantManagerComponent` (`UnrealTurnBasedMechanics`) | Turn-end advancement is gated on this same queue via its own `TurnEndEventTag` — see [Turn-End Is a Consumer of This Queue, Not a Special Case](#turn-end-is-a-consumer-of-this-queue-not-a-special-case) below. |

## Sequence

1. State commits (via the single-source-of-truth pattern), and `OnBoardStateChanged` fires on both server and client.
2. The same component that just committed the change (`UConnectIt_BoardStateComponent` in ConnectIt) inspects what changed and calls the enqueue primitive once per event, in order — e.g. a placement always leads with `PiecePlaced`, then conditionally `LineScored` and/or `PlayerWin`; a shift is always its own, disjoint single call.
3. Each `QueueTagContainer` call enqueues its tag-group and tries to start executing immediately. The subsystem's queue is serialized globally — only one group is actually firing at a time across the whole subsystem/world; further calls while one is running are queued and drained in order, never interleaved. This is what lets several back-to-back enqueue calls behave like an ordered chain without the caller needing any chaining logic or second queue of its own.
4. For each tag-group, every tag in the group triggers. Anything registered with `BindOnTagBegin` for that tag fires instantly, before any gated task even executes. Anything registered as a `UGameEventTask_Async` against that tag runs, and the group is not considered complete until every registered task in it has called `OnComplete`.
5. If nothing is registered against a tag yet, its manager completes with zero phases immediately and the queue falls through to the next group synchronously — sequencing is opt-in per listener, not something that has to be wired up before the game functions at all.
6. Once a group completes, the queue immediately tries the next one, if any is waiting.

## Turn-End Is a Consumer of This Queue, Not a Special Case

This is the answer to a question worth stating explicitly, since it's easy to assume a bespoke mechanism exists for it: **how does the turn system know every visual for a board change has finished before letting the next player's turn start?** It doesn't have its own signal for this at all — it's just another caller of the same FIFO queue described above.

`UTurnBasedParticipantManagerComponent` (`UnrealTurnBasedMechanics`) has a project-configurable `TurnEndEventTag` (set to `ConnectIt_Event_TurnEnd` here). In `BeginPlay` it binds its own `AdvanceToNextParticipant` as that tag's `OnManagerComplete` listener via `BindOnTagComplete`, once. When a turn ends, `EndTurn()` enqueues `TurnEndEventTag` through `QueueTagContainer` — the exact same entry point `UConnectIt_BoardStateComponent::EnqueueBoardEventTags` uses for `PiecePlaced`/`LineScored`/`PlayerWin`/`Shift`.

Because every one of these calls lands on the *same* subsystem's *same* global `ContainerQueue`, and a board change's visual tags are always enqueued (from `SetBoardState`) before that turn's `TurnEndEventTag` is enqueued (from the later `EndTurn()` call), the turn-end container physically cannot start executing until every board-visual container ahead of it in the queue has fully completed. No explicit "wait for visuals" signal, hold counter, or polling loop is needed — it falls out of FIFO ordering on a queue both sides already push into for unrelated reasons.

This replaced an earlier, hand-built mechanism: a `BeginResolutionHold()`/`EndResolutionHold()` counter API on the turn manager, driven by a board-side listener that polled a dedicated sequencer component's `IsIdle()` state before releasing the hold. Once `QueueTagContainer` became a genuine FIFO queue and the board-state component started enqueueing its own tags directly, that entire hold/poll mechanism was redundant — it was manually reconstructing a guarantee the shared queue already provides for free — and was removed outright, not just deprecated. If a turn-based project reusing this pattern is tempted to add its own "wait for visuals" hold API on top of a shared task-sequencing queue, this is the sign it's already solved: make sure the "done" event and the "next thing" enqueue both go through the *same* queue instance, in the right order, and no bridging code is needed at all.

See [RuntimeStateAccess.md's Turn-End & Resolution Sequencing section](../RuntimeStateAccess.md#turn-end--resolution-sequencing) for the full current-state reference (including how to inspect an in-flight sequence for debug UI via `GetTagsInQueue()`/`OnActiveManagerTagsChanged`), and [TurnBasedMechanics's own Systems.md](../../../Plugins/UnrealTurnBasedMechanics/Docs/Systems.md#turn-order--the-participant-state-machine) for the plugin-side mechanics of `TurnEndEventTag`.

## Why It's Reusable

Everything except the tag vocabulary itself (`ConnectIt.Event.*`) is generic, and `UGameEventTaskSubsystem` is already written as project-agnostic plugin infrastructure — it has no dependency on ConnectIt at all. The pattern generalizes to any project with the same shape of problem: "several independent systems need to react to the same event, and some of those reactions take real time that later steps must wait for." What ports to any engine, even without an equivalent to Unreal's gameplay tags (any string/enum/symbol keying scheme works):

- **Decouple "which event happened" from "what should happen in response."** A tag (or equivalent symbolic key) lets any number of unrelated systems register reactions without the event source needing to know about them.
- **Separate "instant notification" from "gated notification" as two distinct primitives**, rather than one delegate type that everyone has to coordinate around. Not every listener needs to hold anything up; forcing all of them through the same gate needlessly couples fast and slow reactions together.
- **A chain/sequence primitive belongs in the generic layer, not hand-rolled per call site.** Before this existed, ConnectIt manually chained "bind this manager's `OnManagerComplete` to trigger the next manager" per caller — building the chaining logic once, generically, means every future ordered-event need reuses it instead of re-deriving it.
- **Serialize the queue, don't let sequences interleave.** A second sequence starting while the first is mid-flight is exactly the kind of bug that's hard to reproduce and easy to introduce without a rule like "one at a time, queue the rest."

## Gotchas

- **Read the historical bug this replaced carefully before reusing the "instant notification" side of this pattern**: ConnectIt used to have *two* parallel notification systems for the same four events — typed delegates directly on the board manager, *and* these tags — kept in hand-written lockstep. That produced a real bug where the "tell me now" delegates ended up accidentally gated on step completion because the two systems got tangled together. **Pick one channel per event type and remove the other entirely** rather than running both "temporarily."
- The tag signal itself carries no parameters by design — if you need payload data, it has to come from somewhere else that's already reliably available on both server and client at the moment the tag fires (in ConnectIt's case, the same replicated snapshot driving the whole flow). Don't be tempted to smuggle data through side-channel member variables set "just before" triggering the tag — that reintroduces exactly the ordering fragility this pattern exists to avoid.
- A tag with nothing registered against it completes **synchronously and immediately** — don't assume every step in a chain necessarily yields a frame or tick; write chain-completion handlers to be safe whether the whole chain resolves synchronously or takes several frames.
- `bIsPersistentTask` is convenient for "permanent" listeners (a VFX manager bound once at startup) but means that listener re-registers for *every* future trigger of that tag for the lifetime of the object — make sure a persistent task's `OnExecuteDelegate` is safe to run an unbounded number of times, and that it correctly re-arms (calls `OnComplete` again) on each trigger.

## Source

Portable rewrite of `Source/ConnectIt/Workflows/GameEventSubsystem_Workflow.txt`, which remains the ConnectIt-specific implementation reference (exact tag list, exact call sites, file paths) — see that file's "TURN SYSTEM WAITS ON THIS -- FOR FREE, VIA QUEUE ORDERING" section specifically for the turn-end integration described above, including a traced client/server RPC timing argument for why the ordering guarantee holds.
