# Slim the request pipeline: the payload is the request; the server supplies the faction

## Context
A board operation now carries everything about a move, yet it still travels inside `FTurnActionRequest`, which
repeats two of its facts (`RequestType`, `FactionID`) and has an unused field (`AdditionalData`). Reviewing it showed:

- `RequestType` on the envelope exists only to be compared with the operation's own type (the Mediator's mismatch
  check is there because the type is stated twice).
- **`FactionID` is a hole.** The action fills it in on the client and the server only overwrites it when unset
  (`AConnectIt_PlayerController::ServerRouteBoardChangeRequest`, where the owner's TODO already suspects this). The
  Mediator looks up the player state by that number, so a modified client could send the opponent's faction on its
  own turn and act as them.
- `ActionTag` is the one envelope field that must stay: it says which loadout action is being spent, which the
  operation doesn't know and the server can't always derive (two actions may send the same kind of operation).

Owner's choices: **slim it down**; board history **not now** (record the discussion only).

Step 0 after approval: move this file to `ConnectIt-Vault/ConnectIt/design/slim-request-pipeline.md` and delete the
staging copy.

## Changes

### Plugin `UnrealTurnBasedMechanics`
- `TurnBasedMechanicsStructs.h`: `FTurnActionRequest` becomes `{ FGameplayTag ActionTag; FInstancedStruct Payload; }`.
  `RequestType`, `FactionID`, `AdditionalData` removed; `IsValid()` = payload is set; `operator==` compares the two.
- `Action/TurnBasedAction.h/.cpp`: `RequestBoardChange(const FInstancedStruct& Payload)` -- the action hands over
  just the payload; the base class builds the request and stamps `ActionTag`, as it does today.
- `TurnBasedActionsComponent` and `DWidget_TurnBasedActionsComponent`: unchanged in behaviour (they pass the request
  through and match the pending one by equality).
- The plugin no longer has any notion of a request "type" or of who is asking.

### Game
- **Actions** (`ConnectIt_PlacePieceAction.cpp`, `ConnectIt_SwapPieceAction.cpp`, `ConnectIt_BoardShiftAction.cpp`):
  build the operation, `RequestBoardChange(FInstancedStruct::Make(Operation))`. No envelope code.
- **Player controller** (`AConnectIt_PlayerController.cpp`): after the existing "is it this participant's turn"
  check, take the faction from the controller's own PlayerState slot (reject if it has none) and pass it on. The
  "stamp if not set" block and its TODOs go.
- **GameMode / Mediator** (`ConnectIt_GameMode.*`, `ConnectIt_BoardRequestMediator.*`):
  `ProcessBoardRequest(Request, int32 RequestingFaction)` → `ProcessRequest(Request, RequestingFaction)`.
  - The Mediator reads the operation out of the payload first (reject if it isn't one) and takes the request type
    from the operation for the loadout gate and for logs.
  - The type-mismatch check is deleted: there is nothing left to mismatch.
  - `Operation.Faction` is set from `RequestingFaction`, never from anything the client sent.
- **AI** (`ConnectIt_AIController.cpp`, `ConnectIt_AIStrategy.h`, `ConnectIt_AIStrategy_MinMax.cpp`): the controller
  passes its own slot as the requesting faction. `FConnectItAIDecision::RequestType` is removed for the same reason
  as the envelope's (it repeated the payload's type); "has a move" = payload is set, and the controller finds the
  loadout action from the payload operation's type.
- **Debug widget** (`ConnectIt_DebugStateWidget`): signature unchanged; it only refreshes.

## Not changed
- What a client may do: still gated by its loadout action, turn order and action uses.
- `ClientNotifyBoardChangeOutcome` still echoes the request back for the pending-request match.
- No Blueprint asset references `FTurnActionRequest` or `RequestBoardChange`.

## Board history (not built)
Recorded as an open question note in `ConnectIt/_questions/`: memory is not a concern (about 1.5 KB per 7x7 state),
a growing history should not be replicated, and a move list of applied operations is the better thing to keep
(states can be rebuilt from it). Build when something needs it.

## Vault
Step 0; decision note "the payload is the request; the server supplies the faction" (records the hole and its fix);
the question note above; log + indexes. The plugin change is logged under `ConnectIt` (the `UnrealTurnBasedMechanics`
domain has no `_logs/`, to be confirmed). Task rows left to the owner.

## Verification
- Build with the editor closed -- this also compiles the two changes still waiting (event library, swap keeps
  nothing) -- then `Automation RunTests ConnectIt`, all pass. The pipeline itself needs a running match; no unit test.
- Owner, PIE 2-player: place, swap, shift from both the host and a client; an out-of-turn or exhausted action is
  still rejected and the client recovers (awaiting-confirmation clears). Vs AI: still plays.

## Status (2026-10-07): built, not yet played

- Implemented as planned. Build clean; `ConnectIt` tests 20/20, no Blueprint compile errors in the run. This build
  also compiled the two changes that had been waiting (event library, swap keeps nothing).
- One detail: `DispatchRequest` takes the payload (an instanced struct) rather than the operation, so it can make its
  own copy before stamping the faction.
- The request pipeline has no unit test (it needs a running match): PIE checks under Verification are open.
- Board history: parked as [a question note](../_questions/board-history.md).
- Decision: [the payload is the request; the server supplies the faction](../_decisions/2026-10-07-payload-is-the-request-server-supplies-faction.md).
