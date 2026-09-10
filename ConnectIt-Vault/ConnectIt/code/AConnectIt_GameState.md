---
schema: code
kind: UCLASS
role: primary
source:
  - Source/ConnectIt/Public/Framework/GameState/ConnectIt_GameState.h
  - Source/ConnectIt/Private/Framework/GameState/ConnectIt_GameState.cpp
reconciled: 2026-09-07
commit: 668872e
---

# AConnectIt_GameState

`ATurnBasedGameState` subclass. Replicated, one instance per world, on server **and every
client** — so it hosts the two things that must be a single source of truth: the
[[UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]] (board) and a
replicated `FConnectItMatchResult` (results screen). Also the convenience read-API layer
for UI.

## When you touch this

- Reading board scores / occupancy / target-score / turn info from UI or gameplay.
- The end-of-match results payload.

## Entry points

- **Board component:** `GetBoardStateComponent()`.
- **Board reads (from the locally replicated snapshot):** `GetFactionScore`,
  `GetAllScores`, `IsTileOccupied`, `IsTileValidForPlacement`, `GetBoardSnapshot`
  (previous + current for animation), `GetTargetScore`, `GetFactionScoreProgress`
  (0..1, returns 0 when target is 0 rather than dividing).
- **Turn reads:** `GetActiveParticipantName`, `IsLocalPlayerTurn`.
- **Match result:** `MatchResult` (`ReplicatedUsing = OnRep_MatchResult`),
  `OnMatchResultUpdated`, server-only `SetMatchResult(WinningFactionSlot, EMatchEndReason,
  TotalTurns)`.
- **Debug seed:** `GetInfo()` → `FConnectItGameStateInfo`.
- Types: `FConnectItMatchResult` (`WinningFactionSlot`, `FinalScores`, `TotalTurnsPlayed`,
  `EndReason`, `bMatchOver`), `EMatchEndReason`.

## Collaborators

- Owns `BoardStateComponent` (`VisibleDefaultsOnly`).
- `AConnectIt_GameMode::HandleGameOver` / `HandleInvalidNumberOfPlayers` call
  `SetMatchResult`.
- Board reads delegate into `BoardStateComponent::GetCurrentState()` / the board-state
  library.

## Gotchas

- Board state lives **here**, not on any board actor — see
  `old/Source/ConnectIt/Docs/Workflows/SingleSourceOfTruth-Replication.md`. Same reasoning as
  `UTurnBasedParticipantManagerComponent` living on the GameState.
- Per-tile / per-state helpers (`IsTileOccupied`, `GetScore`, …) are also on
  `UConnectIt_BoardStateLibrary` (takes `FConnectItBoardState` by const&) so they work on
  *previous* / hypothetical (MinMax) states too — prefer the library when you're not
  asking about the live component.
- `SetMatchResult` is server-only.

## Cross-impact

Changing `FConnectItMatchResult` touches the results UI and `AConnectIt_GameMode`'s
game-over handlers. Board read wrappers mirror `UConnectIt_BoardStateComponent` /
`UConnectIt_BoardStateLibrary` — keep them in sync.

## See also

- In-repo: `old/Source/ConnectIt/Docs/Workflows/SingleSourceOfTruth-Replication.md`; `old/Source/ConnectIt/Docs/RuntimeStateAccess.md`.
- [[board-state-single-source-of-truth|systems/board-state-single-source-of-truth]]
