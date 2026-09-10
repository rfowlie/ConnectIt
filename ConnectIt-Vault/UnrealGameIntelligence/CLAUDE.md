# CLAUDE.md — UnrealGameIntelligence

A game-agnostic AI / decision-support toolkit, engine-only deps. Three loosely-related
parts:

1. **Influence-map debug visualisation** — `IGI_FloatGridDataProviderInterface`
   (`float GetData(FIntPoint)` data-shape contract), `IGI_InfluenceMapVisualiser`
   (rendering lifecycle), `UGI_InfluenceMapSubsystem` (`UWorldSubsystem` registry /
   orchestrator + debug widget), `UGI_InfluenceMapWidget` (abstract UMG base). The
   consuming project implements the interfaces.
2. **MinMax game-tree solver templates** — three header-only, C++20-concept-constrained
   templates in a progression: `MinMaxAlgorithm` → `MinMaxABPruningAlgorithm` →
   `MinMaxABMoveOrderAlgorithm`, plus `MinMaxUtility.h` (node concepts + threaded
   tree-building over `UE::Tasks`).
3. **Utility-score debug overlay** — `IUtilityScoreHandler`, a four-method display
   interface.

## Domain

- **Source:** `../Plugins/UnrealGameIntelligence/Source/` — one Runtime module. Sub-areas:
  `InfluenceMap/`, `MinMax/`, `Utility/Interface/`. No plugin-level deps.
- **Status:** active — enabled. Convention: folder-scoped `GI_` prefix, `FGI_On*` dynamic
  delegates, C++20 `requires`-concepts (unique to this plugin).
- **Type:** game-agnostic plugin, independently extractable.
- **Consumed by:** [[ConnectIt/CLAUDE|ConnectIt]] — **only** the influence-map /
  utility-score debug interfaces. The MinMax templates are **not used** — the game module
  carries its own parallel MinMax implementation.

## Sections

- **`code/`** — per-type pages, `code/systems/`, `code/recipes/`, `code/index.md`.
  Governed by [[_core/_schema/_code|_core/_schema/_code.md]].
- `logs/` — created on first use ([[_core/_schema/_logs|schema]]).

## Start here

[[UnrealGameIntelligence/code/index|code/index.md]].

## Known rough edges

- **MinMax templates are dead weight** here — duplicated by the game module. Converge or
  delete one copy. Abandoned scaffolding trail: `MinMax::TMinMaxManager<TNode>` (abstract,
  orphaned) → `ConcreteMinMaxExample.h` (fully commented out) → the consumer's own live
  MinMax, which uses none of it.
- `MinMaxABMoveOrderAlgorithm` has no public `Solve()` wrapper (only `SolveInternal`).
- **Phantom API:** `IGI_InfluenceMap` / `UGI_InfluenceMap` / `SetUpdateDelegate` —
  referenced in `GI_FloatGridDataProviderInterface.h`'s example and the
  `InfluenceMap_README` design guide, implemented **nowhere**. Real contract is
  `IGI_FloatGridDataProviderInterface` + `IGI_InfluenceMapVisualiser`.
- ~185 lines of commented-out dead code across `MinMaxAlgorithm.h` / `MinMaxUtility.h`.
