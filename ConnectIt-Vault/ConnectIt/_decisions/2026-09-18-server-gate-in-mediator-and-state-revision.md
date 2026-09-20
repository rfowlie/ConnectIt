---
Date: 2026-09-18
status: Active
superseded by:
tags:
  - action-state
  - server-authority
  - mediator
  - replication
---
## Decision

Three implementation calls made while wiring the authoritative per-action state in
(Stage 2 of the class-keyed loadout migration). They refine
[2026-09-18 — Single authoritative counter with limbo](2026-09-18-action-state-single-authoritative-counter-with-limbo.md)
rather than change it.

1. **The per-action gate and spend live in `UConnectIt_BoardRequestMediator::ProcessRequest`,
   not in the controller's `ServerRouteBoardChangeRequest`.** `ProcessRequest` wraps a
   renamed `DispatchRequest`: before dispatch it resolves the request's `ActionTag` against
   the requester's loadout, checks the action may produce the request's `RequestType`, and
   checks `CanUseAction`; after a *successful* dispatch it calls `ConsumeActionUse`. Nothing
   is spent on a rejected request.
2. **`ATurnBasedPlayerState::ActionStateRevision`** — a replicated int32 bumped on every
   mutation. The actions component records it when a request is sent and treats "revision >
   recorded" as "the update I'm waiting for has landed". This makes limbo's ordering guard
   exact: a listen-server host sees the update *before* its outcome RPC, a client usually
   after, and both paths exit correctly. It also means limbo doesn't depend on which counter
   an action happened to change.
3. **`UTurnBasedAction::ProducesRequestType(FGameplayTag)`** (BlueprintNativeEvent, default
   `true`). `FTurnActionRequest::ActionTag` is client-supplied, so without this a client could
   name a cheap action while sending an expensive action's request. Place/Swap/BoardShift
   override it with their own request tag. The default-`true` is deliberate for Blueprint-only
   subclasses; it means an unrestricted action is only as safe as its override.

Also: server turn boundaries are hooked in `UTurnBasedParticipantManagerComponent::StartTurn`
(before the turn notification): `TickActionCooldowns()` then `ResetActionTurnCounters()` on
the active participant's PlayerState. Cooldowns tick on the owner's own turns only, matching
`UTurnBasedAction::ShouldTickCooldown`'s default.

Loadouts are "new system" only when `PlayerState::HasActionConfig()` (config arrays non-empty);
legacy `Actions` loadouts take none of these paths, including SWAP's old
`SwapUsesRemaining` budget, until Stage 3 deletes them.

## Why

- **Mediator over controller RPC:** the Mediator already documents itself as the universal
  choke point for board mutation, so a check there also covers any future server-originated
  (e.g. AI) request, which never passes through the player-controller RPC.
- **Revision counter over comparing `UsesThisTurn`:** an update that leaves the compared
  field unchanged (a reset to 0 that was already 0, a zero-cost action) would look like "no
  update yet" and stall until the timeout.
- **`ProducesRequestType`:** a client-supplied tag is only trustworthy if the server can
  confirm it belongs to the request it arrived with.

## What Would Change It

If actions gain server-side identity of their own (e.g. the server instantiates them), the
`ActionTag` on requests and `ProducesRequestType` would collapse into a direct lookup.
