# UnrealGameIntelligence

## Purpose

UnrealGameIntelligence is a generic AI/decision-support toolkit, kept separate from any consuming game module and from the other first-party plugins so that reusable, game-agnostic AI infrastructure doesn't get tangled up with any one project's gameplay logic. It bundles two loosely related things under one roof: a template-based, C++20-concept-constrained MinMax/alpha-beta game-tree solver toolkit, and an "Influence Map" debug-visualization subsystem (a registry of pluggable visualisers plus a debug UMG widget), along with small utility-score interfaces used for on-screen AI debugging. Because it depends only on engine modules (no other first-party plugin), it can be reused by future projects or lifted out of its current consumer without dragging gameplay-specific code with it.

## Module(s)

| Module | Type | LoadingPhase | Notes |
|---|---|---|---|
| `UnrealGameIntelligence` | Runtime | Default | Single module; entry point `FUnrealGameIntelligenceModule` (`Public/UnrealGameIntelligence.h`). |

## Key Dependencies

- Depends on: `Core`, `GameplayTags`, `UMG`, `SlateCore` (Public); `CoreUObject`, `Engine`, `Slate`, `SlateCore` (Private). No plugin-level dependencies declared in the `.uplugin`.
- Depended on by: the consuming game module (not named here, to keep this plugin's documentation project-agnostic) — specifically, that project's own influence-map data-source and visualiser classes implement this plugin's interfaces (`IGI_FloatGridDataProviderInterface` and `IGI_InfluenceMapVisualiser` respectively).

## When to Use It

- Visualizing any per-grid-cell scalar value (heuristic score, threat/control map, pathing cost) through a common debug UI, without coupling the visualizer to a concrete map/board type.
- Implementing or extending AI decision search that needs a classic minimax / alpha-beta tree solver over a game-tree node type.
- Adding a new on-screen "utility score" debug overlay for an AI system.

## When NOT to Use It / Scope Boundaries

- Project-specific board/piece AI logic (e.g. concrete influence map data sources, concrete visualisers) belongs in the consuming game module, not here — this plugin only supplies the interfaces and orchestration, not board-specific implementations.
- General utility-AI action scoring/selection (evaluator objects picking a best-scored action from weighted tags) belongs in `UnrealAIMechanics`, not here — that is a distinct decision-making pattern from the tree-search and influence-map tooling in this plugin.
- Turn sequencing, event gating, and other non-AI gameplay scaffolding belong in `UnrealGameMechanics`.
- New AI heuristics or search algorithms should extend the existing MinMax family here rather than duplicating a parallel tree-search implementation elsewhere — see Known Discrepancies below regarding a currently-separate, project-specific MinMax implementation found in the current consumer.

## Notable Design Patterns

- Data-provider interface decoupling: `IGI_FloatGridDataProviderInterface` defines only a "data shape" contract (`float GetData(FIntPoint)`), so `UGI_InfluenceMapSubsystem` and its visualisers never need to know the concrete map type they're reading from. This is a reusable pattern worth following for future debug-visualization work.
- Registry/orchestrator pattern: `UGI_InfluenceMapSubsystem` is a `UWorldSubsystem` acting as a central registry for pluggable visualisers, managing selection and debug-widget lifecycle rather than any visualiser doing so itself.
- Template/C++20-concept-constrained algorithm family: three MinMax solvers (plain, alpha-beta pruning, alpha-beta with move ordering) exist side by side as header-only templates, constrained via `c_min_max_node*` concepts. This looks like an intentional progressive set of reference implementations (baseline → pruning → move-ordering) rather than accidental duplication, but is worth confirming with the plugin author.

## Classes

### InfluenceMap

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `IGI_FloatGridDataProviderInterface` | Native Interface | Exposes a float score per `FIntPoint` grid position, decoupling visualisers from concrete map types. | `Public/InfluenceMap/GI_FloatGridDataProviderInterface.h`. Well documented, includes an in-header usage example. Key API: `virtual float GetData(FIntPoint) const = 0`. Implemented by an influence-map provider class in the current consuming project. |
| `UGI_InfluenceMapSubsystem` | `UWorldSubsystem` | Central registry/orchestrator for influence-map visualisers; manages selection and debug-widget lifecycle. | `Public/InfluenceMap/GI_InfluenceMapSubsystem.h`. Key API: `RegisterVisualiser`, `UnregisterVisualiser`, `SelectVisualiser`, `SelectVisualiserByIndex`, `ClearSelection`, `CreateDebugUI`/`ShowDebugUI`/`HideDebugUI`/`DestroyDebugUI`, `OnVisualiserSelected` delegate. |
| `IGI_InfluenceMapVisualiser` | Native Interface | Contract for a visualiser: identity (tag, display name) plus activation lifecycle. | `Public/InfluenceMap/GI_InfluenceMapVisualiserInterface.h`. Key API: `GetTag`, `GetDisplayName`, `Activate`/`Deactivate`/`IsActive`. Implemented by a visualiser class in the current consuming project. |
| `UGI_InfluenceMapWidget` | `UUserWidget` (Abstract) | Base debug UMG widget for selecting/displaying a visualiser. | `Public/InfluenceMap/GI_InfluenceMapWidget.h`. Subclass implements `SetupButtons`; broadcasts `OnSelectionChanged` back to the subsystem. |

### MinMax

All header-only templates, C++20 `requires`-concept constrained.

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `MinMaxABMoveOrderAlgorithm` | Template (none) | Alpha-beta pruning with move ordering: sorts children by `GetOrderingScore()` before recursing, for more cutoffs. | `Public/MinMax/MinMaxABMoveOrder.h`. Key API: `Solve`, `GetOrderedChildren`. |
| `MinMaxABPruningAlgorithm` | Template (none) | Plain alpha-beta pruning solver, no move ordering. | `Public/MinMax/MinMaxABPruning.h`. Key API: `Solve`. |
| `MinMaxAlgorithm` | Template (none) | Plain minimax, no pruning — baseline/reference implementation. | `Public/MinMax/MinMaxAlgorithm.h`. Key API: `Solve`. Contains ~50 lines of a commented-out `TMinMaxAlgorithm` struct kept as historical reference. See Known Discrepancies. |
| `TMinMaxManager<TNode>` (+ `c_min_max_node*` concepts) | Abstract template class | Defines the node concepts the algorithms above require, and tree-building helpers including a threaded variant using `UE::Tasks`. | `Public/MinMax/MinMaxUtility.h`. Has no concrete subclass anywhere in the plugin — orphaned/incomplete scaffolding. Also contains ~135 lines of a commented-out `TMinMaxManagerBase` class. See Known Discrepancies. |

### Utility

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `IUtilityScoreHandler` | Native Interface | Get/Set/Show/Hide a float "utility score", for on-screen AI debug overlays. | `Public/Utility/Interface/UtilityScoreHandler.h`. |

## Known Discrepancies / Issues in This Plugin

- The current consuming project has its own, separate min-max implementations that do not use this plugin's templates — this plugin's MinMax toolkit and that consumer's actual AI tree-search are currently unrelated/parallel implementations. See the project-level [duplication notes](../../../Source/ConnectIt/Docs/Duplication.md).
- `MinMaxAlgorithm` (`MinMaxAlgorithm.h`) contains ~50 lines of commented-out dead code (`TMinMaxAlgorithm`). See the project-level [duplication notes](../../../Source/ConnectIt/Docs/Duplication.md).
- `TMinMaxManager<TNode>` (`MinMaxUtility.h`) is orphaned scaffolding with no concrete subclass, and also contains ~135 lines of commented-out dead code (`TMinMaxManagerBase`). See the project-level [duplication notes](../../../Source/ConnectIt/Docs/Duplication.md).
