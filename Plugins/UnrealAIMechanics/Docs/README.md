# UnrealAIMechanics

## Purpose

UnrealAIMechanics is a small "Utility AI" (weighted-scoring decision system) plugin: a generic action-picker pattern where candidate actions are evaluated and weighted/scored by GameplayTag, and the best-scored action is selected. It's kept as its own plugin so that a generic, reusable utility-AI decision layer can exist independently of any specific gameplay system, in the same spirit as `UnrealGameIntelligence`'s tree-search toolkit — but this plugin covers a distinct decision-making pattern (weighted action scoring) rather than minimax tree search or influence maps.

**Note:** this plugin is not currently wired into the repo's consuming game module — that module's build file does not list it as a dependency at all, unlike the other four first-party plugins. It exists in the repo as available, self-contained infrastructure, but is currently dormant/unintegrated.

## Module(s)

| Module | Type | LoadingPhase | Notes |
|---|---|---|---|
| `UnrealAIMechanics` | Runtime | Default | Single module; entry point `FUnrealAIMechanicsModule` (`Public/UnrealAIMechanics.h`). |

## Key Dependencies

- Depends on: `Core`, `GameplayTags` (Public); `CoreUObject`, `Engine`, `Slate`, `SlateCore` (Private). No plugin-level dependencies declared in the `.uplugin`.
- Depended on by: nobody currently. Not listed in the consuming game module's build dependencies, and no other first-party plugin depends on it either.

## When to Use It

- Future AI decision-making where candidate actions need to be scored and picked based on weighted GameplayTags (classic Utility AI pattern).
- Any future system that needs a generic "evaluate a set of options, weight them per-tag, pick the best" controller, decoupled from any specific gameplay domain.

See [Systems.md](Systems.md) for a narrative walkthrough of how the evaluator/controller pattern fits together end to end — this README stays the exhaustive per-class reference.

## When NOT to Use It / Scope Boundaries

- This plugin is currently dormant and unintegrated — using it in the current consuming project today means new integration work (adding it to that project's build dependencies, wiring up evaluators), not building on top of an existing dependency.
- Minimax/alpha-beta tree search and influence-map visualization belong in `UnrealGameIntelligence`, not here — this plugin's scope is weighted action scoring, not game-tree search.
- Turn sequencing, event gating, pooling, and other general gameplay scaffolding belong in `UnrealGameMechanics`, not here.

## Notable Design Patterns

- Strategy/evaluator pattern: `UAI_UtilityGameActionController` orchestrates a set of pluggable "evaluator" objects (`UAI_UtilityGameActionEvaluator`, held as `Instanced`) and picks the best-scored action, rather than embedding scoring logic directly in the controller.
- Weighted-tag scoring: evaluators score candidate actions using per-GameplayTag weights, and scores from multiple sources are summed into a single total via `FGameActionEvaluated::UpdateScore()`.

See [Systems.md](Systems.md) for how these two patterns compose in practice, including the current gap noted below.

## Classes

### Utility

| Class | Base Class | Purpose | Notes |
|---|---|---|---|
| `UAI_UtilityGameActionController` | `UObject` | Orchestrates a set of evaluator objects and picks the best-scored action. | `Public/Utility/AI_UtilityGameActionController.h`. Key API: `PickNextAction()`, `GetEvaluatedActions()`, `SelectEvaluatedAction()`; holds `TArray<UAI_UtilityGameActionEvaluator*> GameActionHandlers` (Instanced). Has a TODO at line 33 ("this should maybe be another class, how do we make it virtual for C++ and blueprint again?") flagging that the author isn't fully happy with `SelectEvaluatedAction`'s design (not virtual/overridable) — an open design question, not yet resolved. Separately, `SelectEvaluatedAction()`'s current body is an unimplemented stub — it ignores its input array and always returns a default-constructed `FGameActionEvaluated()`, so `PickNextAction()` doesn't yet actually pick anything. A commented-out `GetEvaluatedActionsByClass` overload also sits in both the header and .cpp — dead code, no functional impact. See Known Discrepancies and [Systems.md](Systems.md#known-rough-edges). |
| `UAI_UtilityGameActionEvaluator` | `UObject` (Abstract/BlueprintNativeEvent) | Scores candidate actions using per-tag weights. | `Public/Utility/AI_UtilityGameActionEvaluator.h`. Key API: `GetEvaluatedActions()` (native event), `SetEvaluationWeights()`, `EvaluationWeights` map. |
| `UAI_UtilityLibrary` | `UBlueprintFunctionLibrary` | Sorts evaluated action arrays by score. | `Public/Utility/AI_UtilityLibrary.h`. Key API: `SortGameActionEvaluations()`. |
| `FGameActionEvaluated` (struct) | — | Payload struct: gameplay tag + context object + score + per-source scoring map. | `Public/Utility/AI_UtilityStructs.h`. `UpdateScore()` sums the `Scoring` map into `Score`. |

## Known Discrepancies / Issues in This Plugin

- Its main notable issue is architectural rather than a code discrepancy: it is currently unintegrated with any consuming project (see Purpose/Key Dependencies above), not a bug or duplication to track in the project-level [duplication notes](../../../Source/ConnectIt/Docs/Duplication.md) or [naming conventions notes](../../../Source/ConnectIt/Docs/Conventions.md).
- `UAI_UtilityGameActionController::SelectEvaluatedAction()` is currently an unimplemented stub — it ignores its input array and always returns a default-constructed `FGameActionEvaluated()`, so `PickNextAction()` doesn't yet actually select anything. Distinct from (but related to) the open TODO about making the method virtual/overridable. See [Systems.md](Systems.md#known-rough-edges).
- A commented-out `GetEvaluatedActionsByClass` overload sits in both the header and .cpp of `UAI_UtilityGameActionController` — dead code, no functional impact. See [Systems.md](Systems.md#known-rough-edges).
