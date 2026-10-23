---
schema: systems
spans:
  - Source/ConnectIt/Public/Action/ConnectIt_PlacePieceAction.h
  - Source/ConnectIt/Private/Action/ConnectIt_PlacePieceAction.cpp
  - Source/ConnectIt/Public/Framework/Controller/ConnectIt_PlayerController.h
  - Source/ConnectIt/Private/Framework/Controller/AConnectIt_PlayerController.cpp
  - Source/ConnectIt/Public/Framework/GameMode/ConnectIt_GameMode.h
  - Source/ConnectIt/Public/Framework/GameState/ConnectIt_GameState.h
  - Source/ConnectIt/Public/Board/ConnectIt_BoardRequestMediator.h
  - Source/ConnectIt/Public/Board/Rules/ConnectIt_RuleSet.h
  - Source/ConnectIt/Public/Board/ConnectIt_BoardStateComponent.h
reconciled: 2026-10-22
commit: 1e0d5cf
---

# Board-change request (server-authoritative)

Written for a placement; every player action that changes the board (place, swap, shift...) takes the same path, and
the AI joins it at `ProcessBoardRequest` ([[ai-turn]]).

## What happens

On the local client the action (e.g. `UConnectIt_PlacePieceAction`, a
[[UnrealTurnBasedMechanics/code/UTurnBasedAction|UTurnBasedAction]]) builds an `FTurnActionRequest` -- `ActionTag` plus
the board operation itself as `Payload` -- and requests it.
[[UnrealTurnBasedMechanics/code/UTurnBasedActionsComponent|UTurnBasedActionsComponent]] freezes the action stack
(`bAwaitingRequestConfirmation`) and re-broadcasts. The ConnectIt player controller doesn't send it while the GameState
says the board is busy (`IsBoardChangeResolving`); otherwise it goes to the server (`ServerRouteBoardChangeRequest`),
which checks it is this player's turn and supplies the requester's faction from the connection.

`AConnectIt_GameMode::ProcessBoardRequest` refuses it unless the board is **settled** -- **one board change at a time**:
nothing resolving, no level change waiting. [[UConnectIt_BoardRequestMediator]]`::ProcessRequest` gates it against the
loadout, and `DispatchRequest` applies it:

- **Gates:** the action is in the loadout, produces this operation, and is usable now; a shift's steps match the
  action's.
- **Apply:**
  - game over?
  - stamp the faction;
  - `CanApply`;
  - `FConnectItRuleSet::CarryOutOperation` (the operation's `Apply`, then `ResolveBoardChange` scoring);
  - `StampWinState`;
  - `GameWon` on the transition;
  - commit with `SetBoardState`.

The commit replicates the snapshot and queues the change's events on both machines
([[board-state-single-source-of-truth]]). On the server, the GameMode **locks** until the change's visuals have played
out: the `ConnectIt.Event.BoardChangeComplete` marker, queued behind the events, completes. The use is spent, and the
outcome goes back to the client, which unfreezes.

## Diagram

```mermaid
sequenceDiagram
    actor Player
    participant A as Action (client)
    participant AC as UTurnBasedActionsComponent (client)
    participant PC as AConnectIt_PlayerController
    participant GM as AConnectIt_GameMode (server)
    participant M as UConnectIt_BoardRequestMediator (server)
    participant BSC as UConnectIt_BoardStateComponent (GameState)
    participant TS as UGameEventTaskSubsystem

    Player->>A: select a valid target
    A->>AC: RequestBoardChange {ActionTag, Payload = operation}
    AC->>AC: push AwaitingConfirmationAction (stack frozen)
    AC->>PC: OnBoardChangeRequested
    alt GameState says the board is busy
        PC->>AC: outcome false, locally (action re-enables)
    else
        PC->>GM: ServerRouteBoardChangeRequest -> turn check, faction -> ProcessBoardRequest
        GM->>GM: board settled? (else refused)
        GM->>M: ProcessRequest (loadout gate) -> DispatchRequest
        M->>M: CanApply; CarryOutOperation; StampWinState
        M->>BSC: SetBoardState(NewState, ChangeEvent)
        BSC->>BSC: BroadcastChange (directors react -- level changes wait)
        BSC->>TS: one queue entry per event (server + every client)
        BSC->>GM: OnBoardChangeCommitted -> lock, timeout, GameState busy
        BSC->>TS: ConnectIt.Event.BoardChangeComplete (server)
        GM-->>PC: true (use spent)
        PC->>AC: ClientNotifyBoardChangeOutcome -> unfreeze, Complete
        TS-->>GM: marker complete (after the visuals) -> lock lifts
        GM->>GM: next tick: drain waiting level changes, one board change each
        GM->>GM: settled -> GameState not busy, OnBoardChangeSettled
    end
```

## Steps

1. **Request** -- the action hands over the operation as the payload (`RequestBoardChange`).
2. **Freeze** -- the actions component pushes the awaiting-confirmation action; the stack can't change
   ([[UnrealTurnBasedMechanics/code/systems/action-stack-lifecycle|action-stack-lifecycle]] §4).
3. **Busy check (client)** -- `AConnectIt_PlayerController::HandleBoardChangeRequested`: the GameState says the board
   is resolving -> the outcome is answered locally as a refusal. The mirror can lag; the server checks again.
4. **Route** -- `ServerRouteBoardChangeRequest`: active participant, match not over, faction from this controller's
   PlayerState.
5. **One at a time (server)** -- `ProcessBoardRequest`: refused unless `IsBoardSettled()`.
6. **Gate** -- `ProcessRequest`: payload is an operation; the action is in the loadout, produces it, a shift's
   `Steps` matches, and the action can be used now.
7. **Apply and commit** -- `DispatchRequest`, then `SetBoardState`. The directors hear the change through
   `OnBoardStateChanged` and may ask for level changes, which wait. The events are queued, the GameMode locks
   (`OnBoardChangeCommitted`), and the completion marker is queued last.
8. **Outcome** -- the use is spent; `ClientNotifyBoardChangeOutcome` unfreezes the client's stack.
9. **Release** -- when the marker completes (every event's visuals done), the lock lifts. From the next tick, waiting
   level changes are applied, one board change at a time, each with its own lock. When none are left the board is
   settled: the GameState mirror turns off, and `OnBoardChangeSettled` fires (the AI waits for it).

## Gotchas

- **The client never mutates the board** -- it sends the request and waits for the outcome RPC. A lost outcome wedges
  the action stack (no timeout).
- **A second request before the board settles is refused**, on the server and (if the mirror has arrived) on the
  client. UI should grey out actions on the GameState's `IsBoardChangeResolving` / `OnBoardChangeResolvingChanged`.
- **"Visuals done" is the server's own queue** -- the host's playback on a listen server, near-instant on a dedicated
  server; remote clients' playback isn't awaited.
- **A visual task that never completes** blocks the whole event queue (turn end too). After `BoardChangeTimeoutSeconds`
  (15 s) the GameMode logs what is queued and lifts the lock; it can't unstick the queue.
- **With no visual tasks registered, the marker completes inside `SetBoardState`.** That is why the release only
  schedules the drain (next tick) instead of applying anything.
- `ClientNotifyBoardChangeOutcome` is called on **every** exit path of the server RPC.

## Level-only operations

The Level director's changes join this flow at `ProcessLevelOperation` (no loadout gate, no action use), queued while the
board is busy. These operations exist for it alone -- no action produces them (they are in the operation catalog, so a
loadout could grant one, but none does):

- **Change Goals** -- a side's goals replaced (`Set Win / Lose Conditions`): goal progress restarts, `Goals Changed`.
- **Begin Phase** -- `CurrentPhase` on the board, `Phase Changed`; directors then hear On Phase Changed.
- **Remove Tiles / Add Tiles / Change Layout** -- the board's set of tiles (`Set Layout` queues one Change Layout):
  `Tiles Removed`, `Tiles Added`, `Layout Changed`.

Recipe: [[script-a-phased-level]].

## Cross-impact

- Adding an operation: [[add-a-board-request-type]].
- The gated tag queue: [[UnrealGameMechanics/code/systems/gated-event-tag-queue|gated-event-tag-queue]]; the turn-end
  gate queues behind the same events: [[UnrealTurnBasedMechanics/code/systems/turn-end-tag-gate|turn-end-tag-gate]].
- Level changes (Level director): [[script-a-level]].

## See also

- Decision: [one board change at a time](../../_decisions/2026-10-21-one-board-change-at-a-time.md)
- Design: [one-board-change-at-a-time-plan](../../design/one-board-change-at-a-time-plan.md)
