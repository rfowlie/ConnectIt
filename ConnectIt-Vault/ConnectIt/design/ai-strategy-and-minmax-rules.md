# AI strategy + flexible MinMax rules

Design + plan note (2026-10-02), revision 2 of the [Classic MinMax search redesign](classic-minmax-search-redesign.md). Approved in plan mode.

## Context
Owner review (10-02) of the just-built Classic MinMax redesign raised six points. Outcomes:
1. `TGame` name — **kept**.
2. Score window / PVS / aspiration — **deferred to a task** (owner wants to learn more first).
3. `FConnectItBoardState` (a USTRUCT) in the background search — **safe** (plain data, own copy, made on the game
   thread); add a guard comment so nobody adds UObject references to it.
4. Future-proof naming — namespaces `GameIntelligence::Search` (shared) / `GameIntelligence::Search::MinMax`
   (alpha-beta), and drop the redundant "Search" from names inside the namespace.
5. Flexibility — **instance rules + inheritance**: the search takes a const rules object; variants subclass it;
   evaluation weights become editor data.
6. Pluggable AI — **`UConnectIt_AIStrategy`** (abstract, Instanced on the level config) with
   **`UConnectIt_AIStrategy_MinMax`**; the controller becomes strategy-agnostic. Replaces `FConnectItAIDifficulty`.

Keep the owner's own edits (`FSearchParams::MaxDepth = 1` default, `++Context.Nodes`, whitespace, comment wording).

## Plugin — UnrealGameIntelligence
- `Public/Search/GI_AlphaBetaSearch.h` → `Public/Search/MinMax/GI_MinMaxAlphaBeta.h`, namespace
  `GameIntelligence::Search::MinMax`: `c_search_game` → `c_game` (still over `TGame`), `FSearchParams` → `FParams`,
  `TRootMoveScore` → `TScoredMove`, `TSearchResult` → `TResult`, `TAlphaBetaSearch` → `TAlphaBeta`, `ScoreInfinity` stays.
- **Instance rules:** the concept requires the functions as `const` members on a `const TGame&`;
  `TAlphaBeta<TGame>::Run(const TGame& Game, const FState& Root, const FParams&)`; `Game` threaded through `Negamax`
  and `OrderMoves`. Doc: the instance is shared read-only by the search and must not change while it runs.
- `Public/Search/GI_AsyncSearch.h` → `Public/Search/GI_SearchAsync.h`, namespace `GameIntelligence::Search`:
  `FCancelFlag`/`MakeCancelFlag()`, plus a searcher-agnostic `LaunchAsync<TResult>(TUniqueFunction<TResult()> Work,
  TUniqueFunction<void(TResult&&)> OnGameThread)`. `MinMax::LaunchAlphaBetaAsync<TGame>(TSharedRef<const TGame,
  ThreadSafe> Game, FState Root, FParams, FCancelFlag, OnComplete)` built on it (the task holds the shared ref).

## Game — Classic rules as an instance
`MinMax/ConnectIt_ClassicSearch.h/.cpp` — `FConnectItClassicSearchGame` becomes a class (name kept):
- Constructed on the game thread: `(const FConnectItBoardState& Board, int32 ConnectLength, float WinThreshold,
  const FConnectItMinMaxEvalWeights& Weights)`; builds geometry once as a member (no more shared pointer per node).
- `FState` shrinks to `{ FConnectItBoardState Board; int32 SideToMove; }` — per-search constants (connect length, win
  threshold, geometry, weights) live on the rules instance, so less is copied per node. `MakeRoot(Board, Side)`.
- Concept functions + `LinePotential` are `virtual ... const`; a variant subclasses and overrides only what differs
  (internal calls such as Evaluate→LinePotential dispatch to the override). The search is instantiated once on the base
  type; the strategy holds the instance as `TSharedRef<const FConnectItClassicSearchGame>`.
- `FConnectItMinMaxEvalWeights` (USTRUCT, editor-exposed): `ScoreWeight` 1000, `LinePotentialWeight` 10,
  `OrderMultiplierWeight` 10, `OrderAdjacencyWeight` 5 (today's hardcoded values).
- `ConnectIt_Structs.h`: comment on `FConnectItBoardState` — copied into background AI searches; keep it free of UObject
  references.

## Game — pluggable AI strategy
New `Source/ConnectIt/Public/AI/ConnectIt_AIStrategy.h/.cpp`, `ConnectIt_AIStrategy_MinMax.h/.cpp` (+ Private):
- `FConnectItAIDecisionContext` (USTRUCT): `Board`, `OwnSlot`, `ConnectLength`, `WinScoreThreshold`.
- `FConnectItAIDecision` (USTRUCT): `RequestType` (gameplay tag), `Payload` (`FInstancedStruct`), `Summary` (log text).
  Empty `RequestType` = no move.
- `UConnectIt_AIStrategy` — `UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)`, mirroring the rule
  classes: `MinThinkSeconds`; `BlueprintNativeEvent BeginDecision(Context)` / `CancelDecision()`;
  `BlueprintCallable FinishDecision(Decision)` → native `OnDecisionFinished` delegate, ignored unless a decision is
  active (so late results after a cancel are dropped). Works for C++ and Blueprint strategies.
- `UConnectIt_AIStrategy_MinMax` — `MaxDepth` (4), `TimeBudgetSeconds` (1.5), `TopMovesConsidered` (1),
  `MistakeChance` (0), `EvalWeights`. `BeginDecision`: 2-faction check, builds the rules instance + root, launches the
  search with its own cancel flag; on completion (game thread) picks the move (current `PickMoveIndex` logic moves here)
  and finishes with a `ConnectIt_Game_PlacePiece` decision. `CancelDecision` sets the flag.
- Level config: `FConnectItAIDifficulty AIDifficulty` → `UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced)
  TObjectPtr<UConnectIt_AIStrategy> AIStrategy`; delete `FConnectItAIDifficulty`.

## Controller — strategy-agnostic
`AConnectIt_AIController`:
- `InitialiseFromLevelConfig`: `Strategy = DuplicateObject(LevelConfig->AIStrategy, this)` (the asset's instance is a
  shared template — same reason the registries are duplicated); unset → warn and create a default
  `UConnectIt_AIStrategy_MinMax`. Bind `OnDecisionFinished`.
- `BeginDecision`: read board/slot/connect length/win score (game thread, as now), record turn number + start time,
  `++CurrentDecisionId`, `Strategy->BeginDecision(Context)`. `CancelSearch` → `CancelDecision` (strategy cancel + timer
  clear + id bump); still called from `EndPlay`.
- `HandleDecisionFinished`: stale guard (`IsDecisionStillCurrent`), think timer from `Strategy->MinThinkSeconds`, then
  `SubmitDecision`: the place-piece class lookup generalised to "the loadout action whose `ProducesRequestType`
  matches `Decision.RequestType`", request built from the decision's type + payload, then the existing
  `ProcessBoardRequest` / `CanEndTurn` / re-decide loop.
- No search types in the controller any more.

## Tests
`Private/Tests/ConnectIt_ClassicSearchTests.cpp` — port the 6 tests to the instance API (+ new names), and add
`VariantOverridesEvaluate`: a test-only subclass overriding `Evaluate` changes the chosen move (proves the inheritance
path end to end).

## Vault
- Step 0: plan moved here from the plan-mode staging file (done).
- Task row (owner asked): "Learn about + decide on score windows for the MinMax search (PVS / aspiration / root
  window)" — Design priority; notes: what each gives, the exact-vs-bound root-score catch for the mistake logic.
- Decision note `2026-10-02-pluggable-ai-strategy-and-instance-rules.md` (refines the 09-24 "bespoke per-level AI
  controllers" call into "bespoke per-level strategies on one controller; controller subclass still possible").
- Refresh `UnrealGameIntelligence/code/MinMax.md`; log + index.

## Verification
- Build (editor closed) via `Build.bat ConnectItEditor Win64 Development`.
- `UnrealEditor-Cmd … -ExecCmds="Automation RunTests ConnectIt.AI; Quit"` — all pass; throughput still ≈ depth 4 in 1.5 s.
- Owner, in editor: set `AIStrategy` (MinMax) on the level config; fix the broken MinMax nodes in
  `CI_AIController_Play` / `ConnectIt_AIController_Game`; set `AIControllerClass` + `EnemyLoadout`; PIE vs the AI.

## Status (2026-10-02) — implemented, built, unit-tested

- Built clean; `Automation RunTests ConnectIt.AI` 7/7 pass (the 6 ported tests + `VariantOverridesEvaluate`, which
  runs a subclass through the search compiled for the base type and checks its `Evaluate` override wins).
- Deviations: the MinMax async launcher lives in its own `Search/MinMax/GI_MinMaxAsync.h`; the strategy has
  non-virtual `StartDecision` / `StopDecision` entry points around the `BeginDecision` / `CancelDecision` hooks (keeps
  the "is a decision active" bookkeeping out of subclasses); `WinValue` lost its digit separators (UHT now parses the
  header because of the weights USTRUCT).
- **Throughput — needs a re-check:** the throughput test now reports ~70k nodes/s (depth 4 in 1.5 s, not 5), vs
  ~460k earlier today. The CPU was reading 31% processor performance (throttled) during these runs and the build took
  2.3x longer than the earlier one, so the machine is the likely cause — re-run
  `ConnectIt.AI.ClassicSearch.Throughput` on a cool machine. If it stays slow, suspect the new virtual calls and
  mark the rules class `final`-style / move variants to a template parameter.
- Owner, in editor: set `AIStrategy` → "MinMax (Classic)" on the level config(s); `CI_AIController_Play.uasset`
  still fails to compile (deleted MinMax manager nodes); then PIE vs the AI.

> **Revised again 2026-10-02:** variants no longer subclass the rules -- evaluation and ordering became editor-composed
> terms on the MinMax strategy. See [MinMax evaluation as editor-composed terms](minmax-evaluation-terms.md).
