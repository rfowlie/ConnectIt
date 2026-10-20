# Blueprint surface for designer-made AI strategies

## Context
The owner wants to build a bespoke AI in Blueprint, as a designer would: an "exhaustion" level where the AI places two
pieces per turn on random tiles, with at most one of the two completing a scoring line. A Blueprint subclass of
`UConnectIt_AIStrategy` can already receive the decision context and return a Place Piece operation, and "two pieces
per turn" is loadout configuration (the controller asks again until the turn can end). Three things are missing:

1. No Blueprint way to ask "what would this operation do?" -- operations and scoring rules are plain structs.
2. No "placeable positions" node (only loop-every-tile-and-filter).
3. The decision context doesn't say which turn it is or which decision of the turn, so a strategy can't tell the
   second piece of a turn from the first.

Owner's choice: add all three.

Step 0 after approval: move this file to `ConnectIt-Vault/ConnectIt/design/blueprint-ai-strategy-surface.md` and
delete the staging copy.

## Changes

### 1. One shared "carry out an operation" step (`ConnectIt_RuleSet.h/.cpp`, `ConnectIt_BoardRequestMediator.cpp`)
Today the Mediator's `DispatchRequest` writes out: `Apply` → `ResolveBoardChange` → `StampWinState` → append Game
Won. Move exactly that into
`float FConnectItRuleSet::CarryOutOperation(const FConnectItBoardOperation&, FConnectItBoardState& Board, FConnectItBoardChangeEvent* OutEvents) const`
(returns points scored) and have the Mediator call it. The simulation below calls the same function, so what a
designer simulates is by construction what the server would do. The AI's MinMax search is left alone (it skips the
win stamp on purpose and has its own hot path).

### 2. Blueprint functions (`ConnectIt_BoardStateLibrary.h/.cpp`)
- `Simulate Operation`:
  ```cpp
  UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board State", meta = (ExpandBoolAsExecs = "ReturnValue"))
  static bool SimulateOperation(
      const FConnectItBoardState& Board, const FConnectItRuleSet& Rules,
      const FInstancedStruct& Operation, int32 Faction,
      FConnectItBoardState& ResultBoard, float& PointsScored, int32& NumScoredEvents,
      FConnectItBoardChangeEvent& Events);
  ```
  - `Faction` is stamped onto a copy of the operation, as the server does, so a designer can't forget it.
  - False (the False pin) if the struct isn't a board operation or it can't be applied to that board; `ResultBoard`
    is then the unchanged input.
  - `NumScoredEvents` = how many things scored (for the Lines rule, how many lines were completed), so the common
    question needs no digging through `Events`.
  - The input board is never modified.
- `Get Placeable Positions (Board State)`: every position `FConnectItBoardOperation_PlacePiece::IsTilePlaceable`
  accepts.

### 3. Decision context (`ConnectIt_AIStrategy.h`, `ConnectIt_AIController.cpp`)
Add `int32 TurnNumber` and `int32 DecisionIndexThisTurn` (0 for the first move of the AI's turn, 1 for the second...)
to `FConnectItAIDecisionContext`, filled from values the controller already tracks.

## How the exhaustion AI is then built (for the design note; not code)
- **Loadout:** the AI's place action required twice per turn.
- **Blueprint strategy `BeginDecision`:**
  1. If `DecisionIndexThisTurn` is 0, clear a "scored this turn" variable.
  2. `Get Placeable Positions` on `Context.Board`; shuffle.
  3. For each: make a Place Piece operation, `Simulate Operation` with `Context.Rules` and `Context.OwnSlot`. Accept
     the first with `NumScoredEvents` = 0, or = 1 if nothing has scored yet this turn.
  4. If none qualifies (e.g. the last two tiles both score), take the first placeable one.
  5. Remember whether the chosen one scored; put the operation in the decision's payload; `FinishDecision`.
- **AI profile:** select the Blueprint strategy.

## Tests (`Private/Tests/ConnectIt_MinMaxTests.cpp`)
- `ConnectIt.Board.SimulateOperation`: a placement completing a line returns true, 4 points, 1 scored event, a result
  board with the score, and leaves the input board untouched; a placement completing two lines reports 2; an
  occupied tile returns false; a payload that isn't an operation returns false.
- `Get Placeable Positions` matches the Place Piece check on a board with an occupied and an inactive tile.
- The Mediator refactor and the context fields need a running match; covered by PIE.

## Vault
Step 0 (the design note keeps the build recipe above); log + index. No decision note (no design call changed).
Task rows left to the owner.

## Verification
- Build with the editor closed -- this also compiles the scoring-directions change still waiting -- then
  `Automation RunTests ConnectIt`, all pass.
- Owner, editor: both nodes appear under **ConnectIt > Board State**; the decision context breaks out Turn Number and
  Decision Index This Turn.
- Owner, PIE: the usual place / swap / shift check (the Mediator now goes through `CarryOutOperation`), then the
  Blueprint exhaustion AI itself.

## Status (2026-10-13): built, not yet used from Blueprint

- Build clean; `ConnectIt` tests 26/26, including the new `ConnectIt.Board.SimulateOperation` (and
  `ConnectIt.Board.Rules.LineDirections` from the scoring-directions change, which this build compiled for the first
  time).
- One difference from the plan: `FConnectItRuleSet::CarryOutOperation` covers the board change and its scoring
  (`Apply` + `ResolveBoardChange`) but not the win step. The win step had been reworked in the meantime (win groups,
  checkpoints), so the Mediator keeps calling `StampWinState` itself; `Simulate Operation` calls it too, so its result
  board carries the rule set's game-over verdict.
- Not exercised yet: a Blueprint strategy end to end (only C++ tests call the library), and the two new context
  fields (they need a running match).
