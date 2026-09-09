---
schema: code
reconciled: 2026-09-06
commit: 32e3e7c
---

# UnrealGameIntelligence — code index

Per-type help pages. Governed by [[_schema/code|_schema/code.md]]. An **overlay** on the
authoritative in-repo reference
([`Docs/README.md`](../../../Plugins/UnrealGameIntelligence/Docs/README.md),
[`Docs/Systems.md`](../../../Plugins/UnrealGameIntelligence/Docs/Systems.md), and
[`Source/UnrealGameIntelligence/README/InfluenceMap_README.md`](../../../Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/README/InfluenceMap_README.md)).

Source root: `Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/`
This plugin is two loosely-related systems (influence-map debug visualisation + a
header-only MinMax toolkit) plus a small utility-score interface.
`status`: **current** = written & reconciled · **stub** = no page yet.

| Type | Kind | Area | Role | Source (`Public/…`) | Status |
|---|---|---|---|---|---|
| [[UnrealGameIntelligence/code/UGI_InfluenceMapSubsystem\|UGI_InfluenceMapSubsystem]] | UCLASS (UWorldSubsystem) | InfluenceMap | primary | `InfluenceMap/GI_InfluenceMapSubsystem.{h,cpp}` | current |
| [[UnrealGameIntelligence/code/IGI_InfluenceMapVisualiser\|IGI_InfluenceMapVisualiser]] (+ `IGI_FloatGridDataProviderInterface`, `UGI_InfluenceMapWidget`) | UINTERFACE / UCLASS | InfluenceMap | primary (contracts) | `InfluenceMap/GI_InfluenceMapVisualiserInterface.h`, `.../GI_FloatGridDataProviderInterface.h`, `.../GI_InfluenceMapWidget.h` | current |
| [[UnrealGameIntelligence/code/MinMax\|MinMax]] (`MinMaxAlgorithm`, `MinMaxABPruning`, `MinMaxABMoveOrder`, `MinMaxUtility` concepts + tree builders) | header-only templates | MinMax | primary (unused by the game module today) | `MinMax/*.h` | current |
| [[UnrealGameIntelligence/code/IUtilityScoreHandler\|IUtilityScoreHandler]] | UINTERFACE | Utility | primary | `Utility/Interface/UtilityScoreHandler.h` | current |
| FUnrealGameIntelligenceModule | class (IModuleInterface) | Top-level | internal | `UnrealGameIntelligence.h` | stub |

Phantom type: `IGI_InfluenceMap` / `UGI_InfluenceMap` is referenced in
`GI_FloatGridDataProviderInterface.h`'s example and in `InfluenceMap_README.md` but is
**implemented nowhere** — ignore it when following those examples.

## Related

- Flows: [[UnrealGameIntelligence/systems/index|systems/index.md]]
- Recipes: [[UnrealGameIntelligence/recipes/index|recipes/index.md]]
