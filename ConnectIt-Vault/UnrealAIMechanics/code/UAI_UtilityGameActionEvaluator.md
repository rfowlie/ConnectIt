---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealAIMechanics/Source/UnrealAIMechanics/Public/Utility/AI_UtilityGameActionEvaluator.h
  - Plugins/UnrealAIMechanics/Source/UnrealAIMechanics/Public/Utility/AI_UtilityStructs.h
  - Plugins/UnrealAIMechanics/Source/UnrealAIMechanics/Public/Utility/AI_UtilityLibrary.h
reconciled: 2026-09-10
commit: d0cba36
---

# UAI_UtilityGameActionEvaluator (+ `FGameActionEvaluated`, `UAI_UtilityLibrary`)

`UObject`. One pluggable scorer: given a per-`FGameplayTag` weight map, it produces scored
candidate actions for
[[UnrealAIMechanics/code/UAI_UtilityGameActionController|UAI_UtilityGameActionController]].

> Part of the **dormant** Utility-AI plugin — see the controller page.

## Entry points

### `UAI_UtilityGameActionEvaluator` (subclass in C++ or Blueprint)

- `GetEvaluatedActions(TArray<FGameActionEvaluated>& Out)` — `BlueprintNativeEvent`;
  scoring logic lives here in a subclass.
- `SetEvaluationWeights(const TMap<FGameplayTag, float>&)` — `BlueprintNativeEvent`.
- `EvaluationWeights` (`TMap<FGameplayTag, float>`, `EditAnywhere`).

### `FGameActionEvaluated`

- `GameplayActionTag`, `GameplayActionContext` (`UObject*`), `Score`, and `Scoring`
  (`TMap<FGameplayTag, float>` — per-source breakdown, "to help with debugging").
- `UpdateScore()` — sums `Scoring` values into `Score`.
- Comment marks it as a payload for activating GAS events (tag + context object).

### `UAI_UtilityLibrary`

- `static SortGameActionEvaluations(const TArray<FGameActionEvaluated>&, bool
  SortAscending = false)` — ranks an evaluated array.

## Gotchas

- **`GetEvaluatedActions` is a `BlueprintNativeEvent`** — the base does nothing; a
  subclass must supply scoring.
- `FGameActionEvaluated::Score` is only meaningful after `UpdateScore()` (or if a
  subclass sets it directly).
- The whole pipeline is dormant — the controller can't select yet.

## Cross-impact

`FGameActionEvaluated`'s shape is the contract between evaluators, the library sort, and
the controller's (unimplemented) selection.

## Changes

- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[UnrealAIMechanics/CLAUDE|UnrealAIMechanics overview]].
- [[UnrealAIMechanics/code/UAI_UtilityGameActionController|UAI_UtilityGameActionController]]
