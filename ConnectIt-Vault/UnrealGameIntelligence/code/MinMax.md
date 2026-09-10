---
schema: code
kind: class
role: primary
source:
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Public/MinMax/MinMaxAlgorithm.h
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Public/MinMax/MinMaxABPruning.h
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Public/MinMax/MinMaxABMoveOrder.h
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Public/MinMax/MinMaxUtility.h
reconciled: 2026-09-10
commit: 1cb84bd
---

# MinMax toolkit

Header-only, C++20-concept-constrained templates for game-tree search. A progression:
`MinMaxAlgorithm` (plain minimax) → `MinMaxABPruning` (alpha-beta) → `MinMaxABMoveOrder`
(alpha-beta + move ordering). `MinMaxUtility.h` holds the node concepts and threaded
tree-building helpers.

> **Status:** the `ConnectIt` game module does **not** use these — it carries its own
> parallel MinMax implementation. Treat this as a library that exists but is currently
> unwired (a known suite-level tension — converge or delete one copy).

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

- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[UnrealGameIntelligence/CLAUDE|UnrealGameIntelligence overview]] → *MinMax*.
