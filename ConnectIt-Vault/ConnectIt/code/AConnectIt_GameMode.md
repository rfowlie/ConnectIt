---
schema: code
kind: UCLASS
role: primary
source:
  - Source/ConnectIt/Public/Framework/GameMode/ConnectIt_GameMode.h
  - Source/ConnectIt/Private/Framework/GameMode/ConnectIt_GameMode.cpp
reconciled: 2026-09-10
commit: eddd631
---

# AConnectIt_GameMode

`ATurnBasedGameMode` subclass. The **server-only** hub: it owns the two objects that
replaced the retired `AConnectIt_BoardManager` actor —
[[UConnectIt_BoardRequestMediator|UConnectIt_BoardRequestMediator]] and
[[UConnectIt_BoardRules|UConnectIt_BoardRules]] — and is the single entry point
for board-change requests.

## When you touch this

- Match setup: AI spawn (Adventure), board initialisation, ready-check.
- Adding a new board-request path (it forwards to the mediator).
- Game-over / forfeit handling.

## Entry points

- **Config (`EditDefaultsOnly`):** `MatchType` (`EConnectItMatchType::Adventure` |
  `Online`), `AIControllerClass`, `AIDisplayName`, `NumFactions` (drives scoreboard size).
- `ProcessBoardRequest(const FTurnActionRequest&) → bool` — **the** board-request entry
  point; forwards to `BoardRequestMediator->ProcessRequest`. Callers depend on GameMode's
  surface, not the mediator directly.
- **Overrides:** `PostLogin`, `HandleMatchHasStarted` (→ `SpawnAndRegisterAI`,
  `InitialiseBoard`), `HandleMatchHasEnded`.
- **Bound handlers:** `HandleGameOver(FGameplayTag)` (on the `ConnectIt_Event_PlayerWin`
  tag completing — reads winner from `BoardStateComponent` persistent state, not the tag
  param), `HandleInvalidNumberOfPlayers` (on
  `UTurnBasedParticipantManagerComponent::OnInvalidNumberOfPlayers` → resolves
  winner-by-default, `EndMatch()`).

## Collaborators

- Constructs `BoardRequestMediator` + `BoardRules` in **`HandleMatchHasStarted`**, not the
  constructor (NewObject in the ctor runs before Blueprint-child property overrides — a
  CDO/archetype-timing pitfall this project has hit before).
- `InitialiseBoard` calls
  [[UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]]`::InitialiseBoardState`,
  resolving `TileRegistry`/`PieceRegistry` via `UConnectIt_GameUtilityLibrary` →
  `UConnectIt_BoardRegistrySubsystem` (one canonical per-world instance, initialised at
  `OnWorldBeginPlay` — always ready by the time `InitialiseBoard` runs off
  `HandleMatchHasStarted`).
- `AConnectIt_PlayerController::ServerRouteBoardChangeRequest` → `ProcessBoardRequest`.

## Gotchas

- **Structurally server-only.** `GetWorld()->GetAuthGameMode()` is null on every client,
  so this object (and the mediator/rules it owns) simply doesn't exist client-side — the
  old `if (!HasAuthority()) return false;` guards were removed, not replaced.
- Don't `NewObject` the mediator/rules in the constructor (see Collaborators).
- `HandleGameOver` ignores its `FGameplayTag` param for data — that's just the tag's
  identity for disambiguation; real data is on `BoardStateComponent`.

## Cross-impact

A new request type touches: `AConnectIt_GameMode` (only if a new public forwarder is
wanted), `UConnectIt_BoardRequestMediator` (a `HandleXRequest`), a `FConnectItRequest*`
payload struct, and `FConnectItBoardChangeEvent` fields — see
[[add-a-board-request-type|recipes/add-a-board-request-type]].

## Changes

- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored (`eddd631`).
- 2026-09-09 — `InitialiseBoard` stopped doing its own `FConstPlayerControllerIterator`
  scan for the registries; now pulls them from `UConnectIt_GameUtilityLibrary` →
  [[UConnectIt_BoardRegistrySubsystem|UConnectIt_BoardRegistrySubsystem]], and passes a
  real `PieceRegistry` into `InitialiseBoardState` instead of `nullptr`. Removed a
  silent-failure path (the scan could run before any controller connected). See
  [[ConnectIt/decisions/2026-09-09-board-registries-to-world-subsystem|decisions/2026-09-09-board-registries-to-world-subsystem]].
- 2026-09-08 — board authority moved here: `AConnectIt_BoardManager` (world actor)
  retired; `BoardRequestMediator` + `BoardRules` constructed in `HandleMatchHasStarted`
  as server-only UObject members. See
  [[ConnectIt/decisions/2026-09-08-board-architecture-overhaul|decisions/2026-09-08-board-architecture-overhaul]].

## See also

- Decisions: [[ConnectIt/decisions/2026-09-08-board-architecture-overhaul|board-architecture-overhaul]],
  [[ConnectIt/decisions/2026-09-09-board-registries-to-world-subsystem|board-registries-to-world-subsystem]]
- Flow: [[place-piece-request|systems/place-piece-request]] (the ServerAuthoritative
  action-request pattern)
- [[UConnectIt_BoardRegistrySubsystem|UConnectIt_BoardRegistrySubsystem]]
