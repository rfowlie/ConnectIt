---
schema: code
kind: reference
role: primary
source:
  - Source/ConnectIt/Public/Framework/Library/ConnectIt_GameUtilityLibrary.h
  - Source/ConnectIt/Public/Board/ConnectIt_BoardStateComponent.h
  - Source/ConnectIt/Public/Framework/GameState/ConnectIt_GameState.h
  - Source/ConnectIt/Public/ConnectIt_Structs.h
reconciled: 2026-10-23
commit: 07ce342
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
| What just changed (latest change's event list) | `GetChangeEvent()` → `const FConnectItBoardChangeEvent&` | same | Pure |
| The board event playing right now (in a tag listener) | `UConnectIt_BoardEventLibrary::GetActive...Event` (typed) / `GetActiveBoardEventAs<T>` (C++) | the event queue's payload | Callable |
| Both states + change event | `GetBoardSnapshot()` → `const FConnectItBoardStateSnapshot*` | same | — |
| **The board as the player has seen it** (for visuals: steps along one event at a time) | `GetDisplayBoard()` (+ `OnDisplayBoardChanged`) | same, every machine | Pure |
| Tile lock, power and chain (pass the display board in visuals) | `UConnectIt_BoardStateLibrary::GetPlacementBlocker / GetPowerTurnsLeft(State, Position, CurrentTurn) / GetChainPartner / GetChainedTilesFor`; fields `bIsLocked`, `TilePowerTag`, `PowerReadyTurn`, `TileId`, `ChainPartnerId` on `FConnectItTileData` | static library over a board | Pure |
| The turn a board is at (tile power cooldowns) | `FConnectItBoardState::TurnNumber` | on the struct | ReadOnly |
| Per-tile queries | `FConnectItBoardState::IsTileOccupied / IsTileActive / GetScore` (placeable: `FConnectItBoardOperation_PlacePiece::IsTilePlaceableAt`) | inline on the struct | — |
| A phased level's phase | `AConnectIt_GameState::GetCurrentPhase()` / `FConnectItBoardState::CurrentPhase` | replicated with the board | Pure |
| High-level board queries (all/empty/by-faction tiles, random empty, board full, faction won) | `UConnectIt_GameUtilityLibrary::Get*GridTiles* / IsTileEmpty / GetTileAtPosition / GetRandomEmptyGridTile / IsGameBoardFull / HasFactionWon` | `ConnectIt_GameUtilityLibrary` (static, `WorldContextObject`) | Pure |
| Board-state component lookup | `UConnectIt_GameUtilityLibrary::GetBoardStateComponent` | resolves via `GetGameState<AConnectIt_GameState>()->GetBoardStateComponent()` | Pure |
| Grid ↔ world position, rows/columns, dimensions | `UGridTileRegistryBase::GridPositionToWorld / WorldToGridPosition / GetRow / GetColumn / GetRowCount / GetColumnCount` | `GameUtilityLibrary::GetTileRegistry()` → [[UConnectIt_BoardRegistrySubsystem\|UConnectIt_BoardRegistrySubsystem]] | Pure — **can return null** if the level has no `TileRegistry` template or `OnWorldBeginPlay` hasn't fired |
| Piece actor at a grid position | `UGridPieceRegistryBase::GetPiece(Position)` | `GameUtilityLibrary::GetPieceRegistry()` → same subsystem | Pure — same null caveat; `InitialiseRegistry()` still a stub |
| The board's fixed setup (piece class, pool size) | `UConnectIt_GameUtilityLibrary::GetPieceActorClass / GetPiecePoolInitialSize` | the map's level config (`AConnectIt_WorldSettings::FindLevelConfig`) | Pure |
| The match's rules (scoring rule, each side's goals, AI faction) | `UConnectIt_GameUtilityLibrary::GetMatchRules` / `AConnectIt_GameState::GetMatchRules()` (+ `OnMatchRulesChanged`) | replicated mirror of the GameMode's `GetRules()` | Pure |
| The local player's loadout | `UConnectIt_GameUtilityLibrary::GetLocalPlayerLoadout` | the player's PlayerState | Pure |
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
| All configured actions | `GetAllRuntimeActions()`, `FindActionByTag()` | same | Pure |
| Is a board-change request in flight | `IsAwaitingRequestConfirmation()` | same | Pure |
| Per-action active state | `IsActive()` | on the `UTurnBasedAction` instance | Pure |
| Per-action uses / cooldown / availability | `CanActivate()`, `GetPermanentRuntimeState` / `GetNumberedRuntimeState`, `GetUsesRemaining` (read from the PlayerState -- the action holds none); `bIsCancellable`, `GetActionTag()` | on the `UTurnBasedAction` instance | Pure |

## Sequencing ("game is busy" / animation gating)

| What | Accessor | Owner | BP |
|---|---|---|---|
| Is the board busy (a change playing out / level changes waiting) | `AConnectIt_GameState::IsBoardChangeResolving()` (+ `OnBoardChangeResolvingChanged`); server: `AConnectIt_GameMode::IsBoardSettled()` | replicated mirror | Pure |
| Tags currently firing right now | `GetTagsInQueue()` → `TArray<FGameplayTag>` | `UGameEventTaskSubsystem` | Callable |
| Fires when that set changes | `OnActiveManagerTagsChanged` (no params — re-call `GetTagsInQueue()`) | same | Assignable |

## Gotchas

- **Registry accessors return null on a map whose World Settings name no board** — see
  [[UConnectIt_BoardRegistrySubsystem|UConnectIt_BoardRegistrySubsystem]].
- **The rules are server-owned**: the GameMode's `GetRules()` is the live set; everything else reads the GameState
  mirror (a client can lag a moment behind a mid-match change).
- `AConnectIt_GameState`'s board-derived UI wrappers (`GetFactionScore`, `GetAllScores`,
  `IsTileOccupied`, `GetActiveParticipantName`, `IsLocalPlayerTurn`) all degrade to a
  safe default now; `IsLocalPlayerTurn()` was previously `false` on every remote client
  (it read server-only data) and `GetFactionScore`/`GetAllScores` used to `check()`-crash
  on a null board-state component.

## Changes

- 2026-10-23 — rows for the display board, the tile lock / power / chain getters and the board's `TurnNumber`
  (was `LastModifiedTurn`); `GetPieceActorClass` returns `AConnectIt_PieceBase` (the owner's rename of
  `AConnectIt_GridPiece`). (`process-code` sweep — commit `07ce342`.)
- 2026-10-22 — rows updated for a month of changes: the board event list and its per-event getters; the rule set
  replaced `UConnectIt_BoardRules` (read through the replicated rules mirror); the level config is reached through the
  map's World Settings (`GetLevelConfig` removed); action state lives on the PlayerState (`CompletionsThisTurn`,
  `bIsRequired`, `GetRequiredActions` gone); new rows for the phase and the board-busy flag.
- 2026-09-18 — `ConnectIt_Structs.h`/`ConnectIt_BoardStateComponent.h` gained Board Shift
  fields — see [[ConnectIt/code/ConnectItStructs|ConnectItStructs]]'s own Changes entry
  for the specifics; nothing in this page's own accessor descriptions changed.
  (`process-code` sweep — commit `9187568`.)
- 2026-09-10 — page created from the in-repo `RuntimeStateAccess.md` (the module's
  by-system accessor reference), before `old/` was deleted.

## See also

- [[UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]],
  [[UConnectIt_BoardRegistrySubsystem|UConnectIt_BoardRegistrySubsystem]]
- Flows: [[board-state-single-source-of-truth|systems/board-state-single-source-of-truth]],
  [[place-piece-request|systems/place-piece-request]]
- [[ConnectIt/code/__INDEX|code/__INDEX.md]]
