---
schema: systems
spans:
  - Source/ConnectIt/Public/Action/ConnectIt_PlacePieceAction.h
  - Source/ConnectIt/Private/Action/ConnectIt_PlacePieceAction.cpp
  - Source/ConnectIt/Public/Framework/Controller/ConnectIt_PlayerController.h
  - Source/ConnectIt/Private/Framework/Controller/AConnectIt_PlayerController.cpp
  - Source/ConnectIt/Public/Framework/GameMode/ConnectIt_GameMode.h
  - Source/ConnectIt/Public/Board/ConnectIt_BoardRequestMediator.h
  - Source/ConnectIt/Public/Board/ConnectIt_BoardStateComponent.h
reconciled: 2026-09-07
commit: 668872e
---

# Place-piece request (server-authoritative)

## What happens

On the local client, `UConnectIt_PlacePieceAction` (a
[[UnrealTurnBasedMechanics/code/UTurnBasedAction|UTurnBasedAction]] subclass) runs the
hover→select pipeline, then on a valid selection builds an `FTurnActionRequest`
(`RequestType` = place-piece, `FactionID` from the participant slot,
`FConnectItRequestPlacePiece` in `Payload`) and fires `OnChangeRequested`.
[[UnrealTurnBasedMechanics/code/UTurnBasedActionsComponent|UTurnBasedActionsComponent]]
freezes the action stack (`bAwaitingRequestConfirmation`) and re-broadcasts. The
ConnectIt player controller routes it to the server via `ServerRouteBoardChangeRequest`,
which calls `AConnectIt_GameMode::ProcessBoardRequest` →
[[game/code/UConnectIt_BoardRequestMediator|UConnectIt_BoardRequestMediator]]`::ProcessRequest`
→ `HandlePlacePieceRequest`. That handler validates the tile, writes the piece into a
working `FConnectItBoardState`, runs
[[game/code/UConnectIt_BoardRules|UConnectIt_BoardRules]] (`ApplyScoring`,
`CheckWinCondition`), builds the `FConnectItBoardChangeEvent`, and commits via
[[game/code/UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]]`::SetBoardState`.
That replicates the snapshot and, on **both** server and client,
`EnqueueBoardEventTags()` fires the gated tag sequence (piece-placed → line-scored →
player-win) on
[[UnrealGameMechanics/code/UGameEventTaskSubsystem|UGameEventTaskSubsystem]]. The server's
`ProcessRequest` bool result is sent back via `ClientNotifyBoardChangeOutcome` →
`UTurnBasedActionsComponent::NotifyBoardChangeOutcome`, which unfreezes the stack.

## Diagram

```mermaid
sequenceDiagram
    actor Player
    participant A as UConnectIt_PlacePieceAction (client)
    participant AC as UTurnBasedActionsComponent (client)
    participant PC as AConnectIt_PlayerController
    participant GM as AConnectIt_GameMode (server)
    participant M as UConnectIt_BoardRequestMediator (server)
    participant R as UConnectIt_BoardRules (server)
    participant BSC as UConnectIt_BoardStateComponent (on GameState)
    participant TS as UGameEventTaskSubsystem (server + client)

    Player->>A: select valid tile
    A->>A: build FTurnActionRequest (RequestType, FactionID, FConnectItRequestPlacePiece)
    A->>AC: RequestBoardChange → OnChangeRequested
    AC->>AC: push AwaitingConfirmationAction (stack frozen)
    AC->>PC: OnBoardChangeRequested
    PC->>GM: ServerRouteBoardChangeRequest (Server RPC)
    GM->>M: ProcessBoardRequest → ProcessRequest(Request)
    M->>M: HandlePlacePieceRequest — validate tile, write piece into working state
    M->>R: ApplyScoring(state, pos, faction, out) ; CheckWinCondition(state)
    R-->>M: points, bGameOver/WinningFactionSlot
    M->>BSC: SetBoardState(NewState, ChangeEvent)
    BSC-->>BSC: snapshot replicates (server fires now; clients via OnRep)
    par server and every client
        BSC->>TS: EnqueueBoardEventTags — QueueTagContainer(piece-placed), (line-scored), (player-win)
    end
    M-->>GM: bSucceeded
    GM-->>PC: (return)
    PC->>AC: ClientNotifyBoardChangeOutcome(Request, bSucceeded) → NotifyBoardChangeOutcome
    AC->>AC: unfreeze — force-complete on success, reactivate on failure
```

## Steps

1. **Selection** — `UConnectIt_PlacePieceAction::HandleValidSelection_Implementation`
   builds `FTurnActionRequest` and calls the protected `RequestBoardChange`.
2. **Freeze** — `UTurnBasedActionsComponent::HandleBoardChangeRequested` pushes the
   awaiting-confirmation action; stack mutation is blocked
   ([[UnrealTurnBasedMechanics/systems/action-stack-lifecycle|action-stack-lifecycle]] §4).
3. **Route to server** — `AConnectIt_PlayerController::HandleBoardChangeRequested` →
   `ServerRouteBoardChangeRequest` (Server RPC).
4. **Dispatch** — `AConnectIt_GameMode::ProcessBoardRequest` →
   `UConnectIt_BoardRequestMediator::ProcessRequest` → `HandlePlacePieceRequest` (unwrap
   `Payload`).
5. **Mutate + rules** — validate `IsTileValidForPlacement`, write the piece into a working
   `FConnectItBoardState`, `UConnectIt_BoardRules::ApplyScoring` then `CheckWinCondition`,
   assemble `FConnectItBoardChangeEvent`.
6. **Commit** — `UConnectIt_BoardStateComponent::SetBoardState(NewState, ChangeEvent)` →
   snapshot replication + `EnqueueBoardEventTags()` on both machines.
7. **Outcome** — the `ProcessRequest` bool goes back via
   `ClientNotifyBoardChangeOutcome` → `NotifyBoardChangeOutcome`; stack unfreezes.

## Gotchas

- **The client never mutates the board** — it only sends the request and waits for the
  outcome RPC. A lost outcome wedges the action stack (no timeout).
- Scoring/win run on a *working copy* before commit — the committed `ChangeEvent` already
  says `bLineScored` / `bGameWon`.
- `EnqueueBoardEventTags` runs on server **and** client — visual sequencing must be
  idempotent / client-safe.
- `ClientNotifyBoardChangeOutcome` is called from **every** exit path of the server RPC,
  not just successful ones.
- `HandleForcePlacePieceRequest` skips `IsTileValidForPlacement`; swap skips scoring.

## Cross-impact

Adding a request type mirrors this whole chain — see
[[game/recipes/add-a-board-request-type|recipes/add-a-board-request-type]]. The gated tag
step is [[UnrealGameMechanics/systems/gated-event-tag-queue|gated-event-tag-queue]].

## See also

- In-repo: `old/Source/ConnectIt/Docs/Workflows/ServerAuthoritative-ActionRequest.md`;
  `old/Source/ConnectIt/Docs/Workflows/GameplayTag-EventSequencing.md`.
