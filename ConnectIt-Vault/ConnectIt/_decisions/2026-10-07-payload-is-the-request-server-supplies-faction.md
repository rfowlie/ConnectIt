---
Date: 2026-10-07
status: Active
superseded by:
tags:
  - architecture
  - networking
  - security
  - requests
---
## Decision

A board-change request says two things only: **which action is asking** and **what is being asked**.

- `FTurnActionRequest` (plugin `UnrealTurnBasedMechanics`) is `{ ActionTag, Payload }`. `RequestType`, `FactionID` and
  the unused `AdditionalData` are removed.
- An action calls `RequestBoardChange(Payload)` with just its payload struct (in ConnectIt, a
  `FConnectItBoardOperation`); the base class wraps it and stamps the action tag.
- **Who is asking is supplied by the server**, from the connection the request arrived on: the sending controller's
  own PlayerState slot (`AConnectIt_PlayerController::ServerRouteBoardChangeRequest`), passed alongside the request
  as `RequestingFaction` to `AConnectIt_GameMode::ProcessBoardRequest` and the Mediator. The AI controller passes its
  own slot the same way.
- What kind of request it is comes from the payload operation (`GetRequestType`), for the loadout gate and for logs.
  The Mediator's "payload type must match the claimed type" check is gone: the type is no longer stated twice.
- `FConnectItAIDecision::RequestType` is removed for the same reason; a decision is its payload.

## Why

The operation already carries everything about a move, so the envelope was repeating it. More importantly, the
envelope's `FactionID` was filled in by the client and only overwritten on the server when unset. The Mediator looked
up the player state by that number, so a modified client could send the opponent's faction on its own turn and act as
them, spending their action uses. Taking the requester from the connection closes that.

`ActionTag` stays in the request because the operation doesn't know which loadout action is being spent, and the
server can't always derive it: two actions may send the same kind of operation.

## What Would Change It

Needing data that belongs to the request rather than to the operation or the action (it would be added to the
envelope then, not before). A cheaper pending-request match than echoing the whole request back to the client.
