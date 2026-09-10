---
schema: code
kind: UCLASS
role: primary
source:
  - Source/ConnectIt/Public/Framework/Game State Machine/ConnectIt_State_Game.h
  - Source/ConnectIt/Private/Framework/Game State Machine/ConnectIt_State_Game.cpp
  - Source/ConnectIt/Public/Framework/Game State Machine/ConnectIt_State_Base.h
reconciled: 2026-09-07
commit: 7008bcf
---

# UConnectIt_State_Game (game state machine)

The game's turn-flow state machine, built on `UnrealGameMechanics`' single-state pattern
(`UGameMechanicsStateSimple` + `IGameStateHandlerInterface`). `UConnectIt_State_Game` is
the composite that owns and sequences three sub-states.

Covers `UConnectIt_State_Base` (`GameStateTag`, cached `GameViewModel` + `GameFacade`),
`UConnectIt_State_SelectTile`, `UConnectIt_State_PlacePiece`, `UConnectIt_State_UpdateGameBoard`.

## When you touch this

- Changing the per-turn phase sequence, or what happens on tile-select / piece-placed /
  board-updated.
- Adding a new turn phase.

## Entry points

- `static Create(UObject* Outer)` + `Initialize(UObject* Outer)`.
- **Spawn-exposed inputs (`ExposeOnSpawn`):** `BoardManager` *(legacy name — see
  gotchas)*, `AllPlayerData`, `AllGridTiles`, `PlayerPiecePool`.
- Sub-states (`BlueprintReadOnly`): `StatePlayerTurn` (`…_SelectTile`), `StatePlacePiece`,
  `StateUpdateBoard`; class overrides `StateSelectTileClass` etc.; `Construct…` hooks
  (BlueprintNativeEvent).
- **Phase transitions (BlueprintNativeEvent):** `OnTileSelected`, `OnPiecePlaced`,
  `OnBoardUpdated`, `StartNextPlayerTurn`.
- `OnGameStateChanged` (`FConnectItGameStateDelegate`, carries the phase tag);
  `BroadCastGameState`.
- `GameTurnTracker` (`UGameTurnTracker`).

## Collaborators

- Each `UConnectIt_State_Base` caches a
  [[UConnectIt_GameFacade|UConnectIt_GameFacade]] (read) and
  `UConnectIt_GameViewModel` (write).
- Drives / observes the turn via `UnrealTurnBasedMechanics` (the actual replicated match
  state machine is `UTurnBasedParticipantManagerComponent`; this is the game's local
  phase model on top).

## Gotchas

- **`BoardManager` (`AConnectIt_BoardManager*`) is a legacy field name** — that actor is
  retired. Wiring here may still reference it pending cleanup; the authoritative board
  path is now `AConnectIt_GameMode` → `UConnectIt_BoardRequestMediator`.
- This is the game's *phase* model, not the network-authoritative match state — don't
  confuse it with `UTurnBasedParticipantManagerComponent`.
- Sub-states are `NewObject`'d via the `Construct…` BlueprintNativeEvents — override those
  to swap a phase implementation.

## Cross-impact

A new phase: a `UConnectIt_State_*` subclass + a `…Class` property + a `Construct…` hook +
a transition BlueprintNativeEvent on `UConnectIt_State_Game`, and a phase gameplay tag.

## See also

- In-repo: `old/Source/ConnectIt/Docs/README.md` → game state machine; `UnrealGameMechanics` `old/Plugins/UnrealGameMechanics/Docs/Systems.md`
  → single-state pattern.
