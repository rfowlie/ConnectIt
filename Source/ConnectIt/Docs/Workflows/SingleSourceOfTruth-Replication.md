# Pattern: Single-Source-of-Truth Replication

## Problem

Multiplayer state can be replicated piecemeal — a property here, a delegate broadcast there, another property somewhere else — but every extra independently-replicated piece of state is another place client and server can disagree, and every server-only delegate broadcast is a trap: dynamic delegate broadcasts are local function calls, not RPCs, so code that "just fires a delegate" from server-only logic silently never reaches remote clients at all. ConnectIt hit exactly this: board events used to fire as plain delegate broadcasts directly from server-only code in `AConnectIt_BoardManager`, which meant real remote clients never saw them.

## Pattern

Collapse all of a system's networked state into **one replicated property** holding a snapshot large enough to describe "what changed" as well as "what things are now." Both the server (immediately, on mutation) and each client (on receiving the replication update) run through the *exact same* local broadcast in response — so there is only one code path that reacts to a state change, and it behaves identically regardless of which machine it's running on.

```
SERVER                                    CLIENT
──────                                    ──────
Mutate state
  → SetBoardState(New, ChangeEvent)
      → BoardSnapshot = {Prev, New, ChangeEvent}   -- replicates -->
      → OnBoardStateChanged.Broadcast()                             OnRep_BoardSnapshot()
                                                                        → OnBoardStateChanged.Broadcast()

                              Same listeners, same broadcast, same reaction — on both machines.
```

## Participants / Classes Involved

| Role | ConnectIt class | Note |
|---|---|---|
| The one replicated property | `UConnectIt_BoardStateComponent::BoardSnapshot` (`FConnectItBoardStateSnapshot`, `ReplicatedUsing=OnRep_BoardSnapshot`) | See [ConnectItModule.md](../ConnectItModule.md). The *only* `Replicated` property on the board. |
| The snapshot shape | `FConnectItBoardStateSnapshot { PreviousState, CurrentState, ChangeEvent }` | Carrying "before," "after," and "what kind of change" together means any listener can answer "what happened" without diffing two states itself. |
| Server-side commit | `UConnectIt_BoardStateComponent::SetBoardState(NewState, ChangeEvent)` | `check(IsAuthoritative())`-guarded. Captures the previous state, applies the new one, fires `OnBoardStateChanged` immediately — server doesn't wait for its own replication round-trip to react. |
| Client-side convergence | `OnRep_BoardSnapshot` | Fires the *same* `OnBoardStateChanged` broadcast on receipt — the one place server and client code paths reunite. |
| Listeners (both sides, identical code) | `UConnectIt_TileStateInterpreter`, `UConnectIt_ScoreInterpreter` (both bind `OnBoardStateChanged`) | Neither knows or cares whether it's running because of a local `SetBoardState` call or a replication update. `UConnectIt_PieceSpawnInterpreter` no longer belongs on this row -- it's a `UGridPieceSpawnInterpreter` now, registered directly against `UGameEventTaskSubsystem` tags instead (see the next row). |
| Event-tag enqueueing (both sides, same call site) | `UConnectIt_BoardStateComponent::EnqueueBoardEventTags` | Not a separate listener -- called inline, right after `BroadcastChange()`, from both `SetBoardState` and `OnRep_BoardSnapshot` themselves. Reads `ChangeEvent` and calls `UGameEventTaskSubsystem::QueueTagContainer` once per event; see `Workflows/GameEventSubsystem_Workflow.txt`. |

## Sequence

1. Something server-side decides the state should change (a validated action request — see [ServerAuthoritative-ActionRequest.md](ServerAuthoritative-ActionRequest.md) — resolves to a mutation).
2. The server builds the new state as a value (never mutates the live property directly mid-computation) and a change-description alongside it, then calls the one commit function, which assigns both into the replicated snapshot property and broadcasts locally.
3. Unreal's replication system pushes the updated property to every relevant client.
4. Each client's `OnRep_` callback fires on receipt and re-broadcasts the identical signal locally.
5. Every reactive system in the game — visual interpreters, UI, sequencing — binds that one broadcast once, and never needs to know or branch on which side of the network it's running.

## Why It's Reusable

The core idea — one replicated snapshot, one broadcast, both server-commit and client-`OnRep_` converging on it — is engine-agnostic in spirit (any networking layer with a "value changed, notify me" primitive can implement it) though the specific mechanism (`UPROPERTY(ReplicatedUsing=...)`) is Unreal-specific. What generalizes to any project, in any engine:

- **Never give server-only code a "just tell everyone" side channel that isn't actually networked.** If something needs to reach clients, it needs to go through the replication/notification layer, full stop — a delegate broadcast from server-only code is not that, even though it compiles and "works" in a single-player test.
- **Bundle "previous," "current," and "what changed" together** rather than replicating raw current-state and asking listeners to diff it themselves, or replicating a separate "event happened" signal alongside state that arrives out of order relative to it. One atomic snapshot removes an entire class of ordering bugs.
- **Write every reactive listener to be agnostic to *why* it's firing.** A listener that assumes "I'm always running because of a local mutation" breaks the moment it runs on a client; writing it against the shared broadcast from day one avoids a rewrite when multiplayer is added later — which is close to literally what happened in this project (see the branch name `Reintroduce-ConnectIt-Logic-To-Network-Play`).

## Gotchas

- This only replicates **one value's worth** of "current state" at a time — if two mutations happen close together server-side before replication catches up, a client only ever sees the *latest* snapshot, not every intermediate one. ConnectIt's board manager compensates for this at the sequencing layer (see [GameplayTag-EventSequencing.md](GameplayTag-EventSequencing.md)'s `PendingChangeEventQueue`) rather than relying on replication itself to deliver every intermediate change — worth deciding explicitly whether your project needs every intermediate state or only convergence on the final one.
- The snapshot needs to be big enough to describe *what changed*, not just *what things are now* — without the `ChangeEvent` half, a client receiving a new snapshot has no cheap way to know whether a piece was placed, a line scored, or the game ended; it would have to diff two full board states itself. Decide the "what changed" payload shape up front rather than bolting it on later.
- `TMap` doesn't replicate in Unreal — this is why `FConnectItBoardState` stores tile data as parallel `TArray`s (`TilePositions`/`TileDataArray`) rather than a map. Any project applying this pattern in Unreal needs to pick replication-safe container types for the snapshot from the start.

## Source

Synthesized from `UConnectIt_BoardStateComponent`'s implementation (see [ConnectItModule.md](../ConnectItModule.md)) and the "why it exists" rationale documented in `Source/ConnectIt/Workflows/GameEventSubsystem_Workflow.txt`'s "WHY IT EXISTS" section, which records the specific bug (server-only delegates never reaching clients) this pattern replaced.
