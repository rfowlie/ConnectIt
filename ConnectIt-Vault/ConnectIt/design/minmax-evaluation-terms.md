# MinMax evaluation as editor-composed terms

Design + plan note (2026-10-02), revision 3 of the Classic MinMax work (after [AI strategy + flexible MinMax rules](ai-strategy-and-minmax-rules.md)). Approved in plan mode.

## Context
Owner review (10-02, third pass): changing the AI's evaluation currently needs a C++ rules subclass AND a strategy
subclass overriding `MakeRules` — "a strategy to a strategy". The two layers exist for a real reason (the search runs
off the game thread, so it needs a plain C++ rules object, not the UObject strategy), but the swappable unit was too
coarse. What actually varies is evaluation and move ordering, so those become editor data on the one MinMax strategy.
Decisions: rename `FConnectItClassicSearchGame` → **`FConnectItMinMaxRules`**; evaluation = **weighted term
structs**; move ordering = **term structs too**. The owner's merge of `LaunchAlphaBetaAsync` into
`GI_MinMaxAlphaBeta.h` stays (trade-off accepted: that header now includes the async plumbing).

## Design

### Terms — `Source/ConnectIt/Public/MinMax/ConnectIt_MinMaxTerms.h/.cpp` (new)
- `FConnectItMinMaxEvalTerm` (USTRUCT, base): `UPROPERTY(EditAnywhere) float Weight`;
  `virtual float Evaluate(const FConnectItMinMaxRules& Rules, const FConnectItBoardState& Board, int32 Side) const`
  — value of `Board` for `Side` (positive = good for Side). Doc: called on the search thread millions of times, so a
  term must be a pure function of its inputs — no UObject references, no mutable state.
  - `_ScoreDifference` (Weight 1000): my score − their score.
  - `_LinePotential` (Weight 10): `LinePotential(Side) − LinePotential(Opponent)` (logic moved from the rules).
- `FConnectItMinMaxOrderTerm` (USTRUCT, base): `Weight`; `virtual float Score(Rules, Board, TileIndex, Side) const`.
  - `_TileMultiplier` (Weight 10), `_AdjacentPieces` (Weight 5) — today's ordering, split out.
- New term types are C++ structs; designers pick, order and weight them in the Details panel.

### Rules — `MinMax/ConnectIt_ClassicSearch.*` → `MinMax/ConnectIt_MinMaxRules.h/.cpp`
- `FConnectItClassicSearchGame` → `FConnectItMinMaxRules` (`final`, functions no longer virtual — variants come from
  terms now); alias `FConnectItClassicSearch` → `FConnectItMinMaxSearch`; geometry struct → `FConnectItMinMaxGeometry`.
- Constructor takes board, connect length, win threshold and the two term arrays; copies the arrays and resolves each
  valid entry to a `const` base pointer once, on the game thread (no reflection calls in the search). Copy/move deleted
  so those pointers can't dangle; the strategy holds it by `TSharedRef` as now.
- Keeps the fixed Classic game model: `GenerateMoves`, `ApplyMove` (real `ApplyLineScoring`), `IsTerminal`, the win /
  loss value in `Evaluate` (correctness, not taste), then `Evaluate` = Σ weight × term; `OrderScore` = Σ weight × term.
- Public read-only accessors terms use: `GetGeometry()`, `GetConnectLength()`, `GetWinThreshold()`, `Opponent()`.
- `FConnectItMinMaxEvalWeights` removed.

### Strategy — `UConnectIt_AIStrategy_MinMax`
- Remove `EvalWeights` and the `MakeRules` hook.
- Add `UPROPERTY(EditAnywhere, Category = "AI|MinMax|Evaluation", meta = (ExcludeBaseStruct))
  TArray<TInstancedStruct<FConnectItMinMaxEvalTerm>> EvaluationTerms` and the same for `OrderingTerms`; the
  constructor seeds today's defaults (score difference 1000 + line potential 10; tile multiplier 10 + adjacency 5), so
  play is unchanged.
- `BeginDecision` builds `FConnectItMinMaxRules` from the context + both arrays; warns if `EvaluationTerms` is empty.

### Strategy base — `UConnectIt_AIStrategy`
- Add `BlueprintPure GetOwningController()` (the outer — the controller duplicates the strategy with itself as outer),
  so a strategy can read more than the shared context, on the game thread.

## Hierarchy after the change
`AConnectIt_AIController` → `UConnectIt_AIStrategy` (per level: MinMax or bespoke) →
`UConnectIt_AIStrategy_MinMax` (search settings + evaluation terms + ordering terms, all editor data) → builds an
internal `FConnectItMinMaxRules` per decision. Changing what the AI values = edit or add terms; never a second strategy.

## Tests (`Private/Tests/ConnectIt_ClassicSearchTests.cpp` → `ConnectIt_MinMaxTests.cpp`)
- Port to the new names; build rules with the default term arrays (a small helper).
- Replace `VariantOverridesEvaluate` with `EvaluationTermsDriveChoice`: a test-only term
  (`Private/Tests/ConnectIt_MinMaxTestTerms.h`, USTRUCT marked `Hidden` so it stays out of the picker) that values the
  (0,0) corner; with only that term the search takes the corner, with the defaults it doesn't.
- Test names move to `ConnectIt.AI.MinMax.*`.

## Vault
- Step 0: plan moved here from the plan-mode staging file (done).
- Decision `2026-10-02-minmax-evaluation-as-editor-terms.md`; mark the "variants subclass the rules" part of
  `2026-10-02-pluggable-ai-strategy-and-instance-rules.md` superseded (status line + pointer).
- Update `UnrealGameIntelligence/code/MinMax.md` consumer names; log + index.

## Verification
- Build (editor closed); `Automation RunTests ConnectIt.AI` — all pass, default-term results match the previous
  hand-coded evaluation (same win/block tests); re-check throughput (machine permitting — last run was CPU-throttled).
- Owner, in editor: on the level config's AIStrategy (MinMax), confirm the two term lists show the defaults and can be
  reordered/edited/extended; then the existing PIE checklist (fix `CI_AIController_Play`, set `AIControllerClass`,
  `EnemyLoadout`).

## Status (2026-10-02) — implemented, built, unit-tested

- Built clean; `Automation RunTests ConnectIt.AI` 7/7 pass (tests now `ConnectIt.AI.MinMax.*`, in
  `Private/Tests/ConnectIt_MinMaxTests.cpp`; they build rules from the strategy CDO's own default term lists, so they
  test what ships). `EvaluationTermsDriveChoice` swaps only the evaluation list and gets a different move.
- Throughput: ~378k nodes/s, depth 5 completes inside 1.5 s (CPU reading 42%) — back in line with the first
  measurement; the earlier ~70k reading was taken with the CPU at 31% and the virtual-call rules design, so its cause
  stays unproven.
- Test-only term `FConnectItMinMaxEvalTerm_TestCorner` (`Private/Tests/ConnectIt_MinMaxTestTerms.h`) is marked
  `Hidden` and named "(Test) Corner Owner"; unconfirmed whether the instanced-struct picker honours `Hidden` — if it
  shows up in the editor, ignore it (or ask to move it out of the shipping module).
- Added `UConnectIt_AIStrategy::GetOwningController()`.

## Update (2026-10-07): replaced

The term structs described here no longer exist. See [minmax-named-weights](minmax-named-weights.md).
