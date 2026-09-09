---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealAIMechanics/Source/UnrealAIMechanics/Public/Utility/AI_UtilityGameActionController.h
  - Plugins/UnrealAIMechanics/Source/UnrealAIMechanics/Private/Utility/AI_UtilityGameActionController.cpp
reconciled: 2026-09-06
commit: becf253
---

# UAI_UtilityGameActionController

`UObject`. The top of a **Utility-AI** action picker: holds an `Instanced` array of
[[UnrealAIMechanics/code/UAI_UtilityGameActionEvaluator|UAI_UtilityGameActionEvaluator]]
objects, gathers their scored candidate actions, and (is meant to) pick the best one.

> **Dormant.** `SelectEvaluatedAction` is an unimplemented stub — it returns a default
> `FGameActionEvaluated`, so `PickNextAction()` does not actually pick anything yet. The
> plugin is not enabled in `ConnectIt.uproject`.

## When you touch this

- If/when this plugin is revived: implementing selection, or wiring an AI controller to
  call `PickNextAction`.

## Entry points

- `PickNextAction() → FGameActionEvaluated` — the intended one-call API (currently
  non-functional).
- `GetEvaluatedActions(TArray<FGameActionEvaluated>& Out)` — flattens every evaluator's
  results.
- `SelectEvaluatedAction(const TArray<FGameActionEvaluated>&)` — **stub**; the header even
  asks "how do we make it virtual for C++ and Blueprint again?".
- `GameActionHandlers` (`EditAnywhere, Instanced` array of evaluators).

## Collaborators

- Owns `UAI_UtilityGameActionEvaluator` instances; consumes their
  `FGameActionEvaluated` output.
- `UAI_UtilityLibrary::SortGameActionEvaluations` ranks the flattened array.

## Gotchas

- **Not usable end to end** — see the dormancy note. Don't build on `PickNextAction`
  until `SelectEvaluatedAction` is implemented.
- Distinct from `UnrealGameIntelligence`'s MinMax tree search — this is a
  weighted-scoring action picker.

## Cross-impact

If revived: enable in `.uproject`; the evaluator contract and `FGameActionEvaluated`
shape are the extension surface.

## See also

- In-repo: `Docs/README.md`, `Docs/Systems.md`.
