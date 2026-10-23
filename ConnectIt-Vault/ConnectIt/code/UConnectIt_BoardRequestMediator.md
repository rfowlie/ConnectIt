---
schema: code
kind: UCLASS
role: primary
source:
  - Source/ConnectIt/Public/Board/ConnectIt_BoardRequestMediator.h
  - Source/ConnectIt/Private/Board/ConnectIt_BoardRequestMediator.cpp
reconciled: 2026-10-22
commit: 27b1807
---

# UConnectIt_BoardRequestMediator

Plain `UObject`, **server-only**, constructed and owned by [[AConnectIt_GameMode|AConnectIt_GameMode]]. It carries out
board changes and knows nothing about any particular kind: a request's payload **is** the change -- an
`FConnectItBoardOperation` (place this piece here, swap these two...) -- and every request takes the same path.

## When you touch this

- The gate a player's (or the AI's) request passes: loadout, action, uses.
- The common carry-out path: `CanApply`, the rule set's `CarryOutOperation`, the win stamp, the commit.
- Level changes (the Level director's operations, outside any move).

## Entry points

- `Initialise(const FConnectItRuleSet* InRules)` — once, from `AConnectIt_GameMode` (its own rule set).
- `ProcessRequest(const FTurnActionRequest&, RequestingFaction) → bool` — **the mandatory per-action gate**, then
  `DispatchRequest`. The gate: the payload is a board operation; the requester's PlayerState exists and
  `HasActionConfig()`; `Request.ActionTag` (client-supplied) resolves via `FindActionClassByTag` to an action in *that
  player's* loadout; that class's CDO `ProducesRequestType(operation's type)`; a shift's `Steps` equals the action's
  `ShiftSteps`; `CanUseAction` passes. Only after a successful dispatch does it `ConsumeActionUse` — a rejected request
  never burns a use.
- `ProcessLevelOperation(const FInstancedStruct& Operation, int32 AsFaction) → bool` — the Level director's path: no
  loadout gate, no use; straight to `DispatchRequest`. The GameMode calls it when the board is free.
- `IsDispatching()` — true for the synchronous carry-out (the GameMode's board-busy check includes it).
- **`DispatchRequest`** (private): game over? → stamp the server-known faction on a copy of the operation → `CanApply`
  on the current board → `FConnectItRuleSet::CarryOutOperation` on a copy (the operation's `Apply`, then
  `ResolveBoardChange` scoring) → `StampWinState` (checkpoints, lose conditions, the win) → `Game Won` on the transition
  → `SetBoardState(NewState, ChangeEvent)`.

## Collaborators

- `RequestingFaction` comes from the server (the sending controller's PlayerState slot, or the AI's); a payload never
  says who it is from.
- The rule set (`FConnectItRuleSet`, the GameMode's) does all the game logic: operations and scoring are plain structs
  the AI's search runs too, so a simulated move and a real one can't disagree.
- Commits through [[UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]]`::SetBoardState`, which plays the
  event list and starts the GameMode's board-change lock.
- `AConnectIt_PlayerController` relays the result to the requesting client (`ClientNotifyBoardChangeOutcome`).
- Flow: [[place-piece-request]].

## Gotchas

- **No `HasAuthority()` guard** — there is no client path to this object.
- The operation is copied before the faction is stamped: the client-sent one is never trusted or mutated.
- Level-only operations (Change Goals, Begin Phase, Remove / Add Tiles, Change Layout) are in the operation catalog
  like any other; only the loadout gate keeps players from sending them (no action produces them).
- `CreateGameEventsFromBoardUpdate` / `ExecuteGameEvents` are leftovers of a removed interpreter pipeline (no body);
  visuals come from the board state component's event queue, not from here.

## Cross-impact

A new kind of change is a new operation struct (+ event) -- nothing in this class changes; a new action that sends it
needs `ProducesRequestType` ([[add-a-board-request-type]]). A request field the client could abuse (like Shift's
`Steps`) needs a check in `ProcessRequest` against the sending action.

## Changes

- 2026-10-22 — page rewritten: operations are the requests (the per-type `HandleXRequest` handlers and `FConnectItRequest*`
  payloads are gone -- one `DispatchRequest` path); `Initialise` takes the GameMode's `FConnectItRuleSet`
  (`UConnectIt_BoardRules` retired); `RequestingFaction` supplied by the server; Shift's `Steps` checked against the
  action; `ProcessLevelOperation` for the Level director; `IsDispatching`. See
  [board operation is the request](../_decisions/2026-10-07-board-operation-is-the-request.md),
  [payload is the request](../_decisions/2026-10-07-payload-is-the-request-server-supplies-faction.md).
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
- [[place-piece-request|systems/place-piece-request]] ·
  [[add-a-board-request-type|recipes/add-a-board-request-type]]
