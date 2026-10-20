# Board operations

Design + plan note (2026-10-07). Approved in plan mode. First slice of Phase B in [board-request-objects](board-request-objects.md).

## Context
`FConnectItMinMaxRules::GenerateMoves` is hardcoded to "empty tiles for placing a piece", and `ApplyMove` both places
the piece *and* runs scoring. Meanwhile the Mediator's handlers each hand-write the same sequence (validate → change
the board → score touched tiles → win check). Owner's direction: a move type should own "which moves exist" and "how a
move changes the board"; scoring and other follow-on effects come afterwards, as a separate step.

Decisions (owner): the type is **`FConnectItBoardOperation`** ("Action" already means the player-side input classes);
the list lives **in the rule set**; the **Mediator uses the same operations now for Place and Swap** (Shift untouched --
its line is still computed client-side); the **AI's search still only places** (Swap for the AI stays the wishlist
task: it needs a turn model, use budgets and move-count control). This delivers the first slice of Phase B in
`design/board-request-objects.md`.

## Design

### Operations (`Source/ConnectIt/{Public,Private}/Board/Operations/`, new)
- `FConnectItBoardMove` -- compact value (the search makes hundreds of thousands per second): `OperationIndex` (into the
  rule set's list), `TileA`, `TileB` (tile indices; `TileB` unused by Place). A direction field joins when Shift is
  ported -- not added now.
- `FConnectItBoardOperation` (USTRUCT with virtuals; plain thread-safe data, same contract as the rules):
  - `GetRequestType()` -- the request tag this operation serves (`ConnectIt_Game_PlacePiece`, ...).
  - `GenerateMoves(Board, Rules, Faction, OperationIndex, OutMoves)` -- every legal move, appended.
  - `IsMoveValid(Board, Rules, Faction, Move, FString* OutWhyNot)` -- the server's check of a client request.
  - `ApplyMove(Board&, Faction, Move, OutTouchedTiles)` -- **changes the board only**; reports the tiles it touched.
  - `MoveFromPayload` / `MakePayload` -- to and from the request payload structs the game already uses.
  - `WriteChangeEvent(BoardBefore, Faction, Move, Event&)` -- its kind-specific event fields, for visuals.
- `FConnectItBoardOperation_PlacePiece`: generate = tiles the placement rule allows; valid = same rule; apply = set the
  piece, touched = that tile; event = `bPiecePlaced` fields.
- `FConnectItBoardOperation_SwapPieces`: valid = both occupied and exactly one owned (today's check); apply = swap the
  two pieces, touched = both; event = `bPiecesSwapped` fields. **`GenerateMoves` deliberately generates nothing yet** --
  nothing uses swap moves until the AI can model them (wishlist task).

### Rule set (`Board/Rules/ConnectIt_RuleSet.*`)
- `TArray<TInstancedStruct<FConnectItBoardOperation>> BoardOperations` (default: Place Piece, Swap Pieces);
  `FindBoardOperation(RequestType, OutIndex)`, `GetBoardOperation(Index)`.
- **The separate after-move step:** `ResolveBoardChange(Board&, TouchedTiles, OutDetails = nullptr)` -- scores each touched
  tile that now holds a piece, for whoever occupies it (what the Place/Swap/Shift handlers each do by hand today). The
  Mediator asks for details (points, scoring positions, first scoring faction); the search passes none. Reactions will
  slot in here later.

### Mediator (`Board/ConnectIt_BoardRequestMediator.*`)
- `DispatchRequest`: if the rule set has an operation for `Request.RequestType`, run one generic
  `HandleBoardOperationRequest`: payload → move → `IsMoveValid` (reject with the reason) → copy board → `ApplyMove` →
  `ResolveBoardChange` → `StampWinState` → change event (`WriteChangeEvent` + the common scoring/win fields) →
  `SetBoardState`. `HandlePlacePieceRequest` / `HandleSwapPiecesRequest` and their dispatch branches are deleted.
- Behaviour kept identical, including `ScoringFactionSlot` when nothing scores. The action-use gate in `ProcessRequest`
  is unchanged. Shift / Capture / ForcePlace / the other handlers are untouched.

### Search (`MinMax/ConnectIt_MinMaxRules.*`, `ConnectIt_MinMaxTerms.*`)
- `FMove` = `FConnectItBoardMove`. The rules object is given, per side, which request types that side may use, and
  resolves them to operations once.
- `GenerateMoves` = ask each of the side-to-move's operations. `ApplyMove` = the operation applies → the rule set
  resolves → the turn passes (still one move per turn -- the remaining hardcoded assumption, now isolated in one line).
- Ordering terms take the move (`Score(Rules, Board, Move, Side)`); the two existing terms use its `TileA`.

### AI side (`AI/ConnectIt_AIStrategy*.{h,cpp}`, `Framework/Controller/ConnectIt_AIController.cpp`)
- `FConnectItAIDecisionContext` gains `OwnRequestTypes` / `OpponentRequestTypes`: which of the rule set's operations
  each side's loadout grants (the controller asks each loadout action class `ProducesRequestType`, on the game thread).
- `UConnectIt_AIStrategy_MinMax`: passes each side's types ∩ what the search can model today (**PlacePiece only**, a
  constant with a comment pointing at the wishlist task). The decision is built from the chosen move's operation
  (`GetRequestType` + `MakePayload`) -- no more hardcoded place-piece payload.

## Tests (`Private/Tests/ConnectIt_MinMaxTests.cpp`)
- Existing 9 ported (rules constructor, `FMove`, position checks via the board).
- New: `PlaceOperation` (valid/invalid, apply, touched), `SwapOperation` (validity cases, apply, touched, and a swap
  that completes a line scores for the right faction through `ResolveBoardChange`), `NoOperationNoMoves` (a side whose
  loadout doesn't grant Place generates no moves).

## Vault
Step 0: plan moved here from the plan-mode staging file (done); decision note; `design/board-request-objects.md` updated (which part of Phase B now exists; Shift and the
step-list event still to do); log + index. Task rows left to the owner.

## Verification
- Build (editor closed); `Automation RunTests ConnectIt.AI` all pass; Throughput within ~10% of 386k nodes/s.
- Owner, in editor: the level config's **Rules → Board Operations** shows Place Piece and Swap Pieces (if a config's
  list were empty, place/swap requests are rejected with a clear error).
- Owner, PIE: 2 players -- place and swap behave, score and animate exactly as before (same change events), swap use
  limits still enforced, shift unaffected; vs AI -- still places, wins and blocks.

## Status (2026-10-07): built, not yet played

- Everything above is implemented as planned. Build clean; `ConnectIt.AI` 12/12 (9 ported + `PlaceOperation`,
  `SwapOperation`, `NoOperationNoMoves`); Throughput 407k nodes/s (386k before).
- One addition to the plan: a Place or Swap request arriving when the rule set has no operation for it is rejected
  with an error naming **Rules → Board Operations**, rather than falling through silently.
- Open: the owner's editor and PIE checks under Verification.
- Decision: [board operations own moves](../_decisions/2026-10-07-board-operations-own-moves.md).

## Revision 1 (2026-10-07)

After the owner's review: the placement rule became one concrete function on the Place operation, the change event
reports `ScoringConfigurations`, and a move is gameplay tag + typed data (no index, no generic tile slots). Plan and
detail: [board-operations-revision-1](board-operations-revision-1.md). Parts of the text above that mention
`OperationIndex`, `TileA`/`TileB`, `FConnectItBoardResolution` or the placement rule describe the first version.

## Revision 2 (2026-10-07)

The operation became the request itself: it carries its own data, actions send it, all eight request types are
operations and the Mediator has one path. Plan and detail:
[board-operations-revision-2](board-operations-revision-2.md). This note and revision 1 describe earlier shapes.
