---
schema: code
reconciled: 2026-09-10
commit: d0cba36
---

# UnrealAIMechanics — code index

Inventory + module map. Governed by [[_core/_schema/_code|_core/_schema/_code.md]].
Domain overview: [[UnrealAIMechanics/CLAUDE|CLAUDE.md]].

**Dormant** — not enabled in `ConnectIt.uproject`;
`UAI_UtilityGameActionController::SelectEvaluatedAction` is an unimplemented stub, so
`PickNextAction()` picks nothing.

## Inventory

| Type | Kind | Role | Source (`Public/…`) | Status |
|---|---|---|---|---|
| [[UnrealAIMechanics/code/UAI_UtilityGameActionController\|UAI_UtilityGameActionController]] | UCLASS (UObject) | primary | `Utility/AI_UtilityGameActionController.{h,cpp}` | current |
| [[UnrealAIMechanics/code/UAI_UtilityGameActionEvaluator\|UAI_UtilityGameActionEvaluator]] (+ `FGameActionEvaluated`, `UAI_UtilityLibrary`) | UCLASS / USTRUCT | primary | `Utility/AI_UtilityGameActionEvaluator.h`, `Utility/AI_UtilityStructs.h`, `Utility/AI_UtilityLibrary.{h,cpp}` | current |
| FUnrealAIMechanicsModule | class (IModuleInterface) | internal | `UnrealAIMechanics.h` | stub |

## Map

`Plugins/UnrealAIMechanics/Source/UnrealAIMechanics/` — one Runtime module, no deps, one
sub-area `Utility/`. A **Utility-AI** action picker: `UAI_UtilityGameActionController`
holds an `Instanced` array of `UAI_UtilityGameActionEvaluator` scorers; each scores from a
per-`FGameplayTag` weight map; results flatten into `TArray<FGameActionEvaluated>`;
`UAI_UtilityLibrary::SortGameActionEvaluations()` ranks them. The pick step
(`SelectEvaluatedAction`) is unimplemented, so the pipeline is not end-to-end. No
`systems/` or `recipes/` while dormant.

**Start at:**
[[UnrealAIMechanics/code/UAI_UtilityGameActionController|UAI_UtilityGameActionController]].
