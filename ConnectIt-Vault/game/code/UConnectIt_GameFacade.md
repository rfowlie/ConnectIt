---
schema: code
kind: UCLASS
role: primary
source:
  - Source/ConnectIt/Public/Framework/Data/ConnectIt_GameFacade.h
  - Source/ConnectIt/Private/Framework/Data/ConnectIt_GameFacade.cpp
  - Source/ConnectIt/Public/Framework/Data/ConnectIt_GameViewModel.h
  - Source/ConnectIt/Private/Framework/Data/ConnectIt_GameViewModel.cpp
reconciled: 2026-09-07
commit: 7008bcf
---

# UConnectIt_GameFacade (+ UConnectIt_GameViewModel)

The read / write split for game logic and UI over
[[game/code/UConnectIt_State_Game|UConnectIt_State_Game]]:

- **`UConnectIt_GameFacade`** — *read-only* window onto the game. "Access information
  about the game without being able to change anything."
- **`UConnectIt_GameViewModel`** — the *write* side: `PlacePiece`, `SetPlayerScore`.

## When you touch this

- UI or a game state needs to query grid / player / scoring data, or request a mutation.

## Entry points

### `UConnectIt_GameFacade` (`Create(Outer, UConnectIt_State_Game*)`)

- Delegates: `GameStateDelegate` (`FGameplayTag`), `ScoreDelegate`.
- **Grid:** `GetGridTiles`, `GetEmptyGridTiles`, `IsTileEmpty`,
  `GetGridTilesWithPlayerPieces(PlayerID)`, `GetPositionFromTile` / `GetTileFromPosition`,
  `GetRandomGridTile`.
- **Board:** `IsGameBoardFull` (BlueprintNativeEvent).
- **Player:** `GetAllPlayerData`, `GetPlayerDataById`, `GetCurrentPlayerData`,
  `GetPlayerScores`, `CheckPlayerWon` (BlueprintNativeEvent).
- **Scoring:** `GetConnectionsFromMove(PlayerID, Move)`, `GetConnectionScore(Connection)`.

### `UConnectIt_GameViewModel` (`Create(Outer, UConnectIt_State_Game*, UConnectIt_GameFacade*)`)

- `PlacePiece(UConnectIt_PlayerData*, AGridTileBase*)`, `SetPlayerScore(PlayerId, Score)`.

## Collaborators

- Both hold a `UConnectIt_State_Game*`; the view model also holds the facade.
- Each `UConnectIt_State_Base` caches both (`GameFacade`, `GameViewModel`).

## Gotchas

- **Facade is read-only by contract** — if you need to change state, that's the view
  model (or, for board changes, the server request path via
  [[game/systems/place-piece-request|place-piece-request]]).
- Several methods carry `TODO`s (messy player-ID handling; wanting to return cloned/const
  data so callers can't tamper) — treat return values as read-only even where the
  signature doesn't enforce it.
- `GetConnectionsFromMove` returns `void` (writes elsewhere) — check the .cpp for where
  the result lands.

## Cross-impact

Facade/view-model methods are the stable surface for UI and the state machine — changing
one ripples into every widget and `UConnectIt_State_*` that uses it.

## See also

- In-repo: `old/Source/ConnectIt/Docs/RuntimeStateAccess.md`; `old/Source/ConnectIt/Docs/UIValueCatalogue.md`;
  `old/Source/ConnectIt/Docs/Workflows/SubsystemDiscovery-DualAccessPattern.md`.
