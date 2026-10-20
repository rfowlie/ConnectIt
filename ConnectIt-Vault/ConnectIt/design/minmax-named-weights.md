# MinMax judgement: named weights and handwritten evaluation, instead of term structs

## Context
The MinMax AI's judgement is built from two editor lists of polymorphic "term" structs
(`FConnectItMinMaxEvalTerm`, `FConnectItMinMaxOrderTerm`), each looped over generically. The owner's notes on the
ordering terms: every term is forced through one signature (`Score(Rules, Board, Move, Side)`), so
`AdjacentPieces` and `TileMultiplier` have to dig a position out of a move they don't care about and only work for
Place Piece; the only caller is `FConnectItMinMaxRules::EvaluateMove`; a float return is odd. A handwritten
`EvaluateMove` calling plain helper functions would let each piece of logic take exactly the inputs it needs.

Owner's choices: expose the weights as a **struct of named floats** (not a gameplay-tag map); **convert the
evaluation terms the same way** for consistency.

This reverses the 2026-10-02 decision "MinMax evaluation as editor terms".

Step 0 after approval: move this file to `ConnectIt-Vault/ConnectIt/design/minmax-named-weights.md` and delete the
staging copy.

## What changes

### 1. Weights (new `Public/MinMax/ConnectIt_MinMaxWeights.h`, replacing `ConnectIt_MinMaxTerms.h/.cpp`)
```cpp
// What the AI values in an unfinished position. 0 turns a factor off.
USTRUCT(BlueprintType)
struct FConnectItMinMaxEvaluationWeights
{
    // Real points: my score minus the opponent's
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float ScoreDifference = 1000.f;
    // Lines in the making (pieces² × window multiplier), mine minus theirs
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float LinePotential = 10.f;
};

// Which moves the search tries first. Affects speed only, never which move wins.
USTRUCT(BlueprintType)
struct FConnectItMinMaxOrderingWeights
{
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float TileMultiplier = 10.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float AdjacentPieces = 5.f;
};
```
Same default values as today's terms. `FConnectItMinMaxMove` (the search's move type) moves to
`ConnectIt_MinMaxRules.h`.

### 2. `FConnectItMinMaxRules` (`Public/MinMax/ConnectIt_MinMaxRules.h`, `Private/MinMax/ConnectIt_MinMaxRules.cpp`)
- Constructor takes the two weight structs (by value copy) instead of the two term arrays; the owned term arrays and
  "active term" pointer lists go.
- The logic becomes plain private helpers, each taking only what it needs (bodies moved verbatim from the terms):
  - `float ScoreDifference(Board, Side)`
  - `float LinePotential(Board, Side)` (one faction's side; uses `Geometry.LineWindows`)
  - `float TileMultiplierAt(Board, TileIndex)`
  - `int32 CountAdjacentPieces(Board, TileIndex)` (uses `Geometry.Neighbours`)
- `EvaluateState` is handwritten: weighted sum of the two evaluation factors, skipping a factor whose weight is 0
  (`LinePotential` is the expensive one), then the existing clamp clear of the win band.
- `EvaluateMove` is handwritten per kind of move: for a Place Piece operation, look up its tile
  (`Geometry.TileIndexAt`) and combine `TileMultiplierAt` and `CountAdjacentPieces`. A future kind of move gets its own
  branch and decides which tiles matter to it.

### 3. Strategy (`Public/AI/ConnectIt_AIStrategy_MinMax.h`, `.cpp`)
- `EvaluationTerms` / `OrderingTerms` arrays → `EvaluationWeights` / `OrderingWeights` properties (same categories).
  Constructor no longer adds default terms.
- The "no EvaluationTerms" warning becomes "all evaluation weights are 0".

### 4. Tests
- `ConnectIt_MinMaxTests.cpp`: helpers return the strategy's default weights; constructor calls updated.
- `EvaluationTermsDriveChoice` → `EvaluationWeightsDriveChoice`: faction 0 has three in a row; with only
  `ScoreDifference` weighted the AI completes the line (real points); with only `LinePotential` weighted it does not
  (completing clears the line and its potential). Same intent: the weights alone change the choice.
- `ConnectIt_MinMaxTestTerms.h`: remove the test-only evaluation term; the test-only win condition stays.

## Consequences to know
- Adding a new evaluation or ordering idea is now: a weight, a helper, a line in `EvaluateState` / `EvaluateMove`
  (no longer a new struct picked in the editor).
- **AI profile assets lose their saved term lists** and take the default weights (the same numbers the default terms
  had). Any weights changed on a profile need re-entering.

## Vault
Step 0; decision note "MinMax judgement is named weights" (supersedes 2026-10-02 minmax-evaluation-as-editor-terms --
mark it superseded); log + indexes. Task rows left to the owner.

## Verification
- Build (editor closed); `Automation RunTests ConnectIt` all pass; report Throughput against 362k nodes/s (expected
  the same or better: no virtual call per term).
- Owner, editor: an AI profile's MinMax strategy shows **Evaluation Weights** and **Ordering Weights** with the four
  defaults. PIE vs AI: plays as before.

## Status (2026-10-07): built, not yet played

- Implemented as planned. Build clean; `ConnectIt` tests 19/19; Throughput 390k nodes/s against 362k.
- Beyond the plan: the strategy's now-empty constructor was removed.
- Open: the owner's editor and PIE checks under Verification.
- Decision: [MinMax judgement is named weights](../_decisions/2026-10-07-minmax-judgement-is-named-weights.md).
