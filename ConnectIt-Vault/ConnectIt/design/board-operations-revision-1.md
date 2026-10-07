# Board operations, revision 1: concrete placement check, scoring configurations, tag + typed move

## Context
Follow-up to the board-operations work (built 2026-10-07, uncommitted, 12/12 tests). The owner reviewed it and raised
three things:

1. `FConnectItTilePlaceableRule` is a swappable rule for something that may depend on many factors; it invites a
   pile of small rule structs. Commit to one concrete function on the Place Piece operation instead.
2. The change event says "line" (`bLineScored`, `ScoringLinePositions`) although scoring need not be lines.
3. `FConnectItBoardMove { OperationIndex, TileA, TileB }` is confusing: `TileB` only means something for a swap, and
   operations should be told apart by gameplay tag, not an index.

Owner's choices: move = **gameplay tag + the operation's own typed data** (TVariant); scoring = **a list of scoring
configurations** (not a plain rename); **also rename** the `LineScored` tag and game event class.

Step 0 after approval: move this file to `ConnectIt-Vault/ConnectIt/design/board-operations-revision-1.md` and delete
the staging copy.

## 1. Placement: one concrete function
- Add to `FConnectItBoardOperation_PlacePiece` (`Public/Board/Operations/ConnectIt_BoardOperation_PlacePiece.h`,
  inline -- it runs for every tile of every position the search looks at):
  `static bool IsTilePlaceable(const FConnectItBoardState& Board, int32 TileIndex)` (active and unoccupied today) and
  a `FGridPosition` overload that resolves the index. This is the single place placement conditions are added.
- Delete `Public/Board/Rules/ConnectIt_TilePlaceableRule.h` (base + `_Unoccupied`).
- `FConnectItRuleSet`: remove `TilePlaceableRule`, `GetTilePlaceableRule`, `IsTilePlaceable`.
- Remove the duplicate `FConnectItBoardState::IsTileValidForPlacement` (`Public/ConnectIt_Structs.h`) and point its
  callers at the operation's function: `ConnectIt_PlacePieceAction.cpp` (hover check),
  `ConnectIt_GameUtilityLibrary.cpp` (empty-tiles list), `AConnectIt_GameState::IsTileValidForPlacement`,
  `UConnectIt_BoardStateLibrary::IsTileValidForPlacement`. The last two keep their Blueprint-callable names.
- `ConnectIt_MinMaxRules.cpp`: drop "placement rule missing" from the constructor warning.
- Level config assets: the saved `TilePlaceableRule` value is simply dropped on load.

## 2. Scoring configurations
- New in `ConnectIt_Structs.h`:
  `USTRUCT(BlueprintType) FConnectItScoringConfiguration { int32 FactionSlot; float Points; TArray<FGridPosition> Positions; }`
  -- one thing that scored (for the Lines rule: one completed line).
- `FConnectItBoardChangeEvent`: remove `bLineScored`, `ScoringFactionSlot`, `PointsScored`, `ScoringLinePositions`;
  add `TArray<FConnectItScoringConfiguration> ScoringConfigurations` (empty = nothing scored). This also removes the
  "only one faction is named" limitation. (An array of structs that each hold an array replicates fine; the old
  comment's concern was a bare array of arrays.)
- `FConnectItScoringRule::ApplyScoring(Board, Position, Faction, TArray<FConnectItScoringConfiguration>* OutConfigurations)`
  -- pointer, null from the AI's search so it allocates nothing extra. `FConnectItScoringRule_Lines` adds one
  configuration per completed line (points per line already computed in `ApplyScoringLine`).
- `FConnectItRuleSet::ApplyScoring` and `ResolveBoardChange` take the same optional output;
  `FConnectItBoardResolution` is deleted.
- Mediator (`ConnectIt_BoardRequestMediator.cpp`): `HandleBoardOperationRequest` and the hand-written Force Place,
  Shift and Capture handlers pass `&ChangeEvent.ScoringConfigurations`; their per-handler scoring-faction bookkeeping
  goes away. Operations' `WriteChangeEvent` no longer sets a default scoring faction.
- `ConnectIt_BoardStateComponent.cpp`: queue the scored event when `ScoringConfigurations` is not empty.
- Renames:
  - Tag `ConnectIt.Event.LineScored` → `ConnectIt.Event.Scored` (`ConnectIt_Event_Scored` in
    `ConnectIt_GameplayTags.h/.cpp`), with `+GameplayTagRedirects` in `Config/DefaultGameplayTags.ini`.
  - `UConnectIt_LineScoreGameEvent` → `UConnectIt_ScoreGameEvent` (files renamed; it is a stub today), with a
    `+ClassRedirects` line in `Config/DefaultEngine.ini` `[CoreRedirects]`; update the comment that names it in the
    plugin's `GridPieceRegistryComponent.h`.
- **Owner's Blueprint work:** `CI_PieceVisualHandler` reads the four removed fields and must be rewired to loop
  `ScoringConfigurations`. `CI_GridTile` and `CI_PieceVisualHandler` reference the tag; the redirect should carry
  them, to be confirmed in the editor.

## 3. Move = gameplay tag + typed data
`Public/Board/Operations/ConnectIt_BoardOperation.h`:
```cpp
struct FConnectItPlaceMove { int32 Tile = INDEX_NONE; };
struct FConnectItSwapMove  { int32 TileA = INDEX_NONE; int32 TileB = INDEX_NONE; };

struct FConnectItBoardMove
{
    FGameplayTag RequestType;                             // which operation
    TVariant<FConnectItPlaceMove, FConnectItSwapMove> Data; // that operation's own data
};
```
- Fixed size, no heap allocation (the reason it is not an `FInstancedStruct`). A new operation adds its struct to the
  variant list.
- `FConnectItBoardOperation`: `GenerateMoves` loses the `OperationIndex` parameter (each operation stamps its own
  `GetRequestType()`); add `virtual FString DescribeMove(Board, Move)` so logs don't need to know the kind of move.
- `FConnectItRuleSet`: `FindBoardOperation(Tag)` only; `GetBoardOperation(int32)` and the `OutIndex` parameter go.
- Mediator: `HandleBoardOperationRequest(Operation, Request)`; sets `Move.RequestType`.
- `FConnectItMinMaxRules`: per-side lists stay as operation pointers; `ApplyMove` finds the operation by the move's
  tag in a small cached `{Tag, Operation*}` array (a handful of entries -- no virtual call or rule-set walk per node).
- Ordering terms (`ConnectIt_MinMaxTerms.cpp`): `_TileMultiplier` and `_AdjacentPieces` read
  `Move.Data.TryGet<FConnectItPlaceMove>()` and score 0 for other kinds of move.
- `ConnectIt_AIStrategy_MinMax.cpp`: operation looked up by tag; summary text from `DescribeMove`.

## Tests (`Private/Tests/ConnectIt_MinMaxTests.cpp`)
- Port the 12 to the new move shape and scoring output.
- `SwapOperation`: assert one configuration (faction 0, 4 points, 4 positions).
- Add `ScoringConfigurationsPerLine`: one placement completing two lines yields two configurations whose points sum
  to the score gained.

## Vault
- Step 0 (above). New decision note: placement is a concrete function on the Place operation (reverses the
  placement part of 2026-10-06 rules-are-thread-safe-structs; mark that note partly superseded) and the change event
  reports scoring as configurations. Amend today's board-operations decision note for the new move shape.
- Status line in `design/board-operations.md`; log + `_logs/__INDEX.md`; `_decisions/__INDEX.md`. Task rows left to
  the owner.

## Verification
- Build (editor closed); `Automation RunTests ConnectIt.AI` all pass; Throughput compared with 407k nodes/s (the move
  grows from 12 to roughly 28 bytes -- report the number either way).
- Owner, editor: level configs load (a dropped `TilePlaceableRule` may log once); `CI_PieceVisualHandler` rewired;
  `CI_GridTile` still listens to the renamed tag.
- Owner, PIE: place/swap/shift/capture score and animate; a move that completes two lines shows both; vs AI still
  places, wins and blocks; hover highlight for placing still matches what the server accepts.

## Status (2026-10-07): built, not yet played

- Implemented as planned. Build clean; `ConnectIt.AI` 13/13; Throughput 395k nodes/s (407k before the move grew).
- Beyond the plan: `GenerateMoves` and `IsMoveValid` no longer take the rule set (nothing used it once placement
  stopped being a rule); each operation has a `MakeMove` helper; the change event has `HasScored()` and
  `GetTotalPointsScored()`.
- `CI_PieceVisualHandler` fails to compile until it is rewired to `ScoringConfigurations` (owner).
- Decision: [concrete placement check and scoring configurations](../_decisions/2026-10-07-concrete-placement-check-and-scoring-configurations.md).
