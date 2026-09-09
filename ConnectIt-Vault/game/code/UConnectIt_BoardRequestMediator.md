---
schema: code
kind: UCLASS
role: primary
source:
  - Source/ConnectIt/Public/Board/ConnectIt_BoardRequestMediator.h
  - Source/ConnectIt/Private/Board/ConnectIt_BoardRequestMediator.cpp
reconciled: 2026-09-07
commit: 668872e
---

# UConnectIt_BoardRequestMediator

Plain `UObject`, **server-only**, constructed and owned by
[[game/code/AConnectIt_GameMode|AConnectIt_GameMode]]. Accepts a validated
`FTurnActionRequest`, unwraps its `Payload` into the concrete `FConnectItRequest*` struct
the `RequestType` expects, and routes to a per-type `HandleXRequest`. The successor to the
retired `AConnectIt_BoardManager::ProcessRequest`.

## When you touch this

- Adding a board-change request type (a `HandleXRequest`).
- Working on the (in-progress) game-event queue that turns board changes into visuals.

## Entry points

- `Initialise(UConnectIt_BoardRules*)` — once, from `AConnectIt_GameMode`.
- `ProcessRequest(const FTurnActionRequest&) → bool` — dispatch by `RequestType`; returns
  success. `AConnectIt_PlayerController` relays the result to the requesting client via
  `ClientNotifyBoardChangeOutcome` so `UTurnBasedActionsComponent` can clear its
  awaiting-confirmation state.
- **Private handlers:** `HandlePlacePieceRequest`, `HandleForcePlacePieceRequest` (skips
  `IsTileValidForPlacement`), `HandleDestroyTileMultiplierRequest`,
  `HandleRemovePieceRequest` (rejects `DelayTurns > 0` — no per-turn ticking yet),
  `HandleSwapPiecesRequest` (**does not re-run scoring** — a swap changes two positions
  and there's no single-position scoring call that fits; known gap),
  `HandleToggleTileActiveRequest`, `HandleCapturePieceRequest` (**does** re-run scoring —
  exactly one ownership change).
- `FactionID` rides the `FTurnActionRequest` envelope, not each payload struct.

## Collaborators

- `GetBoardState()` → [[game/code/UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]]
  on the GameState. Handlers build a working `FConnectItBoardState`, run
  [[game/code/UConnectIt_BoardRules|UConnectIt_BoardRules]] (`ApplyScoring` /
  `CheckWinCondition`) on it, then commit via `SetBoardState(NewState, ChangeEvent)`.
- `BoardRules` (injected via `Initialise`).

## Gotchas

- **`ExecuteGameEvents()` has no body yet** and `CreateGameEventsFromBoardUpdate` isn't
  wired — the tag-reactive interpreter that turned board-change tags into piece
  spawn/despawn was removed project-wide and its replacement (`TurnBasedGameEventQueue`,
  ported from `AConnectIt_BoardManager`) is unfinished. **Nothing is reactively driven
  from board-state changes today** beyond `EnqueueBoardEventTags`' tag firing.
- No `HasAuthority()` guard — there is no client path to reach this object at all.
- `HandleSwapPiecesRequest` intentionally skips scoring/win checks — don't "fix" it
  without deciding the two-position scoring semantics.

## Cross-impact

A new request type: this class (`ProcessRequest` dispatch + `HandleXRequest`),
`ConnectIt_Structs.h` (`FConnectItRequest*` payload), `FConnectItBoardChangeEvent`
(result fields), the gameplay tag, `UConnectIt_BoardStateComponent::EnqueueBoardEventTags`,
and a `UTurnBasedAction` subclass that sends it.

## See also

- In-repo: `old/Source/ConnectIt/Docs/Workflows/ServerAuthoritative-ActionRequest.md`; `old/Source/ConnectIt/Docs/LegacyPipeline.md`.
- [[game/systems/place-piece-request|systems/place-piece-request]] ·
  [[game/recipes/add-a-board-request-type|recipes/add-a-board-request-type]]
