---
schema: code
kind: UCLASS
role: primary
source:
  - Source/ConnectIt/Public/Board/ConnectIt_BoardRequestMediator.h
  - Source/ConnectIt/Private/Board/ConnectIt_BoardRequestMediator.cpp
reconciled: 2026-09-21
commit: 19d1772
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
- `ProcessRequest(const FTurnActionRequest&) → bool` — **the mandatory per-action gate**, then
  `DispatchRequest` (the `RequestType` switch). The gate: the requester's PlayerState (via
  `GetPlayerStateForFaction`) must exist and `HasActionConfig()`; `Request.ActionTag` (client-supplied,
  stamped by `UTurnBasedAction::RequestBoardChange`) must resolve via `FindActionClassByTag` to an action
  in *that player's* loadout; that class's CDO must `ProducesRequestType(RequestType)`; and
  `CanUseAction` must pass. Only after `DispatchRequest` succeeds does it `ConsumeActionUse` — a
  rejected request never burns a use. There is no ungated path: an unseeded player can do nothing.
  `AConnectIt_PlayerController` relays the result to the requesting client via
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
  and **does re-run scoring** now,
  once per swapped position against its new occupying faction, then one
  `CheckWinCondition`), `HandleToggleTileActiveRequest`, `HandleCapturePieceRequest`
  (re-runs scoring — exactly one ownership change), `HandleBoardShiftRequest(Request,
  FactionID)` (added for Board Shift — unrestricted, no faction-ownership check on the
  line; re-validates every position server-side against
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
- The use budget is no longer handler-local: SWAP's budget is a `NumberedActions` entry in the loadout,
  checked and spent by the gate in `ProcessRequest` (the client-side action's own pre-check is cosmetic
  only).

## Gotchas

- **`ExecuteGameEvents()` has no body yet** and `CreateGameEventsFromBoardUpdate` isn't
  wired — the tag-reactive interpreter that turned board-change tags into piece
  spawn/despawn was removed project-wide and its replacement (`TurnBasedGameEventQueue`,
  ported from `AConnectIt_BoardManager`) is unfinished. **Nothing is reactively driven
  from board-state changes today** beyond `EnqueueBoardEventTags`' tag firing.
- No `HasAuthority()` guard — there is no client path to reach this object at all.
- **Order matters in `HandleSwapPiecesRequest`**: occupied-check → faction-ownership, both
  *before* touching `NewState`. The use is spent by `ProcessRequest` only after the handler returns true.
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

- 2026-09-21 — **Mandatory action gate added** in `ProcessRequest` (PlayerState action config, `ActionTag` in loadout,
  `ProducesRequestType`, `CanUseAction`, then `ConsumeActionUse`); dispatch moved to `DispatchRequest`; the
  swap handler's own `SwapUsesRemaining` check/consume removed. See
  [legacy action system removed](../_decisions/2026-09-20-legacy-action-system-removed-stage-3.md).
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
