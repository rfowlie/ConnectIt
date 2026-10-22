---
schema: code
kind: UCLASS
role: primary
source:
  - Source/ConnectIt/Public/Framework/GameState/ConnectIt_GameState.h
  - Source/ConnectIt/Private/Framework/GameState/ConnectIt_GameState.cpp
reconciled: 2026-10-22
commit: 1e0d5cf
---

# AConnectIt_GameState

`ATurnBasedGameState` subclass. Replicated, one instance per world, on server **and every
client** — so it hosts what must be a single source of truth: the
[[UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]] (board), a replicated `FConnectItMatchResult` (results
screen), and the read-only **mirrors** of what the server's GameMode owns: the match's rules and whether the board is busy
(one board change at a time). Also the convenience read-API layer for UI.

## When you touch this

- Reading board scores / occupancy / target-score / phase / turn info from UI or gameplay.
- Reading the match's rules (goals, scoring rule) on a client.
- Greying out input while a board change plays out.
- The end-of-match results payload.

## Entry points

- **Board component:** `GetBoardStateComponent()`.
- **Board reads (from the locally replicated snapshot):** `GetFactionScore`,
  `GetAllScores`, `IsTileOccupied`, `IsTileValidForPlacement`, `GetBoardSnapshot`
  (previous + current for animation), `GetTargetScore`, `GetFactionScoreProgress`
  (0..1, returns 0 when target is 0 rather than dividing), `GetCurrentPhase` (a phased level's phase, from the
  committed board -- ahead of a Phase Changed event still playing).
- **Rules mirror:** `GetMatchRules()` (the live `FConnectItRuleSet`, including mid-match changes), `OnMatchRulesChanged`
  (server and clients); server-only `SetMatchRules` (the GameMode's `PublishMatchSetup`).
- **Board-busy mirror:** `IsBoardChangeResolving()` (a change resolving or level changes waiting -- the GameMode's "not
  settled"), `OnBoardChangeResolvingChanged(bResolving)` (server and clients); server-only `SetBoardChangeResolving`.
- **Turn reads:** `GetActiveParticipantName`, `IsLocalPlayerTurn`.
- **Match result:** `MatchResult` (`ReplicatedUsing = OnRep_MatchResult`),
  `OnMatchResultUpdated`, server-only `SetMatchResult(WinningFactionSlot, EMatchEndReason,
  TotalTurns)`.
- **Debug seed:** `GetInfo()` → `FConnectItGameStateInfo`.
- Types: `FConnectItMatchResult` (`WinningFactionSlot`, `FinalScores`, `TotalTurnsPlayed`,
  `EndReason`, `bMatchOver`), `EMatchEndReason` (`ScoreThresholdReached`, `OpponentForfeited`,
  `OpponentDisconnected`, `OpponentLost` -- the other side met a lose condition, `Unknown`).

## Collaborators

- Owns `BoardStateComponent` (`VisibleDefaultsOnly`).
- `AConnectIt_GameMode::HandleGameOver` / `HandleInvalidNumberOfPlayers` call
  `SetMatchResult`; `PublishMatchSetup` calls `SetMatchRules`; the board lock calls `SetBoardChangeResolving`.
- `AConnectIt_PlayerController` doesn't send a request while `IsBoardChangeResolving` (the server re-checks).
- Board reads delegate into `BoardStateComponent::GetCurrentState()` / the board-state
  library.

## Gotchas

- Board state lives **here**, not on any board actor — see
  [[ConnectIt/CLAUDE|ConnectIt overview]] (SingleSourceOfTruth-Replication pattern). Same reasoning as
  `UTurnBasedParticipantManagerComponent` living on the GameState.
- Per-tile / per-state helpers (`IsTileOccupied`, `GetScore`, …) are also on
  `UConnectIt_BoardStateLibrary` (takes `FConnectItBoardState` by const&) so they work on
  *previous* / hypothetical (MinMax) states too — prefer the library when you're not
  asking about the live component.
- `SetMatchResult`, `SetMatchRules` and `SetBoardChangeResolving` are server-only; the GameMode is their only writer.
- The mirrors replicate, so a client can lag a little: the board-busy flag is a courtesy, the server refuses anything
  that arrives early.

## Cross-impact

Changing `FConnectItMatchResult` touches the results UI and `AConnectIt_GameMode`'s
game-over handlers. Board read wrappers mirror `UConnectIt_BoardStateComponent` /
`UConnectIt_BoardStateLibrary` — keep them in sync.

## Changes

- 2026-10-22 — mirrors of the GameMode's live setup: the match rules (`GetMatchRules`, `OnMatchRulesChanged`) and the
  board-busy flag (`IsBoardChangeResolving`, `OnBoardChangeResolvingChanged`); `GetCurrentPhase`;
  `EMatchEndReason::OpponentLost`.
- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[ConnectIt/CLAUDE|ConnectIt overview]].
- [[board-state-single-source-of-truth|systems/board-state-single-source-of-truth]]
