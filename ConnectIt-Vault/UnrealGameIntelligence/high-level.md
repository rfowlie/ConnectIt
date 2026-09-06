# UnrealGameIntelligence — high level

## Purpose

A game-agnostic AI / decision-support toolkit that depends only on engine modules. It
bundles two loosely-related systems plus a small debug helper:

1. **Influence-map debug visualisation.** `IGI_FloatGridDataProviderInterface` (a
   `float GetData(FIntPoint)` "data shape" contract), `IGI_InfluenceMapVisualiser`
   (rendering lifecycle), `UGI_InfluenceMapSubsystem` (a `UWorldSubsystem` registry /
   orchestrator that manages visualiser selection and a debug widget), and
   `UGI_InfluenceMapWidget` (abstract UMG base). The consuming project implements the
   interfaces.
2. **MinMax / game-tree solver templates.** Three header-only, C++20-concept-constrained
   templates forming a progression: `MinMaxAlgorithm` (plain minimax) ->
   `MinMaxABPruningAlgorithm` (alpha-beta) -> `MinMaxABMoveOrderAlgorithm` (alpha-beta +
   move ordering), plus `MinMaxUtility.h` with node concepts and threaded tree-building
   helpers over `UE::Tasks`.
3. **Utility-score debug overlay.** `IUtilityScoreHandler`, a four-method interface for
   on-screen utility-score display.

## Status

Active — enabled in `ConnectIt.uproject`. **Note:** the MinMax templates are currently
*not* used by the game module, which has its own parallel implementations. No plugin-level
dependencies.

## Key modules

- `UnrealGameIntelligence` (Runtime). Sub-areas: `InfluenceMap/`, `MinMax/`,
  `Utility/Interface/`.

## Public API surface

- **Influence map:** `IGI_FloatGridDataProviderInterface`, `IGI_InfluenceMapVisualiser`,
  `UGI_InfluenceMapSubsystem` (`RegisterVisualiser` / `SelectVisualiser` / `CreateDebugUI`,
  `OnVisualiserSelected`), `UGI_InfluenceMapWidget`.
- **MinMax:** `MinMaxAlgorithm::Solve`, `MinMaxABPruningAlgorithm::Solve`,
  `MinMaxABMoveOrderAlgorithm::SolveInternal` (no public `Solve()` wrapper),
  `MinMax::TMinMaxManager<TNode>` (abstract, orphaned),
  `BuildNodeRecursiveWithThreadDepth`.
- **Utility:** `IUtilityScoreHandler`.

## Consumers

`ConnectIt` game module — currently only the influence-map / utility-score debug
interfaces. The MinMax templates are unused (parallel implementations live in the game
module).

## In-repo reference

- [`../../Plugins/UnrealGameIntelligence/Docs/README.md`](../../Plugins/UnrealGameIntelligence/Docs/README.md)
- [`../../Plugins/UnrealGameIntelligence/Docs/Systems.md`](../../Plugins/UnrealGameIntelligence/Docs/Systems.md)
- [`../../Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/README/InfluenceMap_README.md`](../../Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/README/InfluenceMap_README.md) — composite-influence-map design guide

## Known rough edges

- MinMax templates duplicated by the game module rather than reused — decide whether to
  converge on the plugin or delete the plugin copy.
- Phantom `IGI_InfluenceMap` / `UGI_InfluenceMap` interface referenced in
  `GI_FloatGridDataProviderInterface.h`'s example and in `InfluenceMap_README.md` but
  implemented nowhere.
- ~185 lines of commented-out dead code across `MinMaxAlgorithm.h` and `MinMaxUtility.h`.

_Verify against the in-repo `Docs/` before acting on this list — it reflects a past snapshot._
