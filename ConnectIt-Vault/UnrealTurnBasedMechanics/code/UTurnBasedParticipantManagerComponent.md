---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Turn/Participant/TurnBasedParticipantManagerComponent.h
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Private/Turn/Participant/TurnBasedParticipantManagerComponent.cpp
reconciled: 2026-09-10
commit: 3086271
---

# UTurnBasedParticipantManagerComponent

`UActorComponent` that lives on `ATurnBasedGameState` and **is the match state machine**:
registration, ready-check, turn start/end, timeout, forfeit, reconnect, and tag-gated
turn advancement. Server-authoritative; a handful of fields replicate to clients.

## When you touch this

- Changing match/turn flow, timers, forfeit or reconnect behaviour.
- Wiring a project system into the turn-end gate.
- Choosing / writing a turn-order strategy.

## Entry points

- **Config:** `TurnDuration`, `ForfeitThreshold`, `ReconnectTimeout`,
  `TurnOrderStrategy` (`TScriptInterface<ITurnOrderInterface>`, inline, defaults to
  `USequentialTurnOrderStrategy`), `TurnEndEventTag` (**project must set**).
- **Server-only calls:** `RegisterParticipant`, `BeginReadyCheck`,
  `NotifyParticipantReady`, `NotifyTurnEndSubmitted`, `NotifyParticipantDisconnected` /
  `NotifyParticipantReconnected`.
- **Queries (client-safe where replicated):** `IsActiveParticipant(AController*)`,
  `IsMatchOver()`, `GetActiveParticipant(bool&)`, `GetParticipantBySlot(int32, bool&)`,
  `GetControllerAtIndex` (null on clients), `GetInfo()`.
- **Override (Blueprint):** `CheckAllParticipantsRegistered` (BlueprintImplementableEvent).
- **Fires:** `OnTurnPhaseChanged`, `OnTurnChanged`, `OnActiveControllerChanged`,
  `OnParticipantForfeited`, `OnParticipantIndexChanged`, `OnAllParticipantsReady`,
  `OnGameOver`, `OnInvalidNumberOfPlayers`.
- **Private state-machine core:** `SetPhase`, `StartTurn`, `EndTurn(ETurnEndReason)`,
  `AdvanceToNextParticipant(FGameplayTag)`.

## Collaborators

- **UnrealGameMechanics `UGameEventTaskSubsystem`:** `EndTurn` queues `TurnEndEventTag`
  via `QueueTagContainer`; `AdvanceToNextParticipant` is bound as that tag's
  `BindOnTagComplete` handler in `BeginPlay`. See
  [[UnrealTurnBasedMechanics/code/systems/turn-end-tag-gate|turn-end-tag-gate]].
- **`ITurnOrderInterface`:** delegates next/first index selection.
- **`ATurnBasedGameState` / `ATurnBasedPlayerState`:** the only sanctioned writer of their
  match/participant fields (`friend`); `SetMatchPhase`, participant setters.
- **`UTurnBasedParticipantComponent`** on each controller: `BroadcastTurnStart`,
  `NotifyActiveParticipant`, `BuildNotification` → client RPCs.

## Gotchas

- **Server authority.** All `Notify*` / turn-control methods are server-only. Client code
  must never call them.
- **Never use `UTurnBasedParticipantComponent::IsMyTurn()` on the server** — it's a
  client-only flag. Use `IsActiveParticipant()` here instead.
- **`TurnEndEventTag` unset** → `AdvanceToNextParticipant` runs immediately with an error
  logged; no gating happens.
- `TurnNumber` uses `ReplicatedUsing` (not plain `Replicated`) precisely so its OnRep
  fires every turn advance even when the value would otherwise coalesce.
- `GetParticipantBySlot` searches by the `SlotIndex` **field**, not by array index — they
  coincide today only by registration order.
- Turn countdown is display-only: clients derive remaining time from
  `ReplicatedTurnStartServerTime` + `ReplicatedTurnDuration`; only the server's
  `TurnTimerHandle` actually ends a turn.
- The component **does not declare the match over** on too-few-players — it fires
  `OnInvalidNumberOfPlayers` and lets the project decide.

## Cross-impact

Change the phase set / lifecycle and also update: `ETurnPhase` / `EMatchPhase` /
`ETurnEndReason` enums, `ATurnBasedGameState` (replicated `MatchPhase`, passthroughs),
`UTurnBasedParticipantComponent` (`IsMyTurnStartingPhase` / `…EndingPhase`),
`FTurnNotification` / `FTurnBasedParticipantManagerInfo`,
`UDWidget_TurnBasedParticipantManagerComponent`.

## Changes

- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[UnrealTurnBasedMechanics/CLAUDE|UnrealTurnBasedMechanics overview]] → *Turn/Participant*.
