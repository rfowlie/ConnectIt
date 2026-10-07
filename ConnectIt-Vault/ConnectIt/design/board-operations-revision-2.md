# Board operations, revision 2: the operation IS the request

## Context
After revision 1 a request still reaches the board through several matching steps: a request payload struct, a
gameplay-tag lookup of the operation in the rule set, a conversion to `FConnectItBoardMove` with its per-kind data
struct, and a fall-through if-chain for the six request types that have no operation. The owner finds this confusing
and asked for two things:

1. An operation should carry everything it needs (Place: the tile and the faction; Swap: the two tiles...). Actions
   send the operation itself instead of a request struct.
2. No fall-through: every request type becomes an operation.

Owner's choices: the rule set's per-level **Board Operations list is removed** (loadouts decide what a player can do);
the AI search's move type is a **TVariant of the operations it can model** (today Place Piece only).

Step 0 after approval: move this file to `ConnectIt-Vault/ConnectIt/design/board-operations-revision-2.md` and delete
the staging copy.

## 1. The operation struct
`Public/Board/Operations/ConnectIt_BoardOperation.h` (base) -- a specific move, with its data:
```cpp
USTRUCT(BlueprintType)
struct FConnectItBoardOperation
{
    // Who is making the move. Client-supplied when it arrives in a request:
    // the server overwrites it with the requester's faction before use.
    UPROPERTY(BlueprintReadWrite) int32 Faction = INDEX_NONE;

    virtual FGameplayTag GetRequestType() const;                       // for the loadout gate
    virtual bool CanApply(const FConnectItBoardState&, FString* OutWhyNot = nullptr) const;
    virtual void Apply(FConnectItBoardState&,
                       FConnectItTouchedPositions& OutTouched,         // tiles a piece arrived on
                       FConnectItBoardChangeEvent* OutEvent) const;    // null from the AI's search
    virtual FString Describe() const;                                  // for logs
};
```
- Fields are grid positions (what requests, scoring and events already use) and are all `UPROPERTY`, because the
  struct now travels inside the request RPC.
- `Apply` fills the move's own part of the change event itself (replaces `WriteChangeEvent`; Shift needs the data it
  computes while applying).
- `OutTouched` = positions where a piece arrived or changed owner -- the only ones the after-move step scores. Remove,
  Destroy Multiplier and Toggle report none, which keeps today's behaviour (they don't score).
- Still a plain thread-safe struct: no UObject references.

Concrete operations, all in one header + one cpp (`ConnectIt_BoardOperations.h/.cpp`, replacing the two per-operation
headers), each taking over its request struct's fields and its Mediator handler's validation and mutation verbatim:

| Operation | Fields | From |
| --- | --- | --- |
| `_PlacePiece` | `Position` | `FConnectItRequestPlacePiece` + rev-1 operation |
| `_SwapPieces` | `PositionA`, `PositionB` | `FConnectItRequestSwapPieces` + rev-1 operation |
| `_ForcePlacePiece` | `Position` | `HandleForcePlacePieceRequest` |
| `_CapturePiece` | `Position` | `HandleCapturePieceRequest` |
| `_Shift` | `Positions`, `Direction` | `HandleBoardShiftRequest` (unshiftable-tile rotation unchanged) |
| `_RemovePiece` | `Position`, `DelayTurns` | `HandleRemovePieceRequest` (still rejects `DelayTurns > 0`) |
| `_DestroyTileMultiplier` | `Position` | `HandleDestroyTileMultiplierRequest` |
| `_ToggleTileActive` | `Position` | `HandleToggleTileActiveRequest` |

`_PlacePiece` keeps the static `IsTilePlaceable` / `IsTilePlaceableAt` (the one placement check) and gains a static
generator for the AI: `ForEachMove(Board, Faction, Callback)` -- one Place operation per placeable tile.

## 2. Deleted
- All eight `FConnectItRequest...` structs (`Public/ConnectIt_Structs.h`).
- `FConnectItPlaceMove`, `FConnectItSwapMove`, `FConnectItBoardMove`, `MoveFromPayload`, `MakePayload`, `MakeMove`.
- `FConnectItRuleSet::BoardOperations`, `FindBoardOperation` (a saved list on a level config is dropped on load).
  `ResolveBoardChange` stays, taking touched positions.
- Every `Handle...Request` function and the if-chain in the Mediator.

## 3. Mediator (`ConnectIt_BoardRequestMediator.h/.cpp`)
`ProcessRequest` (the loadout gate) is unchanged. `DispatchRequest` becomes the only path:
1. Board exists, game not over, request valid (as today).
2. `Request.Payload.GetPtr<FConnectItBoardOperation>()` -- reject if the payload is not an operation.
3. **Reject if `Operation.GetRequestType() != Request.RequestType`.** The gate approved the envelope's type; this stops
   a client labelling, say, a Capture as a Place Piece.
4. Copy the operation and set `Faction = Request.FactionID`.
5. `CanApply` (reject with its reason) → copy board → `Apply` (with the event) → `Rules->ResolveBoardChange` (fills
   `ScoringConfigurations`) → `Rules->StampWinState` → win fields → `SetBoardState`.

## 4. Senders
- `ConnectIt_PlacePieceAction.cpp`, `ConnectIt_SwapPieceAction.cpp`, `ConnectIt_BoardShiftAction.cpp`: build the
  operation, `Request.Payload = FInstancedStruct::Make(Operation)`, `Request.RequestType = Operation.GetRequestType()`.
  `ProducesRequestType` overrides unchanged.
- AI (`ConnectIt_AIStrategy_MinMax.cpp`): the chosen move already is an operation; it becomes the decision's payload.
  `ConnectIt_AIController::SubmitDecision` unchanged.

## 5. AI search
- `ConnectIt_MinMaxRules.h`: `using FMove = TVariant<FConnectItBoardOperation_PlacePiece>;` -- the operations the
  search can model, the one place to extend when the AI learns to swap.
- `GenerateMoves`: if the side to move may place, `FConnectItBoardOperation_PlacePiece::ForEachMove`. `ApplyMove`:
  visit the variant → `Apply(Board, Touched, nullptr)` → `ResolveBoardChange` → turn passes.
- Constructor keeps its per-side request-type containers (tests unchanged there).
- `FConnectItMinMaxGeometry` gains an O(1) position → tile-index table (`TileIndexAt`), because ordering terms run for
  every candidate move and need the tile's neighbours; a linear position search there would be the hot spot.
  Ordering terms take `const FMove&` and read `TryGet<FConnectItBoardOperation_PlacePiece>()`.
- Decision context (`ConnectIt_AIStrategy.h`): replace `OwnRequestTypes` / `OpponentRequestTypes` with the two sides'
  loadouts; a base-strategy helper `LoadoutGrantsRequestType(Loadout, Tag)` (the CDO `ProducesRequestType` loop that is
  in `ConnectIt_AIController.cpp` today moves there). The MinMax strategy asks it for Place Piece per side.

## Tests (`Private/Tests/ConnectIt_MinMaxTests.cpp`)
- Port the 13 (moves are Place operations; touched positions).
- One test per newly converted operation -- Force Place, Capture, Shift (including an unshiftable tile staying put),
  Remove, Destroy Multiplier, Toggle: `CanApply` accept/reject cases, the board after `Apply`, touched positions and
  the change-event fields. These are the only coverage for the five operations nothing sends yet.
- The Mediator itself (gate, type-mismatch rejection, faction stamping) needs a running match; not unit-tested.

## Vault
Step 0; decision note "the operation is the request" (supersedes the rule-set list and the move value in today's
board-operations decision; mark that one partly superseded); status lines in `design/board-operations.md` and
`board-request-objects.md`; log + indexes. Task rows left to the owner.

## Verification
- Build (editor closed); `Automation RunTests ConnectIt.AI` all pass; report Throughput against 395k nodes/s.
- Owner, PIE 2-player: place, swap and shift work, score and animate; swap use limits hold; a rejected move logs the
  operation's reason. Vs AI: still places, wins, blocks.
- Still open from revision 1: rewire `CI_PieceVisualHandler` to `ScoringConfigurations`; confirm the tag redirect.

## Status (2026-10-07): built, not yet played

- Implemented as planned. Build clean; `ConnectIt` tests 19/19 (13 ported, 6 new under `ConnectIt.Board.Operations`).
- Throughput 362k nodes/s against 395k: about 8% slower. A Place operation names a grid position, so applying it
  looks the tile up; ordering terms use the new O(1) table.
- One behaviour difference: a shift used to try to score every occupied tile in its line, including one holding a
  non-faction blocker; the shared resolve step skips tiles with no faction's piece.
- Open: the owner's PIE checks under Verification.
- Decision: [the operation is the request](../_decisions/2026-10-07-board-operation-is-the-request.md).
