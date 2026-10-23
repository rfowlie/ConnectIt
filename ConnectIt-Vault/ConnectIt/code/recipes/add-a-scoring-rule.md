---
schema: recipes
task: Change what scores -- a new kind of scoring rule (a FConnectItScoringRule struct), set on a match preset or swapped mid-match by the Level director.
touches:
  - Source/ConnectIt/Public/Board/Rules/Scoring/ConnectIt_ScoringRule.h (the base, FConnectItScoringRule; a new rule is its own file beside it)
  - Source/ConnectIt/Public/Board/Rules/Scoring/ConnectIt_LineScoringRule.h (the reference rule, Lines)
  - Source/ConnectIt/Public/Board/ConnectIt_BoardChanges.h (what a pass reports: ScoredThrough, tile changes, ClampMultiplier)
  - Source/ConnectIt/Public/Board/Rules/ConnectIt_RuleSet.h (ScoringRule; SetScoringRuleFrom)
  - Source/ConnectIt/Public/Framework/Data/ConnectIt_MatchPreset.h (the preset's Rules > Scoring Rule)
  - (mid-match) the Level director Blueprint's Set Scoring Rule
reconciled: 2026-10-23
commit: 54c1284
---

# Recipe: add a scoring rule

## Goal

A new scoring shape (e.g. a fixed pattern, squares, a different weighting of multipliers) that the board resolves
after every change, in the real game and in the AI's search alike.

A scoring rule is a **plain struct**, not a UObject: the server's rule set and the AI's background search call the
same rule, so it must be a pure function of the board and its own properties (the note at the top of
`ConnectIt_ScoringRule.h`). Decision:
[rules are thread-safe structs](../../_decisions/2026-10-06-rules-are-thread-safe-structs.md); design:
[rules as structs and shared simulation](../../design/rules-as-structs-and-shared-simulation.md).

## Prerequisites

- How a change resolves: [[tile-power-chain]]. Scoring is the chain's Score step (`ScorePass`), run on every piece
  that arrived since the last pass, then the Scoring powers react to what it ran through.
- `FConnectItScoringRule_Lines` (`ConnectIt_LineScoringRule.h/.cpp`) is the reference implementation (N in a row,
  allowed directions).

## Steps

1. **The struct** -- its own header in `Board/Rules/Scoring/` (it has real logic, so it doesn't join a family file):
   ```cpp
   USTRUCT(BlueprintType, meta = (DisplayName = "Squares"))
   struct CONNECTIT_API FConnectItScoringRule_Squares : public FConnectItScoringRule
   {
       GENERATED_BODY()

       // Tunable values: UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring")

       virtual float ScorePass(FConnectItBoardState& Board,
           TConstArrayView<FConnectItPieceArrival> Arrivals,
           FConnectItBoardChanges& OutChanges,
           FConnectItBoardChangeEvent* OutEvents) const override;

       virtual int32 GetMinPiecesToScore() const override; // 0 = this rule doesn't say
   };
   ```
2. **`ScorePass`:**
   - **Find everything that scores first**, from every arrival on the board as it is, **then apply it all**, so two
     arrivals can't break each other's scores.
   - **Apply:** add points to `Board.ScoreBoard[Faction]`, clear pieces, change multipliers -- each new multiplier
     through `OutChanges.ClampMultiplier(Tile, Value)` (the tile type's floor and ceiling).
   - **Keep the arrivals that survive:** an arrival with `bSurvivesScoring` stays on its tile when it scores (Lines'
     "kept piece"); the others are cleared with the rest.
   - **Report to `OutChanges`:** each position scoring ran through, once, in `ScoredThrough` (with the scoring faction
     and the index of its Scored event), and `AddTileChanged(Position, Multiplier, Faction, EventIndex)` for each
     multiplier it changed. Scoring powers fire on `ScoredThrough`; the next settle sees the tile changes.
   - **Events:** if `OutEvents` isn't null, append one `FConnectItBoardEvent_Scored` per thing that scored --
     `Faction`, `Points`, `Positions`, `ClearedPositions`, `NewMultipliers` (the clamped values). The AI's search
     passes null.
   - Return the points scored.
3. **`GetMinPiecesToScore`:** the fewest pieces of one faction a score can be made from. Conditions such as Not
   Enough Pieces To Score read it.
4. **Use it:** on a match preset, **Rules > Scoring Rule** → your struct, with its values. Mid-match: the Level
   director's **Set Scoring Rule** (a timing, like any level change; the GameState's rules mirror and the AI pick it
   up).

## Verify

- Tests like `ConnectIt.Board.ScoringPass` and `ConnectIt.Board.Events.ScoredEventPerLine`: the points, what is
  cleared, the multipliers, one Scored event per score; and a replay (the board before + every event's `ApplyTo` =
  the board after).
- `ConnectIt.AI.MinMax.RuleSetScoresLikeTheGame`: the AI's search scores like the game.
- `ConnectIt.AI.MinMax.Throughput`: `ScorePass` runs at every node of the search, so watch the nodes/s.
- PIE: a scoring move changes the score by the expected amount; the Scored events play.

## Pitfalls

- **The AI's evaluation only understands Lines.** Its line-potential evaluation and move ordering read
  `GetScoringRuleAs<FConnectItScoringRule_Lines>()`; under another rule the search still scores correctly, but it
  can't see a score coming. Give the MinMax evaluation a term for your shape if the AI should play towards it.
- **Applying while finding** -- a score found after an earlier one was applied can miss pieces that one cleared.
  Find all, then apply.
- **Reporting a position twice in `ScoredThrough`** fires its Scoring power once anyway, but the record should list it
  once (check `WasScoredThrough`).
- **A multiplier set without `ClampMultiplier`** breaks the tile types' limits (a ceiling-0 tile scoring).
- **No UObject references, no mutable state:** it runs on the AI's worker thread.
- **`ScoreBoard` index = faction slot**; check `IsValidIndex`.

## See also

- Flow: [[tile-power-chain]], [[place-piece-request]]
- [[ConnectItStructs|ConnectItStructs]]; the retired class this replaced: [[UConnectIt_BoardRules]]
- Recipes: [[script-a-level]] (Set Scoring Rule), [[add-a-win-condition]]
- In-repo: [[ConnectIt/CLAUDE|ConnectIt overview]].
