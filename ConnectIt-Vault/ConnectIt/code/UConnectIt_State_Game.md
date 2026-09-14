---
schema: code
kind: UCLASS
role: internal
source:
  - Source/ConnectIt/Public/Framework/Game State Machine/ConnectIt_State_Game.h
  - Source/ConnectIt/Private/Framework/Game State Machine/ConnectIt_State_Game.cpp
  - Source/ConnectIt/Public/Framework/Game State Machine/ConnectIt_State_Base.h
reconciled: 2026-09-10
commit: eddd631
---

# UConnectIt_State_Game (retired legacy)

> **Dead code.** This is ConnectIt's original **non-networked** turn-flow state machine.
> Nothing in the live pipeline (`Board/`, `Action/`, `Framework/Controller/`,
> `Framework/GameMode/ConnectIt_GameMode`, `Framework/GameState/`, `GameEvent/`)
> constructs, includes, or references it — confirmed by exhaustive grep. It is kept in
> the source tree, not built on. Do not extend it or wire new code to it. See
> [[ConnectIt/_decisions/2026-09-08-retire-legacy-mvvm-pipeline|decisions/2026-09-08-retire-legacy-mvvm-pipeline]].

## What it is

A classic **MVVM-flavoured state machine** on `UnrealGameMechanics`'
`UGameMechanicsStateSimple` + `IGameStateHandlerInterface`. `UConnectIt_State_Game` is
the composite that owned three child states — **SelectTile → PlacePiece →
UpdateGameBoard** — and drove one player's turn, then handed off to the next. Covers
`UConnectIt_State_Base` (`GameStateTag`, cached `GameViewModel` + `GameFacade`),
`UConnectIt_State_SelectTile`, `UConnectIt_State_PlacePiece`,
`UConnectIt_State_UpdateGameBoard`.

## When you touch this

- Essentially never. Only if you are **removing** the legacy pipeline, or tracing why a
  class in `Framework/Game State Machine/` exists.

## What replaced it

| Legacy responsibility | Live replacement |
|---|---|
| `UConnectIt_State_Game`'s owned SelectTile → PlacePiece → UpdateBoard machine | `UnrealTurnBasedMechanics` action stack + [[place-piece-request|systems/place-piece-request]] |
| `UConnectIt_GameFacade` read queries | `UConnectIt_GameUtilityLibrary` (its own doc comment: *"Replaces UConnectIt_GameFacade for the networked game"*) |
| `UConnectIt_GameViewModel::PlacePiece` mutation | [[UConnectIt_BoardRequestMediator|UConnectIt_BoardRequestMediator]]`::ProcessRequest` → `HandlePlacePieceRequest` |
| `IConnectIt_GameStateHandlerInterface` pub/sub | `OnBoardStateChanged` + the `ConnectIt.Event.*` gameplay tags |

The load-bearing reason for the swap: the state machine assumes it's the only copy of the
truth running — it has no natural replication boundary. The live pipeline is built around
one authoritative replicated snapshot on
[[UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]] that server and client
converge on.

## Gotchas

- `BoardManager` (`AConnectIt_BoardManager*`) on `UConnectIt_State_Game` is a field
  referencing an **actor that no longer exists** — a second layer of staleness on top of
  the whole class being dead.
- Two of the four sub-states (`_PlacePiece`, `_UpdateGameBoard`) are near-empty even in
  the legacy design — a `Create()` factory and a forwarded `SelectedTile`, nothing more.
- `ConnectIt_State_Game.cpp:70` `BroadCastGameState` carries an unresolved
  mid-debugging comment and a commented-out early `return;` — the legacy code was left
  partially reverted.

## Cross-impact

Removing this pipeline also removes: `UConnectIt_State_*` (5 classes),
[[UConnectIt_GameFacade|UConnectIt_GameFacade]] + `UConnectIt_GameViewModel`,
`UConnectIt_PlayerData`, `AConnectIt_GameMode_Play`, `IConnectIt_FacadeHandlerInterface`,
`IConnectIt_GameStateHandlerInterface`. Check level Blueprints / editor utility widgets
(outside the C++ grep's reach) before deleting.

## Changes

- 2026-09-10 — reframed as **retired legacy / dead code** (was documented as the game's
  "local phase model"). `LegacyPipeline.md` from the in-repo docs proves the whole
  pipeline is unreferenced; captured the legacy→replacement mapping here and the
  rationale in a decision note. Provenance re-anchored (`eddd631`).
- 2026-09-10 — re-ingested to the `_code` schema.

## See also

- Decision: [[ConnectIt/_decisions/2026-09-08-retire-legacy-mvvm-pipeline|decisions/2026-09-08-retire-legacy-mvvm-pipeline]]
- Live path: [[place-piece-request|systems/place-piece-request]],
  [[board-state-single-source-of-truth|systems/board-state-single-source-of-truth]]
- [[UConnectIt_GameFacade|UConnectIt_GameFacade]] (the read/write split, also retired)
