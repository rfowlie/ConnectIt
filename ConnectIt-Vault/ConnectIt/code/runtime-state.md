---
schema: code
kind: reference
role: primary
source:
  - Source/ConnectIt/Public/Framework/Library/ConnectIt_GameUtilityLibrary.h
  - Source/ConnectIt/Public/Board/ConnectIt_BoardStateComponent.h
  - Source/ConnectIt/Public/Framework/GameState/ConnectIt_GameState.h
  - Source/ConnectIt/Public/ConnectIt_Structs.h
reconciled: 2026-09-10
commit: 6477d5d
---

# Runtime state access — which accessor to call

The "I need to read live board / turn / action / sequencing state — where from?" table.
Distilled from the in-repo `RuntimeStateAccess.md`. **BP** = Blueprint access (`Pure` /
`ReadOnly` / `Callable` / `— ` = C++ only). Prefer the `UConnectIt_GameUtilityLibrary`
convenience wrappers over reaching into a manager directly.

## Board state

| What | Accessor | Owner | BP |
|---|---|---|---|
| Full current board (tiles, scores, game-over) | `GetCurrentState()` → `const FConnectItBoardState&` | `UConnectIt_BoardStateComponent` | Pure |
| Board state before the last change | `GetPreviousState()` | same | Pure |
| What just changed | `GetChangeEvent()` → `const FConnectItBoardChangeEvent&` | same | Pure |
| Both states + change event | `GetBoardSnapshot()` → `const FConnectItBoardStateSnapshot*` | same | — |
| Per-tile queries | `FConnectItBoardState::IsTileOccupied / IsTileActive / IsTileValidForPlacement / GetScore` | inline on the struct | — |
| High-level board queries (all/empty/by-faction tiles, random empty, board full, faction won) | `UConnectIt_GameUtilityLibrary::Get*GridTiles* / IsTileEmpty / GetTileAtPosition / GetRandomEmptyGridTile / IsGameBoardFull / HasFactionWon` | `ConnectIt_GameUtilityLibrary` (static, `WorldContextObject`) | Pure |
| Board-state component lookup | `UConnectIt_GameUtilityLibrary::GetBoardStateComponent` | resolves via `GetGameState<AConnectIt_GameState>()->GetBoardStateComponent()` | Pure |
| Grid ↔ world position, rows/columns, dimensions | `UGridTileRegistryBase::GridPositionToWorld / WorldToGridPosition / GetRow / GetColumn / GetRowCount / GetColumnCount` | `GameUtilityLibrary::GetTileRegistry()` → [[UConnectIt_BoardRegistrySubsystem\|UConnectIt_BoardRegistrySubsystem]] | Pure — **can return null** if the level has no `TileRegistry` template or `OnWorldBeginPlay` hasn't fired |
| Piece actor at a grid position | `UGridPieceRegistryBase::GetPiece(Position)` | `GameUtilityLibrary::GetPieceRegistry()` → same subsystem | Pure — same null caveat; `InitialiseRegistry()` still a stub |
| Config (loadouts, piece class, pool size) | `UConnectIt_GameUtilityLibrary::GetLevelConfig()` → `UConnectIt_LevelConfigDataAsset` props | resolved symmetrically server + client from the level→asset map | Pure / ReadOnly |
| Scoring / win-condition strategy in use | `AConnectIt_GameMode::BoardRules` → `UConnectIt_BoardRules` (**server-only**) | `Board/Rules/` | `GetActiveWinConditionName()` / `GetActiveScoringRuleName()` — server-side debug only |
| Win score target (progress bar) | `FConnectItBoardState::TargetScore` / `AConnectIt_GameState::GetTargetScore()`, `GetFactionScoreProgress(int32)` | replicated | Pure |

## Turn / participant / match state

| What | Accessor | Notes | BP |
|---|---|---|---|
| Current turn phase | `ATurnBasedGameState::GetCurrentTurnPhase()` / `UConnectIt_GameUtilityLibrary::GetCurrentTurnPhase` | wrapper preferred | Pure |
| Active participant slot | `ATurnBasedGameState::GetActiveParticipantIndex()` | | Pure |
| Match-wide turn number | `ATurnBasedGameState::GetActiveTurnNumber()` | | Pure |
| Participant list (name, slot, connected, turns taken, type) | `ATurnBasedGameState::GetParticipants()` → `const TArray<FTurnParticipantInfo>&` | | Pure |
| Match phase (WaitingForParticipants / InProgress / Paused / GameOver) | `ATurnBasedGameState::GetMatchPhase()` | | Pure |
| Per-player forfeits / turns-missed / ready | `ATurnBasedPlayerState::GetTurnsMissed / IsForfeited / IsReady / GetSlotIndex / GetParticipantType` | on each `FTurnParticipantInfo::PlayerState` | Pure |
| "Is it my turn" (local player) | `UConnectIt_GameUtilityLibrary::IsLocalPlayerTurn` / `GetLocalPlayerSlotIndex` | correct on every client | Pure |
| Match result (winner, final scores, end reason) | `AConnectIt_GameState::MatchResult` | `ReplicatedUsing=OnRep_MatchResult` | ReadOnly |
| Turn timer — seconds remaining, any participant, any client | `ATurnBasedGameState::GetTurnTimeRemaining(bool&)` / `GetTurnTimeRemainingFraction()` | derived locally from replicated start-time + duration + `GetServerWorldTimeSeconds()`; no per-tick replication | Pure |
| Raw controller at a slot | `UTurnBasedParticipantManagerComponent::GetControllerAtIndex(int32)` | **null on clients** (`ServerControllers` never replicated) — do not use client-side | — |
| Client-side "is it my turn" from the participant component | `UTurnBasedParticipantComponent::IsMyTurn()` | **always `false` on the server** — never gate server logic with it | Pure |

## Action / stack state

| What | Accessor | Owner | BP |
|---|---|---|---|
| What's active (top of stack) | `GetTopAction()`, `GetRootAction()`, `IsRootOnTop()`, `GetStackDepth()` | `UTurnBasedActionsComponent` | Pure |
| All configured / required actions | `GetAllRuntimeActions()`, `GetRequiredActions()`, `FindActionByTag()` | same | Pure |
| Is a board-change request in flight | `IsAwaitingRequestConfirmation()` | same | Pure |
| Per-action active state | `IsActive()` | on the `UTurnBasedAction` instance | Pure |
| Per-action completions / cooldown / config | `CompletionsThisTurn`, `TurnsUntilAvailable`, `bIsRequired`, `bIsCancellable`, `ActionTag` | same | ReadOnly |

## Sequencing ("game is busy" / animation gating)

| What | Accessor | Owner | BP |
|---|---|---|---|
| Tags currently firing right now | `GetTagsInQueue()` → `TArray<FGameplayTag>` | `UGameEventTaskSubsystem` | Callable |
| Fires when that set changes | `OnActiveManagerTagsChanged` (no params — re-call `GetTagsInQueue()`) | same | Assignable |

## Gotchas

- **Registry accessors return null until a level config is authored** — see
  [[UConnectIt_BoardRegistrySubsystem|UConnectIt_BoardRegistrySubsystem]].
- `AConnectIt_GameState`'s board-derived UI wrappers (`GetFactionScore`, `GetAllScores`,
  `IsTileOccupied`, `GetActiveParticipantName`, `IsLocalPlayerTurn`) all degrade to a
  safe default now; `IsLocalPlayerTurn()` was previously `false` on every remote client
  (it read server-only data) and `GetFactionScore`/`GetAllScores` used to `check()`-crash
  on a null board-state component.

## Changes

- 2026-09-10 — page created from the in-repo `RuntimeStateAccess.md` (the module's
  by-system accessor reference), before `old/` was deleted.

## See also

- [[UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]],
  [[UConnectIt_BoardRegistrySubsystem|UConnectIt_BoardRegistrySubsystem]]
- Flows: [[board-state-single-source-of-truth|systems/board-state-single-source-of-truth]],
  [[place-piece-request|systems/place-piece-request]]
- [[ConnectIt/code/index|code/index.md]]
