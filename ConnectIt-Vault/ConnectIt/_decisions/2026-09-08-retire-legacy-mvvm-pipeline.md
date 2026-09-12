---
Date: 2026-09-08
status: Active
superseded by:
tags:
  - legacy
  - cleanup
  - networking
---

## Decision

Treat the entire non-networked **state-machine / facade / view-model** pipeline as
**dead code**: keep it in the source tree for now, but document it as retired and build
nothing new on it. The set:

- `Framework/Game State Machine/` — `UConnectIt_State_Base` + `UConnectIt_State_Game`,
  `…_SelectTile`, `…_PlacePiece`, `…_UpdateGameBoard`.
- `Framework/Data/` — `UConnectIt_GameFacade`, `UConnectIt_GameViewModel`,
  `UConnectIt_PlayerData` (**not** `UConnectIt_LevelConfig*`, which are live).
- `Framework/Interface/` — `IConnectIt_FacadeHandlerInterface`,
  `IConnectIt_GameStateHandlerInterface`.
- `Framework/GameMode/ConnectIt_GameMode_Play` — empty `AGameModeBase` that only declares
  those two interfaces.

Legacy → live mapping:

| Legacy | Replacement |
|---|---|
| `UConnectIt_GameFacade` (reads) | `UConnectIt_GameUtilityLibrary` — its doc comment says *"Replaces UConnectIt_GameFacade for the networked game."* |
| `UConnectIt_GameViewModel::PlacePiece` (mutate) | `UConnectIt_BoardRequestMediator::ProcessRequest` → `HandlePlacePieceRequest`; visual sequencing handled by gated tags, not the view model |
| `UConnectIt_PlayerData::Score` (no notify) | `FConnectItBoardState::ScoreBoard`, replicated, with `ConnectIt.Event.LineScored` as the change signal |
| `UConnectIt_State_Game`'s SelectTile→PlacePiece→UpdateBoard machine | `UnrealTurnBasedMechanics` action stack + the server request flow |
| `IConnectIt_GameStateHandlerInterface` pub/sub | `OnBoardStateChanged` + `ConnectIt.Event.*` tags |

## Why

Exhaustive grep of the live pipeline (`Board/`, `Action/`, `Framework/Controller/`,
`Framework/GameMode/ConnectIt_GameMode`, `Framework/GameState/`, `GameEvent/`, `Library/`)
finds **no** reference to any class in the legacy set — it's referenced only by itself.
The design assumes it's the only copy of the truth running and has no place for a
replication boundary, which is precisely why the networked pipeline was built instead of
extending it. Deleting it now is a larger, riskier change (an entire subfolder tree) than
the current work wants to take on; documenting it as retired stops anyone re-adopting it
by mistake in the meantime.

## What Would Change It

- Confirmation that nothing **outside** the C++ layer references it either — level
  Blueprints, editor utility widgets — after which the pipeline can simply be deleted
  (this is the single largest dead-weight removal candidate in the module).
- If a future need for a local, non-replicated phase model appears, build a fresh one
  against the replicated board state — do not revive this.
