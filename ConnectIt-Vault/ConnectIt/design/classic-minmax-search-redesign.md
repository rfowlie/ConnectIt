# Classic MinMax: search redesign

Design + plan note (2026-10-02), revising the [09-24 rebuild](classic-minmax-rebuild.md). Approved in plan mode.

## Context
On 09-24 a session rebuilt the Classic MinMax AI (uncommitted, never compiled — DLL is 09-20). Its clean-up is good
(one node model on the real `FConnectItBoardState`, real scoring via static `UConnectIt_LineScoringRule::ApplyLineScoring`,
a request bridge, registration fix), but review on 10-02 found the search itself is wrong and expensive:
- **Off by one:** root `FactionTurn` = AI slot, but `FConnectItMinMaxNode::GenerateChildren` places for `FactionTurn+1`, so
  root children are the *opponent's* moves.
- **Perspective:** `EvaluateNode` scores for `Node.FactionTurn` (alternates by depth) while min/max assumes a fixed side;
  the 0 clamp makes every losing position equal.
- **Cost:** `BuildTreeAsync` stores the whole tree (≈113k board copies at depth 3 on 7x7, ~2.4k tasks), then
  `SolveTreeAsync` walks it — alpha-beta can't save generation.
- **Heuristic:** own-score gain + piece count; ignores open lines, threats, multipliers; deterministic first-of-ties.

Owner decisions (10-02): rethink search, evaluation, integration and plugin boundary; Classic = **2 players, 1 placement
per turn**; **keep the direct Mediator call**; difficulty = **time budget + max depth + imperfection**; new plugin search
is **added alongside** the existing templates; **build on** the 09-24 working tree.

## Design

### Plugin — `UnrealGameIntelligence` (new, alongside `MinMax/`)
`Public/Search/GI_AlphaBetaSearch.h` (header-only):
- Concept `c_search_game<TGame>`: `TGame::FState`, `TGame::FMove`, static `GenerateMoves(const FState&, TArray<FMove>&)`,
  `ApplyMove(const FState&, const FMove&) -> FState`, `IsTerminal(const FState&)`,
  `Evaluate(const FState&, int32 Ply) -> int32` **from the side to move**, `OrderScore(const FState&, const FMove&) -> int32`.
- `TAlphaBetaSearch<TGame>::Run(Root, FParams) -> FResult`: negamax + alpha-beta, children generated per node (no stored
  tree), move ordering by `OrderScore`, iterative deepening 1..MaxDepth stopping on time budget or cancel flag; keeps the
  last *completed* depth. Root moves searched with a full window so every root score is exact (needed for imperfection).
  `FResult`: per-root-move scores, depth reached, nodes visited, `bCancelled`.
- `Public/Search/GI_AsyncSearch.h`: `LaunchSearchAsync<TGame>(Root, Params, TSharedRef<std::atomic<bool>> Cancel,
  TFunction<void(FResult)> OnGameThread)` — one `UE::Tasks::Launch`, result marshalled via `AsyncTask(GameThread)`.
- Existing `MinMax/*` templates untouched (flag the `MinMaxABMoveOrder` `Reserve()`-instead-of-reverse bug in the vault).

### Game — ConnectIt Classic search
Replace `MinMax/ConnectIt_MinMaxTreeBuilder.h/.cpp` (09-24 version) with `MinMax/ConnectIt_ClassicSearch.h/.cpp`:
- `FConnectItClassicSearchGame` traits. `FState { FConnectItBoardState Board; int32 SideToMove; int32 ConnectLength;
  float WinThreshold; TSharedPtr<const FConnectItLineWindows> Windows; }`, `FMove = FGridPosition`.
- `GenerateMoves`: tiles where `IsTileValidForPlacement`. `ApplyMove`: copy, `SetFactionPiece`, `ApplyLineScoring`
  (reused, unchanged), flip side. `IsTerminal`: a score ≥ `WinThreshold`, or no moves.
- **Evaluator** (`FConnectItClassicEvalWeights`, defaults in code): `±(WinValue − Ply)` on a win (prefers faster wins);
  otherwise `WScore·(myScore − oppScore) + WLines·(myPotential − oppPotential)`. Potential sums every precomputed
  window of `ConnectLength` active cells on the 4 scoring axes that holds only one faction's pieces:
  `count² × (sum of window multipliers)`. Windows are built once per decision from geometry (index-based, so no
  linear `IndexOfByKey` lookups in the hot path).
- `OrderScore`: tile multiplier + adjacency to existing pieces.
- Perf note: `ApplyLineScoring` still uses linear lookups on `FConnectItBoardState`; acceptable at 7x7 — measure the
  nodes/sec log before optimising (dense-grid state would be the follow-up).

### Difficulty
`FConnectItAIDifficulty` on `UConnectIt_LevelConfigDataAsset`, replacing `AISearchDepth`/`AIThreadDepth`:
`MaxDepth`, `TimeBudgetSeconds`, `MinThinkSeconds`, `TopMovesConsidered` (1 = always best), `MistakeChance` (0–1).
Pick: with `MistakeChance` choose uniformly among the top `TopMovesConsidered` root moves, else the best; ties random.

### Controller — `AConnectIt_AIController` (direct Mediator call kept)
- Turn hook unchanged: `UConnectIt_AIActionsComponent::OnTurnStarted` → `OnMyTurnStarted()`.
- `BeginDecision`: warn and stop if `NumFactions != 2`; read connect length (existing `GetMinimumConnectLength` with the
  4 fallback) and `GetTargetScore`; record `GameState->GetActiveTurnNumber()`; build root (`SideToMove` = own slot —
  fixes the off-by-one); `LaunchSearchAsync` with a fresh cancel token.
- On result: drop it if cancelled, or turn number / active participant changed; wait out `MinThinkSeconds` (timer);
  pick move; `SubmitBestMove` keeps the 09-24 request bridge (`ProducesRequestType` class lookup, `GetTagForClass`,
  `ProcessBoardRequest`, `CanEndTurn` → `RequestTurnEnd`, re-decide loop capped at `MaxDecisionsPerTurn`).
- Cancel token set in `EndPlay` and on match end. Log per decision: depth reached, nodes, ms, chosen move, score.
- Remove `TreeBuilder`, `HandleTreeBuilt`/`HandleTreeSolved`.

### Kept from 09-24
Static scoring extraction + `GetMinimumConnectLength` (+ `UConnectIt_BoardRules` wrapper, `GameMode::GetBoardRules`),
double-registration fix, `ATurnBasedAIController` `FObjectInitializer` constructor, `UConnectIt_AIActionsComponent`,
deletion of `UConnectIt_MinMaxManager` / `ConcreteMinMaxExample`.

## Steps
0. Plan moved here from the plan-mode staging file (done).
1. Plugin: `GI_AlphaBetaSearch.h`, `GI_AsyncSearch.h`.
2. Game: `ConnectIt_ClassicSearch.h/.cpp`; delete the 09-24 `ConnectIt_MinMaxTreeBuilder.h/.cpp`.
3. Level config difficulty struct; controller rework.
4. Automation tests (`ConnectIt.AI.ClassicSearch`): takes an immediate winning line; blocks an opponent's open
   (ConnectLength−1) line; never returns an occupied/inactive tile; root side = own faction; respects cancel.
5. Build (editor closed): `Build.bat ConnectItEditor Win64 Development -Project=…ConnectIt.uproject`.
6. Vault: decision note `_decisions/2026-10-02-classic-minmax-negamax-search-and-difficulty.md`; log + index; refresh
   `UnrealGameIntelligence/code/MinMax.md` (new search + legacy-template flag). Task rows left to the owner.

## Verification
- Build clean; run `UnrealEditor-Cmd … -ExecCmds="Automation RunTests ConnectIt.AI; Quit" -unattended -nullrhi`.
- Owner, in editor: clear `ConnectIt_AIController_Game.uasset`'s orphaned `MinMaxManager` nodes; set `AIControllerClass`
  on the GameMode BP and `EnemyLoadout` (PlacePiece only) + difficulty on the level config.
- PIE Classic match vs AI: per-decision log shows depth/nodes/ms within budget; AI completes and blocks lines; requests
  accepted by the Mediator gate; turn passes back; `MistakeChance` > 0 visibly varies play.

## Status (2026-10-02) — implemented, built, unit-tested

- Steps 0–5 done; both `UnrealEditor-*` modules built clean (this also compiled the 09-24 changes for the first time).
- `Automation RunTests ConnectIt.AI` — 6/6 pass: TakesImmediateWin, BlocksOpponentWin, OnlyLegalMoves,
  RootMovesAreOwnFaction (09-24 off-by-one regression), RespectsCancel, Throughput.
- Throughput on a 7x7 mid-game board: depth 4 completes inside 1.5 s, depth 5 runs out (~460k nodes/s) — the
  `FConnectItAIDifficulty` defaults (MaxDepth 4, 1.5 s) fit.
- Deviations from the plan: `UConnectIt_LineScoringRule::GetScoringDirections` made public (the evaluator's line windows
  use the same axes as real scoring); a cancelled search returns no root scores and cancel is also checked before each
  depth; tests live in `Source/ConnectIt/Private/Tests/ConnectIt_ClassicSearchTests.cpp`.
- **Not yet verified in PIE** (owner): the headless run reports Blueprint compile errors in
  `Content/_ConnectItNetworked/Framework/Controller/CI_AIController_Play.uasset` (Construct Object from Class of type
  None, `Min Max` Set/Get pins) — it still references the deleted `UConnectIt_MinMaxManager`;
  `ConnectIt_AIController_Game.uasset` likely the same. `AIDifficulty` replaced `AISearchDepth`/`AIThreadDepth`, so
  level configs show defaults.

> **Revised 2026-10-02:** see [AI strategy + flexible MinMax rules](ai-strategy-and-minmax-rules.md) — the search moves to
> `GameIntelligence::Search::MinMax` with instance rules, and difficulty becomes a pluggable `UConnectIt_AIStrategy`.
