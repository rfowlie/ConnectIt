---
schema: recipes
task: Add a new server-validated board mutation (a new FTurnActionRequest RequestType) end to end.
touches:
  - Source/ConnectIt/Public/ConnectIt_Structs.h (new FConnectItRequest* payload)
  - Source/ConnectIt/Public/Board/ConnectIt_BoardRequestMediator.h (dispatch + HandleXRequest)
  - Source/ConnectIt/Public/ConnectIt_Structs.h — FConnectItBoardChangeEvent (result fields)
  - Source/ConnectIt/Public/Board/ConnectIt_BoardStateComponent.h — EnqueueBoardEventTags branch
  - Source/ConnectIt/Public/ConnectIt_GameplayTags.h (a request-type tag + a board-event tag)
  - a UTurnBasedAction subclass that builds and sends the request
reconciled: 2026-09-07
commit: 668872e
---

# Recipe: add a board request type

## Goal

A new server-authoritative board mutation (e.g. "rotate a 2×2 block") that a player action
requests, the server validates and applies, and visuals react to — following the same
chain as place-piece.

## Prerequisites

- Read [[game/systems/place-piece-request|place-piece-request]] and
  [[game/systems/board-state-single-source-of-truth|board-state-single-source-of-truth]].
- The existing `FConnectItRequest*` structs + `HandleXRequest` handlers are your
  templates.

## Steps

1. **Payload struct** — in `ConnectIt_Structs.h`, add
   `USTRUCT(BlueprintType) FConnectItRequestMyThing { ... FGridPosition(s) / params ... };`.
   **Do not** add a faction field — `FTurnActionRequest::FactionID` on the envelope
   already carries "who".
2. **Request-type tag** — add a gameplay tag in `ConnectIt_GameplayTags.h` (e.g.
   `ConnectIt.Request.MyThing`).
3. **Dispatch** — in `UConnectIt_BoardRequestMediator::ProcessRequest`, add a branch on
   the new `RequestType` that unwraps `Request.Payload` into `FConnectItRequestMyThing`
   and calls a new private `HandleMyThingRequest(const FConnectItRequestMyThing&, int32
   FactionID) const`.
4. **Handler** — in `HandleMyThingRequest`: get the working state
   (`GetBoardState()->GetCurrentState()` copied), validate, mutate it, run
   `BoardRules->ApplyScoring` / `CheckWinCondition` **only if** exactly one position
   changes ownership (see the swap-vs-capture precedent), assemble a
   `FConnectItBoardChangeEvent`, and commit via
   `GetBoardState()->SetBoardState(NewState, ChangeEvent)`. Return success.
5. **Change-event fields** — in `ConnectIt_Structs.h`, add `bMyThingHappened` + any
   position/faction fields to `FConnectItBoardChangeEvent`; set them in step 4.
6. **Board-event tag + sequencing** — add a `ConnectIt.Event.MyThing` tag; in
   `UConnectIt_BoardStateComponent::EnqueueBoardEventTags`, add a branch that
   `QueueTagContainer`s it when `bMyThingHappened`, in the right order relative to
   piece-placed / line-scored / player-win.
7. **The action** — subclass `UTurnBasedAction` (like `UConnectIt_PlacePieceAction`):
   selection hooks → build `FTurnActionRequest` (new `RequestType`, `FactionID`,
   `FConnectItRequestMyThing` in `Payload`) → `RequestBoardChange`. Register it in a
   `UActionLoadoutDataAsset` — see
   [[UnrealTurnBasedMechanics/recipes/add-a-turn-action|add-a-turn-action]].

## Verify

- PIE: activating the action and selecting produces the mutation on server **and**
  replicates to the client; `FConnectItBoardChangeEvent::bMyThingHappened` is set
  (`UDWidget_ConnectIt_BoardStateComponent`).
- The `ConnectIt.Event.MyThing` tag fires on `UGameEventTaskSubsystem` on both machines
  (bind `BindOnTagBegin` to confirm) and any gated visual reaction runs.
- On an invalid request the handler returns `false` and
  `ClientNotifyBoardChangeOutcome(..., false)` reactivates the action for a retry (stack
  unfreezes).
- Scoring/win behave per your step-4 decision (re-run for single ownership change; skip
  otherwise).

## Pitfalls

- **Mutating `GetCurrentState()` in place** — copy it, mutate the copy, commit via
  `SetBoardState`.
- **Duplicating `FactionID` into the payload** — it lives on the envelope.
- **Forgetting the `EnqueueBoardEventTags` branch** — the state changes but nothing
  visual reacts.
- **Re-running scoring for a multi-position change** — `IConnectIt_ScoringRule::ApplyScoring`
  is single-position; there's no defined call for two (the swap handler deliberately skips
  it).
- Not calling `ClientNotifyBoardChangeOutcome` on the failure path (the mediator returns
  the bool; the player controller RPC must relay it from *every* exit).

## See also

- [[game/code/UConnectIt_BoardRequestMediator|UConnectIt_BoardRequestMediator]] ·
  [[game/code/UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]] ·
  [[game/code/ConnectItStructs|ConnectItStructs]]
- In-repo: `old/Source/ConnectIt/Docs/Workflows/ServerAuthoritative-ActionRequest.md`;
  `old/Source/ConnectIt/Docs/Workflows/GameplayTag-EventSequencing.md`.
