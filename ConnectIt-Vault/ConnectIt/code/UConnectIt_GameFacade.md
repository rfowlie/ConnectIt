---
schema: code
kind: UCLASS
role: internal
source:
  - Source/ConnectIt/Public/Framework/Data/ConnectIt_GameFacade.h
  - Source/ConnectIt/Private/Framework/Data/ConnectIt_GameFacade.cpp
  - Source/ConnectIt/Public/Framework/Data/ConnectIt_GameViewModel.h
  - Source/ConnectIt/Private/Framework/Data/ConnectIt_GameViewModel.cpp
reconciled: 2026-09-10
commit: eddd631
---

# UConnectIt_GameFacade (+ UConnectIt_GameViewModel) — retired legacy

> **Dead code.** The read/write split for the retired, non-networked MVVM pipeline (see
> [[UConnectIt_State_Game|UConnectIt_State_Game]]). Unreferenced by any live class.
> `UConnectIt_GameFacade`'s replacement — `UConnectIt_GameUtilityLibrary` — says so in
> its own doc comment: *"Replaces UConnectIt_GameFacade for the networked game."* Kept in
> the tree, not built on. See
> [[ConnectIt/_decisions/2026-09-08-retire-legacy-mvvm-pipeline|decisions/2026-09-08-retire-legacy-mvvm-pipeline]].

## What it is

- **`UConnectIt_GameFacade`** — a large *read-only* query surface over
  [[UConnectIt_State_Game|UConnectIt_State_Game]]: grid-tile queries (`GetGridTiles`,
  `GetEmptyGridTiles`, `IsTileEmpty`, `GetGridTilesWithPlayerPieces`, tile↔position),
  board (`IsGameBoardFull`), player (`GetAllPlayerData`, `GetCurrentPlayerData`,
  `GetPlayerScores`, `CheckPlayerWon`), scoring (`GetConnectionsFromMove`,
  `GetConnectionScore`). Its doc comment: *"access information about the game without
  being able to change anything."*
- **`UConnectIt_GameViewModel`** — the *write* side, deliberately tiny: `PlacePiece`,
  `SetPlayerScore`.

## When you touch this

- Only when removing the legacy pipeline.

## What replaced it

- Reads → `UConnectIt_GameUtilityLibrary` (networked, Blueprint-safe).
- `PlacePiece` mutation → [[UConnectIt_BoardRequestMediator|UConnectIt_BoardRequestMediator]]`::ProcessRequest`,
  with the animation/visual-sequencing problem (`PlacePiece`'s own `TODO`: *"will need to
  work with animations etc."*) solved separately by gated tag sequencing, not the view
  model's job.
- `UConnectIt_PlayerData::Score` (no change notification) → `FConnectItBoardState::ScoreBoard`,
  replicated, with the `ConnectIt.Event.LineScored` tag as the notification.

## Gotchas

- Documented contract vs behaviour gap even in the legacy code: `GetAllPlayerData()`
  returns live `UConnectIt_PlayerData*` pointers, letting callers mutate through a
  "read-only" class — a `TODO` notes the old Blueprint version cloned first.
- `GetConnectionsFromMove` returns `void` and writes its result elsewhere.

## Cross-impact

Part of the legacy set — see [[UConnectIt_State_Game|UConnectIt_State_Game]] Cross-impact
for the full removal list.

## Changes

- 2026-09-10 — reframed as **retired legacy / dead code** (was documented as a live
  "read / write split for game logic and UI"). Captured the legacy→replacement mapping
  and rationale in a decision note. Provenance re-anchored (`eddd631`).
- 2026-09-10 — re-ingested to the `_code` schema.

## See also

- Decision: [[ConnectIt/_decisions/2026-09-08-retire-legacy-mvvm-pipeline|decisions/2026-09-08-retire-legacy-mvvm-pipeline]]
- [[UConnectIt_State_Game|UConnectIt_State_Game]] (the state machine this fed)
- Live replacement path: [[place-piece-request|systems/place-piece-request]]
