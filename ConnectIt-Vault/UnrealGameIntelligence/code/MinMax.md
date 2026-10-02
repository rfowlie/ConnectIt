---
schema: code
kind: class
role: primary
source:
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Public/MinMax/MinMaxAlgorithm.h
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Public/MinMax/MinMaxABPruning.h
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Public/MinMax/MinMaxABMoveOrder.h
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Public/MinMax/MinMaxUtility.h
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Public/Search/GI_SearchAsync.h
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Public/Search/MinMax/GI_MinMaxAlphaBeta.h
reconciled: 2026-10-02
commit: 1cb84bd
---

# MinMax toolkit

Header-only, C++20-concept-constrained templates for game-tree search. A progression:
`MinMaxAlgorithm` (plain minimax) → `MinMaxABPruning` (alpha-beta) → `MinMaxABMoveOrder`
(alpha-beta + move ordering). `MinMaxUtility.h` holds the node concepts and threaded
tree-building helpers.

> **Status (2026-10-02):** the game uses the newer **`Search/`** headers, not these.
> - `Search/MinMax/GI_MinMaxAlphaBeta.h` — `GameIntelligence::Search::MinMax::TAlphaBeta<TGame>`: negamax + alpha-beta
>   over a **const rules instance** (`c_game`: `TGame::FState`, `TGame::FMove`, const members `GenerateMoves` /
>   `ApplyMove` / `IsTerminal` / `Evaluate` (side to move) / `OrderScore`), children generated on demand, move ordering,
>   iterative deepening, time budget, cancel flag; root moves get exact (full-window) scores. `Run(Game, Root, FParams)`
>   → `TResult` (`TScoredMove` list best-first, depth, nodes, time).
> - `Search/GI_SearchAsync.h` — `GameIntelligence::Search`: `FCancelFlag` / `MakeCancelFlag`, searcher-agnostic
>   `LaunchAsync(Work, OnGameThread)`.
> - `MinMax::LaunchAlphaBetaAsync<TGame>(SharedRules, Root, Params, Cancel, OnComplete)` — in `GI_MinMaxAlphaBeta.h`
>   (merged in by the owner; that header now includes the async plumbing).
> - Consumer: ConnectIt's `FConnectItMinMaxRules` (built per decision by `UConnectIt_AIStrategy_MinMax` from its
>   editor term lists) — see [the decision](../../ConnectIt/_decisions/2026-10-02-minmax-evaluation-as-editor-terms.md).
> - Score windows (root window / PVS / aspiration) are an open task, not built.
>
> The `MinMax/` templates below are **kept for reference, unused** (owner's call). They build and store a whole tree
> before solving it. Known bug: `MinMaxABMoveOrder::GetOrderedChildren` calls `Ordered.Reserve()` where it means to
> reverse the order for the minimiser (never instantiated, so it has never failed to compile).

## When you touch this

- If you decide to converge the game module's AI onto the plugin templates.
- Adding a fourth refinement in the progression.

## Entry points

- **Concepts (`MinMaxUtility.h`):**
  - `c_min_max_node` — needs `GetChildren() -> TArray<TNode>`.
  - `c_min_max_move_order` — also `GetOrderingScore() -> int32`.
  - `c_min_max_tree_builder` — `GetChildren() const&`, `SetChildren(TArray&&)`,
    `GenerateChildren()`.
- **Tree building:** `BuildNodeRecursive(Node, CurrentDepth, MaxDepth, IsGameOver)`
  (single-threaded); `BuildNodeRecursiveWithThreadDepth(...)` (parallel down to a depth,
  over `UE::Tasks`).
- **Solvers:** `MinMaxAlgorithm::Solve` and `MinMaxABPruning::Solve`;
  `MinMaxABMoveOrder` exposes `SolveInternal` (**no public `Solve()` wrapper** — a rough
  edge).
- `MinMax::TMinMaxManager<TNode>` — abstract, orphaned.

## Collaborators

None inside the plugin — templates parameterised on a caller's `TNode`. No UObject, no
reflection.

## Gotchas

- **Not `BlueprintCallable` anything** — pure C++ templates.
- `MinMaxABMoveOrder` has no `Solve()` — callers must reach `SolveInternal` directly.
- `MinMaxUtility.h` / `MinMaxAlgorithm.h` carry ~185 lines of commented-out dead code
  (per the plugin's known rough edges).
- `TMinMaxManager` is abstract and unreferenced — don't build on it expecting support.

## Cross-impact

Isolated. Converging the game module onto these means providing a `TNode` that satisfies
`c_min_max_tree_builder` and deleting the game's own implementation.

## Changes

- 2026-10-02 (latest) — owner merged `LaunchAlphaBetaAsync` into `GI_MinMaxAlphaBeta.h` (`GI_MinMaxAsync.h` removed);
  ConnectIt consumer renamed to `FConnectItMinMaxRules`.
- 2026-10-02 (later) — search moved to `GameIntelligence::Search::MinMax` (`GI_MinMaxAlphaBeta.h`), takes a const
  rules instance; async split into searcher-agnostic `GI_SearchAsync.h` + `GI_MinMaxAsync.h`; old `Search/` file names removed.
- 2026-10-02 — added `Search/GI_AlphaBetaSearch.h` + `Search/GI_AsyncSearch.h` (the search ConnectIt actually
  uses); `MinMax/` templates left as unused reference.
- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[UnrealGameIntelligence/CLAUDE|UnrealGameIntelligence overview]] → *MinMax*.
