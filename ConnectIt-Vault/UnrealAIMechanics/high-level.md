# UnrealAIMechanics — high level

## Purpose

A small, self-contained **Utility AI** (weighted-scoring decision) plugin — a generic
action-picker pattern, distinct from
[[../UnrealGameIntelligence/high-level|UnrealGameIntelligence]]'s tree search.
`UAI_UtilityGameActionController` holds an `Instanced` array of pluggable
`UAI_UtilityGameActionEvaluator` objects; each evaluator scores candidate actions using a
per-`FGameplayTag` weight map (`GetEvaluatedActions()` is a `BlueprintNativeEvent`, so
scoring logic lives in C++ or Blueprint subclasses), and results flatten into one
`TArray<FGameActionEvaluated>` where `UpdateScore()` sums a per-source `Scoring` audit map
into a total. `UAI_UtilityLibrary::SortGameActionEvaluations()` ranks an evaluated array.

## Status

**Dormant.** Not listed in `ConnectIt.uproject` or the game module's build dependencies.
`UAI_UtilityGameActionController::SelectEvaluatedAction()` is an unimplemented stub (always
returns a default `FGameActionEvaluated`), so `PickNextAction()` does not yet actually
pick anything. Needs `SelectEvaluatedAction()` implemented and the plugin enabled before
it can be used.

## Key modules

- `UnrealAIMechanics` (Runtime). Single sub-area: `Utility/`. No plugin-level
  dependencies.

## Public API surface

- `UAI_UtilityGameActionController` (`PickNextAction`, `GetEvaluatedActions`,
  `SelectEvaluatedAction`, `GameActionHandlers` array).
- `UAI_UtilityGameActionEvaluator` (`GetEvaluatedActions` native event,
  `SetEvaluationWeights`, `EvaluationWeights` map).
- `UAI_UtilityLibrary` (`SortGameActionEvaluations`).
- `FGameActionEvaluated` struct (tag + context object + score + `Scoring` map +
  `UpdateScore()`).

## Consumers

None yet.

## In-repo reference

- [`../../Plugins/UnrealAIMechanics/Docs/README.md`](../../Plugins/UnrealAIMechanics/Docs/README.md)
- [`../../Plugins/UnrealAIMechanics/Docs/Systems.md`](../../Plugins/UnrealAIMechanics/Docs/Systems.md)

## Known rough edges

- `SelectEvaluatedAction()` is a stub — the plugin does not function end to end.
- Not enabled in the `.uproject`.

_Verify against the in-repo `Docs/` before acting on this list — it reflects a past snapshot._
