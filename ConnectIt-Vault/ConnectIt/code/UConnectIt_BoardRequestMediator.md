---
schema: code
kind: UCLASS
role: primary
source:
  - Source/ConnectIt/Public/Board/ConnectIt_BoardRequestMediator.h
  - Source/ConnectIt/Private/Board/ConnectIt_BoardRequestMediator.cpp
reconciled: 2026-09-18
commit: 9187568
---

# UConnectIt_BoardRequestMediator

Plain `UObject`, **server-only**, constructed and owned by
[[AConnectIt_GameMode|AConnectIt_GameMode]]. Accepts a validated
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
- **Private handlers:** `HandlePlacePieceRequest` (validates via
  `BoardRules->IsTilePlaceable` — a pluggable rule now, not the hardcoded
  `Current.IsTileValidForPlacement` the TODO used to flag), `HandleForcePlacePieceRequest`
  (skips that check entirely — only requires the position to be registered, so it can
  overwrite an inactive/occupied tile), `HandleDestroyTileMultiplierRequest`,
  `HandleRemovePieceRequest` (rejects `DelayTurns > 0` — no per-turn ticking yet),
  `HandleSwapPiecesRequest(Request, FactionID)` (requires exactly one of the two
  positions to belong to `FactionID` — a trade, not an arbitrary reposition — checks and
  consumes the acting player's `SwapUsesRemaining` via
  [[AConnectIt_PlayerState|AConnectIt_PlayerState]], and **does re-run scoring** now,
  once per swapped position against its new occupying faction, then one
  `CheckWinCondition`), `HandleToggleTileActiveRequest`, `HandleCapturePieceRequest`
  (re-runs scoring — exactly one ownership change), `HandleBoardShiftRequest(Request,
  FactionID)` (added for Board Shift — unrestricted, no faction-ownership check on the
  line and no use-budget, unlike SWAP; re-validates every position server-side against
  the actual board state, rotates whole `FConnectItTileData` one step along the
  *shiftable* subset — `bCanShift == false` tiles are skipped and keep their own data,
  wrapping the far end to the near end — then re-runs scoring on every now-occupied
  destination).
- `FactionID` rides the `FTurnActionRequest` envelope, not each payload struct.

## Collaborators

- `GetBoardState()` → [[UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]]
  on the GameState. Handlers build a working `FConnectItBoardState`, run
  [[UConnectIt_BoardRules|UConnectIt_BoardRules]] (`IsTilePlaceable` / `ApplyScoring` /
  `CheckWinCondition`) on it, then commit via `SetBoardState(NewState, ChangeEvent)`.
- `BoardRules` (injected via `Initialise`).
- `HandleSwapPiecesRequest` resolves the acting player's
  [[AConnectIt_PlayerState|AConnectIt_PlayerState]] via
  `UConnectIt_GameUtilityLibrary::GetPlayerStateForFaction` — the server-authoritative
  budget check/decrement; the client-side action's own pre-check is cosmetic only.

## Gotchas

- **`ExecuteGameEvents()` has no body yet** and `CreateGameEventsFromBoardUpdate` isn't
  wired — the tag-reactive interpreter that turned board-change tags into piece
  spawn/despawn was removed project-wide and its replacement (`TurnBasedGameEventQueue`,
  ported from `AConnectIt_BoardManager`) is unfinished. **Nothing is reactively driven
  from board-state changes today** beyond `EnqueueBoardEventTags`' tag firing.
- No `HasAuthority()` guard — there is no client path to reach this object at all.
- **Order matters in `HandleSwapPiecesRequest`**: occupied-check → faction-ownership →
  budget-check, all *before* touching `NewState`; the `SwapUsesRemaining` decrement only
  happens after `SetBoardState` succeeds — never burn a use on a request that gets
  rejected.
- `HandleSwapPiecesRequest`'s per-position scoring calls mean the rare case of *both*
  swapped positions completing a line for their (different) factions in one swap can only
  name one of them in `ChangeEvent.ScoringFactionSlot` (a single-value field) — `ScoreBoard`
  itself is still correct for both, only the UI-facing event report is a simplification.

## Cross-impact

A new request type: this class (`ProcessRequest` dispatch + `HandleXRequest`),
`ConnectIt_Structs.h` (`FConnectItRequest*` payload), `FConnectItBoardChangeEvent`
(result fields), the gameplay tag, `UConnectIt_BoardStateComponent::EnqueueBoardEventTags`,
and a `UTurnBasedAction` subclass that sends it.

## Changes

- 2026-09-18 — **`HandleBoardShiftRequest` added** (see Entry points) — the
  `ConnectIt_Game_Shift` dispatch branch and handler for Board Shift. Explicitly ruled
  out `UGridMechanics_GridShiftLibrary`/`FShiftOperation` for this (orthogonal shifts
  only, no diagonal support) in favor of a direct rotation computed here.
  (`process-code` sweep — commit `9187568`.)
- 2026-09-14 — `HandlePlacePieceRequest` now validates through
  `BoardRules->IsTilePlaceable` instead of the hardcoded
  `Current.IsTileValidForPlacement` (closes the TODO this page used to cite verbatim).
  `HandleSwapPiecesRequest` gained a `FactionID` param, faction-ownership validation,
  `SwapUsesRemaining` budget check/consume, and re-run scoring — the "does not re-run
  scoring" gotcha this page previously documented no longer applies. See
  [[ConnectIt/_discussions/2026-09-14-swap-implementation-qa|_discussions/2026-09-14-swap-implementation-qa]].
- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[ConnectIt/CLAUDE|ConnectIt overview]].
- [[place-piece-request|systems/place-piece-request]] Â·
  [[add-a-board-request-type|recipes/add-a-board-request-type]]
