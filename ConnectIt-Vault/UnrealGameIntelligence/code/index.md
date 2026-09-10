---
schema: code
reconciled: 2026-09-10
commit: 94b135f
---

# UnrealGameIntelligence — code index

Inventory + module map. Governed by [[_core/_schema/_code|_core/_schema/_code.md]].
Domain overview: [[UnrealGameIntelligence/CLAUDE|CLAUDE.md]].

`status`: **current** = written & reconciled · **stub** = no page yet.

## Inventory

| Type | Kind | Area | Role | Source (`Public/…`) | Status |
|---|---|---|---|---|---|
| [[UnrealGameIntelligence/code/UGI_InfluenceMapSubsystem\|UGI_InfluenceMapSubsystem]] | UCLASS (UWorldSubsystem) | InfluenceMap | primary | `InfluenceMap/GI_InfluenceMapSubsystem.{h,cpp}` | current |
| [[UnrealGameIntelligence/code/IGI_InfluenceMapVisualiser\|IGI_InfluenceMapVisualiser]] (+ `IGI_FloatGridDataProviderInterface`, `UGI_InfluenceMapWidget`) | UINTERFACE / UCLASS | InfluenceMap | primary (contracts) | `InfluenceMap/GI_InfluenceMapVisualiserInterface.h`, `.../GI_FloatGridDataProviderInterface.h`, `.../GI_InfluenceMapWidget.h` | current |
| [[UnrealGameIntelligence/code/MinMax\|MinMax]] (`MinMaxAlgorithm`, `MinMaxABPruning`, `MinMaxABMoveOrder`, `MinMaxUtility`, `TMinMaxManager`, `ConcreteMinMaxExample`) | header-only templates | MinMax | primary — **unused by the game module** | `MinMax/*.h` | current |
| [[UnrealGameIntelligence/code/IUtilityScoreHandler\|IUtilityScoreHandler]] | UINTERFACE | Utility | primary | `Utility/Interface/UtilityScoreHandler.h` | current |
| FUnrealGameIntelligenceModule | class (IModuleInterface) | Top-level | internal | `UnrealGameIntelligence.h` | stub |

**Phantom type** — `IGI_InfluenceMap` / `UGI_InfluenceMap` / `SetUpdateDelegate`:
referenced in `GI_FloatGridDataProviderInterface.h`'s example and in the
composite-influence-map recipe, **implemented nowhere**. The real contract is
`IGI_FloatGridDataProviderInterface` + `IGI_InfluenceMapVisualiser`.

## Map

`Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/` — one Runtime module, no
plugin deps. Three loosely-related parts:

- **`InfluenceMap/`** — a debug-visualisation framework. The project implements
  `IGI_FloatGridDataProviderInterface` (`float GetData(FIntPoint)`) on a map and
  `IGI_InfluenceMapVisualiser` on a renderer;
  [[UnrealGameIntelligence/code/UGI_InfluenceMapSubsystem|UGI_InfluenceMapSubsystem]]
  (`UWorldSubsystem`) registers visualisers, drives selection, and owns a debug widget.
  See
  [[UnrealGameIntelligence/code/systems/influence-map-visualisation|systems/influence-map-visualisation]].
- **`MinMax/`** — header-only C++20-concept-constrained game-tree templates, a progression
  `MinMaxAlgorithm` → `MinMaxABPruning` → `MinMaxABMoveOrder`, + `MinMaxUtility.h` (node
  concepts, threaded tree building). **Dead weight** — ConnectIt has its own parallel
  MinMax; the abandoned trail is `TMinMaxManager` (orphaned) → `ConcreteMinMaxExample.h`
  (commented out) → nothing.
- **`Utility/Interface/`** — `IUtilityScoreHandler`, a four-method on-screen score
  display contract.

**Start at:** `UGI_InfluenceMapSubsystem` → `systems/influence-map-visualisation` →
`recipes/add-an-influence-map-visualiser`.

## Related

- Flows: [[UnrealGameIntelligence/code/systems/index|code/systems/index.md]]
- Recipes: [[UnrealGameIntelligence/code/recipes/index|code/recipes/index.md]]
