# Plan: one board change at a time

Discussion: round 12 of [tile-powers-in-the-board-pipeline](../_questions/tile-powers-in-the-board-pipeline.md). Closes the
known issue in [tile-powers-plan](tile-powers-plan.md) ("Not in this plan"). Written 2026-10-21; approved the same day.
**Stage 1 built** (47/47 tests).

## Context

A board change is one snapshot: the board before, the board after, and the ordered events between them. The snapshot is
one replicated property, so a second change made before the first was sent overwrites it: the client never gets the
first change's events and its board jumps. Today that happens when:

- an Immediately level change runs in the same call as the move (`AConnectIt_GameMode::ProcessBoardRequest` →
  `ApplyLevelChanges(LevelChanges.TakeAfterMove())`);
- several level changes queued for one turn start are applied back-to-back (`HandleTurnPhaseChanged` →
  `TakeNextTurn`);
- the AI makes a second move in one turn: `SubmitDecision` calls `BeginDecision` straight after a success.

**Owner's rule (2026-10-21):** a new board change while one is still resolving -- its visuals still playing -- is an
error. The server holds a lock from a change until its visuals are complete.

- The lock is **ConnectIt-side**: a state on the GameMode, replicated on the GameState, not a new plugin turn phase.
- **"Visuals complete" = the server's own event queue** for now. That is the host's playback on a listen server; it is
  near-instant on a dedicated server, and remote clients' playback isn't awaited. Client acknowledgement comes later,
  with online play.

What already exists and stays:

- The client freezes its action stack until the outcome RPC.
- The turn-end gate queues `TurnEndEventTag` behind the board events and advances only when they complete.
- The Mediator's `bDispatching` covers the synchronous call.

## Shape

- **Locked by** every board change that reaches `SetBoardState` (a player or AI request, a level operation). A scoring
  rule change doesn't touch the board, so it doesn't lock.
- **Released when** a marker tag, `ConnectIt.Event.BoardChangeComplete`, completes. The server queues it right after
  the change's events. The queue is first in, first out, so the marker completes only after every event's visuals.
- **While locked:**
  - **Player and AI requests** are refused with a warning, on the existing failure path (the client's action
    re-enables).
  - **Level changes** wait in order. Each one locks again, so they play one after another.
- **On release:** the next waiting level change is applied on the **next tick**. With no visuals registered the marker
  completes synchronously, inside `SetBoardState`, inside the Mediator's dispatch; deferring a tick keeps that from
  nesting a dispatch.
- **Safety timeout:** after `BoardChangeTimeoutSeconds` (on the GameMode, default 15) the lock logs an error listing
  what is queued, and releases. *(As built: this can't unstick the match by itself. A visual task that never completes
  also blocks the event queue behind it, turn end included. The timeout lifts the lock and reports what's stuck.)*
- **Game over:** release works as normal; the Mediator already refuses everything once the game is over.

## Stage 1: the lock (server)

- **Tag:** `ConnectIt_Event_BoardChangeComplete` (`ConnectIt_GameplayTags.h/.cpp`). It is not a board event: no struct,
  never in a change list.
- **`UConnectIt_BoardStateComponent::SetBoardState`** (server only): after `EnqueueBoardEventTags`, queues the marker
  and broadcasts a new native `OnBoardChangeCommitted`. The seed (`InitialiseBoardState`) doesn't lock.
- **`AConnectIt_GameMode`:**
  - `bBoardChangeResolving` replaces `bApplyingBoardChange` (which only covered the synchronous call) and becomes the
    single "busy" answer. `IsBoardChangeResolving()` is BlueprintPure.
  - It is set on `OnBoardChangeCommitted`.
  - It is cleared in a handler bound once (`BindOnTagComplete` on the marker), which also clears the timeout timer and
    schedules `DrainLevelChanges` for the next tick.
  - `ProcessBoardRequest` refuses while resolving: a warning naming the request type and the faction; returns false.
  - It stops calling `TakeAfterMove` itself; the release drains.
- **Level-change queue** (`ConnectIt_LevelChange.h/.cpp`):
  - `Add` treats resolving as busy.
  - The turn-start changes (`TakeNextTurn`) join the waiting list instead of being applied in a loop.
  - `DrainLevelChanges` applies waiting changes in order and stops after the first that changed the board (it locked).
    Scoring rule changes go through without stopping.
  - The 64-in-a-row guard stays, counted across drains within one turn.
- **Turn start:** `HandleTurnPhaseChanged` queues that turn's level changes, drains, then tells the directors (as
  today). The board may still be resolving when they hear it -- see the AI below.
- **AI** (`ConnectIt_AIController`): it decides only from a settled board.
  - `OnMyTurnStarted` and the post-move "decide again" path wait while the board is resolving.
  - They resume on a new GameMode delegate, `OnBoardChangeSettled` (broadcast on release).
  - `IsDecisionStillCurrent` is unchanged: the turn number still guards stale results.
  - Turn end is unaffected: `ServerSubmitTurnEnd` already queues behind the visuals.
- **Loadout changes** keep their own deferral (`bMoveInProgress` stays on `IsDispatching`).
- *As built (2026-10-21):*
  - The AI also notices the board changing **while it thinks**: the GameMode counts committed changes
    (`GetNumBoardChanges`). If the count moved, or the board isn't settled at submit, the AI decides again once it
    settles. This covers a director's Immediately change from `OnTurnStarted`, whatever order the turn-start handlers
    run in.
  - `IsBoardSettled` = nothing resolving and no level change waiting. `OnBoardChangeSettled` fires from the drain.
  - A drain stops after any change that committed, even when its marker completed inside the call (no visuals): the
    next change then waits a tick.
- **Tests** (queue logic, no world):
  - `Add` while resolving waits.
  - A drain stops after a board change and continues past scoring rule changes.
  - Turn-start changes join behind waiting ones, in order.
- Builds; all existing tests pass.

## Stage 2: clients see the lock

- **`AConnectIt_GameState`:** replicated `bBoardChangeResolving` (set by the GameMode), `OnRep` →
  `OnBoardChangeResolvingChanged` (BlueprintAssignable), `IsBoardChangeResolving()`.
- **`AConnectIt_PlayerController::HandleBoardChangeRequested`:** if the GameState says resolving, don't send the RPC;
  report the outcome as failed locally (same path as a refusal), with a log line.
  - The flag can lag, so the server stays authoritative; this only saves a round trip.
- **Owner, editor:** action buttons / input read `Is Board Change Resolving` and its event (greyed out while the board
  resolves).

## Stage 3: vault

- **Decision note:** one board change at a time; a ConnectIt-side lock; server visuals for now; level changes queue,
  requests are refused.
- **Question:** round 12's resolution.
- **Plans:**
  - [tile-powers-plan](tile-powers-plan.md): the known issue points here as fixed.
  - [three-directors](three-directors.md) / `code/recipes/script-a-level.md`: Immediately = after the current change's
    visuals; several changes play one after another.
- **Systems notes:**
  - `code/systems/place-piece-request.md` (lock and release; also stale -- it still names `UConnectIt_BoardRules`);
  - `ai-turn.md` (waits for a settled board);
  - `game-state-machine.md` if it lists the turn flow.
- **Log.**

## Not in this plan

- **Clients' visuals:** waiting for each client to report its playback done (with a timeout) -- with online play,
  still undecided ([lose-conditions-online](../_questions/lose-conditions-online.md)).
- **A dedicated server:** no visuals there, so the lock releases at once; changes made then could still merge on
  clients. Merging such changes into one snapshot is the fallback if a dedicated server is ever planned.
- **A new turn phase:** rejected. The turn stays active; the board is busy.

## Verification

- Each code stage: build (editor closed); `Automation RunTests ConnectIt` all pass.
- **Owner, PIE:**
  - **Swap level:**
    - play a move whose level director reacts Immediately: the move's events play, then the level change's;
    - the log shows two commits, each released.
  - **AI with a loadout that needs two moves:** the second move waits for the first's visuals.
  - **Two level changes queued for one turn start** (test director): they play one after another before the AI moves.
  - **Click during an animation:** refused, and greyed out once Stage 2's UI is wired.
  - **Timeout:** a debug visual task that never completes logs an error after 15 s, listing the stuck queue. The lock
    lifts, but the turn can't end until the task completes.
  - **2-player listen server:** the client sees every change's events in order.
