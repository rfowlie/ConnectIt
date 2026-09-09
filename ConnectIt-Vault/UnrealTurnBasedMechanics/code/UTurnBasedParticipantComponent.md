---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Turn/Participant/TurnBasedParticipantComponent.h
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Private/Turn/Participant/TurnBasedParticipantComponent.cpp
reconciled: 2026-09-06
commit: e28339c
---

# UTurnBasedParticipantComponent

`UActorComponent` on each controller — the per-participant client/server bridge to the
match. Small: a client-safe `IsMyTurn()`, two server RPCs, two client RPCs, two local
delegates.

## When you touch this

- Sending "I'm ready" / "I end my turn" from a controller.
- Reacting locally to this participant's own turn starting/ending or an opponent's turn
  starting.

## Entry points

- **Server RPCs (`Server, Reliable`):** `ServerNotifyReady()`, `ServerSubmitTurnEnd()`.
- **Client RPCs (called by the manager):** `ClientReceiveTurnNotification(FTurnNotification)`,
  `ClientReceiveOpponentTurnStarted(int32)`.
- **Queries:** `IsMyTurn()` (client-only), `GetActiveParticipantSlotIndex()`,
  `GetParticipantType()`, `GetParticipantManager()` (resolves the manager on the
  GameState — works on server and clients).
- **Fires:** `OnTurnNotificationReceived(+_Native)` (`const FTurnNotification&`),
  `OnOpponentTurnStarted(+_Native)` (`int32` slot).

## Collaborators

- **`UTurnBasedParticipantManagerComponent`** (`friend`) — sets `CachedSlotIndex` /
  `ParticipantType`, drives the client RPCs.
- **`UTurnBasedControllerCoordinatorComponent`** binds `OnTurnNotificationReceived` /
  `OnOpponentTurnStarted` and turns them into `UTurnBasedActionsComponent` `Notify*`
  calls.

## Gotchas

- **`IsMyTurn()` / `bIsMyTurn` is client-side only** — set solely by the client RPCs,
  always `false` on the server. Server code checks
  `UTurnBasedParticipantManagerComponent::IsActiveParticipant()` instead.
- `CachedSlotIndex` / `ParticipantType` are `Replicated`; `bIsMyTurn` is not.
- `ServerSubmitTurnEnd` is a *request* — the manager still validates it's actually this
  participant's turn.
- Delegates carry `FTurnNotification` **by value** in the dynamic version (a
  `const&` dynamic multicast doesn't compile — the native-only sibling exists for that
  reason; see `TurnBasedMechanicsDelegates.h`).

## Cross-impact

Change the notification payload and also update: `FTurnNotification`,
`UTurnBasedParticipantManagerComponent::BuildNotification` / `NotifyActiveParticipant`,
`UTurnBasedControllerCoordinatorComponent` (consumers), any debug widget reading it.

## See also

- In-repo: `Docs/README.md` → *Turn/Participant*; `Docs/Systems.md` →
  *Turn/Participant — the Match State Machine*.
