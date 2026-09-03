# Legacy Pipeline: Game State Machine, Facade, ViewModel & PlayerData

This document is a historical account of ConnectIt's original, non-networked game-flow architecture — a state machine + facade + view-model pipeline that predates the replicated `AConnectIt_BoardManager` pipeline described everywhere else in this documentation set. It is **not currently used anywhere in the live game** (confirmed by grep — see [Confirmation It's Unreferenced](#confirmation-its-unreferenced)), but is written up in full here rather than just noted as dead code, both to preserve the design reasoning for anyone who encounters it in the source tree and because it's a useful before/after contrast for understanding *why* the current networked pipeline is shaped the way it is. For the duplication/removal recommendation, see [Duplication.md #1](Duplication.md#1-legacy-game-flow-pipeline-vs-live-networked-pipeline).

## What It Was For

Before ConnectIt needed to support real network play, the game's flow was organized as a classic **MVVM-flavored state machine**: a top-level state object owned child states for each phase of a turn, a read-only "facade" mediated all board/player queries, and a separate "view-model" mediated all mutations — keeping read and write access to game data behind two distinct, purpose-scoped objects rather than one big manager class.

## How It Was Structured

### The state machine (`Framework/Game State Machine/`)

- **`UConnectIt_State_Base`** (`Public/Framework/Game State Machine/ConnectIt_State_Base.h`) — abstract base for every state, built on the *plugin's* `UGameMechanicsStateSimple` (from `UnrealGameMechanics`) plus `IGameStateHandlerInterface`. Holds a `FGameplayTag GameStateTag` identifying the state, and cached `UConnectIt_GameViewModel*`/`UConnectIt_GameFacade*` references so every state has read/write access to game data without needing to be handed it again.
- **`UConnectIt_State_Game`** (`ConnectIt_State_Game.h`) — the top-level/owning state. Holds `AConnectIt_BoardManager* BoardManager`, `TArray<UConnectIt_PlayerData*> AllPlayerData`, `TArray<AGridTileBase*> AllGridTiles`, a `UActorPool* PlayerPiecePool`, and a `UGameTurnTracker* GameTurnTracker` — i.e. it was the single object holding references to essentially everything the old game loop needed. It constructs and owns three child states (`StatePlayerTurn`, `StatePlacePiece`, `StateUpdateBoard`) via `TSubclassOf` properties so subclasses/Blueprints could swap in custom state implementations. `OnTileSelected`/`OnPiecePlaced`/`OnBoardUpdated`/`StartNextPlayerTurn` are all `BlueprintNativeEvent`s meant to drive the phase transitions.
- **`UConnectIt_State_SelectTile`** — tracks a `SelectedTile`, exposes `GetSelectedTile`/`CheckSelectedTile` (both `BlueprintNativeEvent`), and binds a `FGridTileSelectorDelegate` (from `UnrealGridMechanics`'s `IGridTileSelector`) — i.e. this is the state that listened for the player picking a tile.
- **`UConnectIt_State_PlacePiece`** — trivial: just carries the `SelectedTile` forward via `SetSelectedTile`.
- **`UConnectIt_State_UpdateGameBoard`** — trivial: a `Create()` factory only, no additional members. Presumably meant to be where board-mutation-triggered side effects (scoring, win checks) would run.

### The facade and view-model (`Framework/Data/`)

- **`UConnectIt_GameFacade`** (`ConnectIt_GameFacade.h`) — its own doc comment states its entire purpose: *"access information about the game without being able to change anything."* A large read-only query surface: grid-tile queries (`GetGridTiles`, `GetEmptyGridTiles`, `IsTileEmpty`, `GetGridTilesWithPlayerPieces`, `GetPositionFromTile`, `GetTileFromPosition`, `GetRandomGridTile`), board queries (`IsGameBoardFull`), player queries (`GetAllPlayerData`, `GetPlayerDataById`, `GetCurrentPlayerData`, `GetPlayerScores`, `CheckPlayerWon`), and scoring queries (`GetConnectionsFromMove`, `GetConnectionScore`). Also exposed two Blueprint-assignable delegates, `GameStateDelegate` and `ScoreDelegate`.
- **`UConnectIt_GameViewModel`** (`ConnectIt_GameViewModel.h`) — the mutation counterpart, deliberately small: `PlacePiece(PlayerData, GridTile)` and `SetPlayerScore(PlayerId, Score)`. Constructed with both a `UConnectIt_State_Game*` and the `UConnectIt_GameFacade*` it was paired with, so a mutation could be validated/informed by facade reads.
- **`UConnectIt_PlayerData`** (`ConnectIt_PlayerData.h`) — a plain `UObject` (not an `APlayerState`) representing one player: `PlayerObject`, `GameTurnParticipantComponent`/`GameTurnParticipant` (both from `UnrealGameMechanics`), `PlayerId`, `Score`, `Name`, `LastTileSelected`, `LastPiecePlaced`. `AddToScore(InScore)` was the only mutator, and had no change notification of its own.

### Supporting interfaces (`Framework/Interface/`)

- **`IConnectIt_FacadeHandlerInterface`** — one function, `GetGameFacade()` (`BlueprintNativeEvent`), so any actor could expose "here's how to get the facade" without depending on a concrete class.
- **`IConnectIt_GameStateHandlerInterface`** — `RegisterGameStateCallback`/`UnregisterGameStateCallback`, taking an `FOnConnectItStateDelegate` (tag-parameterized) — a pub/sub mechanism for state-change notifications.
- **`AConnectIt_GameMode_Play`** (`Framework/GameMode/ConnectIt_GameMode_Play.h`) — the only class that actually implements both interfaces above. A thin `AGameModeBase` subclass with an empty body beyond the interface declarations — i.e. scaffolding for a game mode that was meant to wire the facade/state-handler interfaces up, but never grew beyond the declaration.

## Why It Was Superseded

This design worked as a **single-machine, non-replicated** game loop: one `UConnectIt_State_Game` owning direct references to everything, mutated in-place via the view-model, with no notion of server/client separation. When ConnectIt needed real network play (the work this documentation set's host branch, `Reintroduce-ConnectIt-Logic-To-Network-Play`, is named for), this architecture had no natural place for a replication boundary — there's no single authoritative property a client could receive an update through; the state machine assumes it's the only copy of the truth running.

The replacement pipeline (`AConnectIt_BoardManager` + `UConnectIt_BoardStateComponent` + `UConnectIt_GameUtilityLibrary` + `AConnectIt_GameState`, described throughout the rest of this documentation set — see [ConnectItModule.md](ConnectItModule.md) and [SingleSourceOfTruth-Replication.md](Workflows/SingleSourceOfTruth-Replication.md)) solves the same underlying problems — "what's on the board," "whose turn is it," "what's each player's score" — but is built around one authoritative replicated snapshot that server and client converge on, with `UGameEventTaskSubsystem` gameplay-tag sequencing standing in for the facade's delegates. Specifically:

| Legacy responsibility | Replacement |
|---|---|
| `UConnectIt_GameFacade` (read-only board/player queries) | `UConnectIt_GameUtilityLibrary` — its own doc comment explicitly states it *"Replaces UConnectIt_GameFacade for the networked game."* |
| `UConnectIt_GameViewModel::PlacePiece` (mutate + "will need to work with animations etc.") | `AConnectIt_BoardManager::ProcessRequest` → `HandlePlacePieceRequest`, with the animation/visual-sequencing problem solved separately by gated tag sequencing (see [GameplayTag-EventSequencing.md](Workflows/GameplayTag-EventSequencing.md)) rather than being the view-model's job. |
| `UConnectIt_PlayerData::Score` (no change notification) | `FConnectItBoardState::ScoreBoard`, replicated as part of the board snapshot, with the `ConnectIt.Event.LineScored` tag (carrying `PointsScored` on `FConnectItBoardChangeEvent`) as the change notification the old class never had. |
| `UConnectIt_State_Game`'s owned state machine (SelectTile → PlacePiece → UpdateBoard) | The turn/action pipeline described in [ServerAuthoritative-ActionRequest.md](Workflows/ServerAuthoritative-ActionRequest.md), driven by `UnrealTurnBasedMechanics`'s action stack instead of a bespoke state machine. |
| `IConnectIt_GameStateHandlerInterface` pub/sub | `OnBoardStateChanged` (broadcast from both `SetBoardState` on the server and `OnRep_BoardSnapshot` on clients) plus the `ConnectIt.Event.*` gameplay tags. |

## Known Issues in the Legacy Code (for the record only — not being fixed)

These were captured in `Source/ConnectIt/TODO` before this write-up and are preserved here since they're part of the legacy pipeline's own documented history, not because they're expected to be fixed:

- **`ConnectIt_GameFacade.h:67`** — `GetAllPlayerData()` returns live `UConnectIt_PlayerData*` pointers, letting callers mutate through a class whose own doc comment promises read-only access. A TODO notes the old Blueprint version cloned the data before returning it specifically to prevent this — a real gap between the documented contract and actual behavior.
- **`ConnectIt_GameFacade.h:39`** — `GetGridTiles()` returns `TArray<AGridTileBase*>` by value; open question of whether it should return a const array instead.
- **`ConnectIt_GameFacade.h:49`** — `GetGridTilesWithPlayerPieces(int32 PlayerID)` — TODO calls the logic "messy," suggesting it should take player data rather than a raw ID.
- **`ConnectIt_State_Game.cpp:70`** — `BroadCastGameState` has an unresolved debugging comment ("why is this not working? Because not Tag set?") plus a commented-out early `return;` right below it, suggesting the function was short-circuited mid-debugging and only partially reverted. Whether `GetGameStateTag()` can return an unset tag, and what that does to bound listeners, was never confirmed.
- **`ConnectIt_GameViewModel.h:25`** — `PlacePiece`'s TODO ("will need to work with animations etc.") is the clearest evidence that the gated-visual-sequencing problem the current pipeline solves (see [GameplayTag-EventSequencing.md](Workflows/GameplayTag-EventSequencing.md)) was already recognized as unsolved here.
- **`ConnectIt_PlayerData.cpp:10`** — `AddToScore` has no change callback/delegate, the gap the replicated `ConnectIt.Event.LineScored` tag now fills.

## Confirmation It's Unreferenced

Every class in `Framework/Game State Machine/`, `Framework/Data/` (except `UConnectIt_ConfigComponent`, which is live and documented in [ConnectItModule.md](ConnectItModule.md) — it isn't part of this legacy set), and `Framework/Interface/` was checked against the entire live pipeline (`Board/`, `Action/`, `Framework/Controller/`, `Framework/GameMode/ConnectIt_GameMode.h/.cpp` — note not `ConnectIt_GameMode_Play`, `Framework/GameState/`, `GameEvent/`, `Library/`). No live class constructs, includes, or references any of `UConnectIt_GameFacade`, `UConnectIt_GameViewModel`, `UConnectIt_PlayerData`, `UConnectIt_State_Base` or its four subclasses, `IConnectIt_FacadeHandlerInterface`, `IConnectIt_GameStateHandlerInterface`, or `AConnectIt_GameMode_Play`. `AConnectIt_GameMode` (live) and `AConnectIt_GameMode_Play` (legacy) are two entirely separate game mode classes — the project's actual `DefaultGameMode` should be checked in `DefaultEngine.ini`/world settings if there's ever doubt about which one a given map uses, but no C++ class treats them as related.

## Recommendation

See [Duplication.md #1](Duplication.md#1-legacy-game-flow-pipeline-vs-live-networked-pipeline): delete this pipeline once it's confirmed nothing outside the C++ layer (level Blueprints, editor utility widgets) still references it either.
