---
schema: code
reconciled: 2026-09-06
commit: d0cba36
---

# UnrealAIMechanics — code index

Per-type help pages. Governed by [[_schema/code|_schema/code.md]]. Overlay on
[`Docs/README.md`](../../../Plugins/UnrealAIMechanics/Docs/README.md) /
[`Docs/Systems.md`](../../../Plugins/UnrealAIMechanics/Docs/Systems.md).

Source root: `Plugins/UnrealAIMechanics/Source/UnrealAIMechanics/`.
**The plugin is dormant** — not enabled in `ConnectIt.uproject`, and
`UAI_UtilityGameActionController::SelectEvaluatedAction` is an unimplemented stub, so
`PickNextAction()` does not yet pick anything.

| Type | Kind | Role | Source (`Public/…`) | Status |
|---|---|---|---|---|
| [[UnrealAIMechanics/code/UAI_UtilityGameActionController\|UAI_UtilityGameActionController]] | UCLASS (UObject) | primary | `Utility/AI_UtilityGameActionController.{h,cpp}` | current |
| [[UnrealAIMechanics/code/UAI_UtilityGameActionEvaluator\|UAI_UtilityGameActionEvaluator]] (+ `FGameActionEvaluated`, `UAI_UtilityLibrary`) | UCLASS / USTRUCT | primary | `Utility/AI_UtilityGameActionEvaluator.h`, `Utility/AI_UtilityStructs.h`, `Utility/AI_UtilityLibrary.{h,cpp}` | current |
| FUnrealAIMechanicsModule | class (IModuleInterface) | internal | `UnrealAIMechanics.h` | stub |

No `systems/` or `recipes/` while the plugin is a stub.
