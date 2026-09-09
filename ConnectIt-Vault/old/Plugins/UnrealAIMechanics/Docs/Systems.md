# UnrealAIMechanics — Systems & Conventions

[README.md](README.md) is the exhaustive per-class reference — every class, its base, its purpose, its key API. This document is the other half: a narrative walkthrough of how the plugin's one system actually works end to end, and the conventions in use across it, for anyone orienting themselves before extending or (eventually) integrating the plugin. Read this first; use README.md to look up a specific class once you know how the pieces fit.

## Systems Overview

### Utility-AI Evaluation & Scoring (`Utility/`)

This plugin implements exactly one pattern — classic Utility AI — across four small classes, and it's worth walking through the intended data flow end to end, including where that flow currently stops short.

**The intended flow**, driven by `UAI_UtilityGameActionController::PickNextAction()`:

1. The controller holds an `Instanced` array of pluggable evaluators, `TArray<UAI_UtilityGameActionEvaluator*> GameActionHandlers` — each one a self-contained scoring strategy (e.g. "score actions based on board proximity," "score actions based on resource value"), configured independently via `SetEvaluationWeights(TMap<FGameplayTag, float>)`.
2. `GetEvaluatedActions()` loops every non-null handler in that array, calls each evaluator's own `GetEvaluatedActions()` (a `BlueprintNativeEvent` — the actual scoring logic lives entirely in a subclass override, C++ or Blueprint), and appends every evaluator's results into one flat `TArray<FGameActionEvaluated>`.
3. Each `FGameActionEvaluated` is a payload — a `GameplayActionTag`, an optional `GameplayActionContext` object, a running `Score`, and a `Scoring` map that records *where* that score came from (one entry per contributing source/reason). `UpdateScore()` sums the `Scoring` map into `Score` — the map is the audit trail, `Score` is the derived total, kept for debugging so a designer can see why one action outscored another rather than just the final number.
4. `PickNextAction()` hands the full flattened array to `SelectEvaluatedAction()`, whose job is to pick the single best-scored entry and return it as the controller's chosen action.

**Where it currently stops**: step 4 isn't implemented yet. `SelectEvaluatedAction()`'s body is `return FGameActionEvaluated();` — it takes the evaluated array as a parameter and ignores it entirely, so `PickNextAction()` today always returns an empty, default-constructed result no matter what the evaluators produced. Steps 1–3 (orchestration, scoring, aggregation) are real and functional; step 4 (the actual "pick the best one" decision) is the one piece of the pattern not yet written. See Known Rough Edges below.

Independently of the controller, `UAI_UtilityLibrary::SortGameActionEvaluations()` offers a standalone way to rank an already-evaluated array by `Score` (ascending or descending) — useful for a caller that wants to inspect or display the whole ranked list rather than only the single best pick `SelectEvaluatedAction()` is meant to return.

Because this plugin is not currently wired into any consuming module's build dependencies (see README's Purpose/Key Dependencies), none of this has real callers yet — it's infrastructure sitting ready rather than an active decision loop.

## Conventions

- **Type prefixes**: standard `U`/`F` only — no interfaces (`I`), no actors (`A`), no enums (`E`) anywhere in this plugin. The smallest type-prefix footprint of any plugin in this repo.
- **`AI_` name prefix**: every class/struct in `Utility/` is additionally prefixed `AI_` ahead of the standard `U`/`F` (`UAI_UtilityGameActionController`, `UAI_UtilityGameActionEvaluator`, `UAI_UtilityLibrary`, `FGameActionEvaluated` is the one exception — it dropped the `AI_` infix despite living in the same folder as the rest).
- **No delegates at all** — unlike its sibling plugins (`UnrealGameMechanics`, `UnrealTurnBasedMechanics`), nothing in this plugin declares a delegate, dynamic or native. Communication is purely synchronous call/return (`GetEvaluatedActions` fills an out-array, `PickNextAction`/`SelectEvaluatedAction` return a value directly).
- **`Category` strings are mostly consistent, but not universally applied**: every `UFUNCTION`/`UPROPERTY` that does specify one uses the same verbatim style, `"AI | Utility"` or `"AI | Utility | Evaluator"` (pipe with spaces on both sides, hierarchical). But two `UFUNCTION`s on the controller — `GetEvaluatedActions()` and `SelectEvaluatedAction()` — specify no `Category` at all, unlike `PickNextAction()` right above them in the same class, which does. So the convention itself is uniform where it's used; it just isn't applied everywhere it could be.
- **`BlueprintNativeEvent` is the extension point**: `UAI_UtilityGameActionEvaluator`'s two functions are both `BlueprintNativeEvent` with empty `_Implementation` bodies in the base class — the entire point of the class is to be subclassed (in C++ or Blueprint) and overridden per scoring strategy, matching the strategy-pattern description in README.md's Notable Design Patterns.
- **Comment density is low and consistent** — every file still carries the default `// Fill out your copyright notice...` header comment (never replaced with a real copyright/file banner), and prose comments beyond that are sparse: a couple of one-line "what this does" comments (`AI_UtilityStructs.h`'s payload comment, `CodingUtilsComponentLibrary`-style inline notes are absent here) and exactly one `TODO`. This plugin hasn't been recently revisited the way parts of `UnrealGameMechanics` have — nothing here carries the denser, rationale-heavy comment style noted in that plugin's own Systems.md.

## Known Rough Edges

This plugin doesn't have its own cross-cutting duplication/issues file the way the consuming project does — these are noted here rather than starting a second file for one plugin's internal findings. Each also has a pointer from the relevant row in [README.md](README.md)'s class table.

- **`UAI_UtilityGameActionController::SelectEvaluatedAction()` is an unimplemented stub.** Its body is `return FGameActionEvaluated();` — the input array of evaluated candidates is accepted as a parameter and then never read. `PickNextAction()` calls straight into it, so the controller's headline "pick the best action" entry point currently always hands back an empty result regardless of what the evaluators scored. This is a distinct gap from the item below — even if `SelectEvaluatedAction` were made virtual today, it would still need an actual selection body. See [README.md](README.md#utility) (`UAI_UtilityGameActionController` row).
- **Open design-question TODO on the same method** (`Public/Utility/AI_UtilityGameActionController.h` line 33): *"this should maybe be another class, how do we make it virtual for C++ and blueprint again?"* — the author is flagging that `SelectEvaluatedAction()` isn't virtual/overridable, and is unsure of the cleanest way to make selection strategy pluggable the way evaluation strategy already is (via the `Instanced` evaluator array). No resolution recorded yet; a future pass could either make the method virtual directly or split selection out into its own pluggable strategy object, mirroring the evaluator pattern. See [README.md](README.md#utility).
- **A commented-out `GetEvaluatedActionsByClass` overload** sits in both `AI_UtilityGameActionController.h` and its `.cpp` — a filtered variant that would return evaluations from only one evaluator class. Dead code, no functional impact; a small cleanup candidate independent of any behavioral change.
