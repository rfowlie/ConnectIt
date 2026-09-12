# CLAUDE.md — UnrealAIMechanics

A small, self-contained **Utility AI** (weighted-scoring decision) plugin — a generic
action-picker pattern, distinct from
[UnrealGameIntelligence](../UnrealGameIntelligence/CLAUDE.md)'s tree search.
`UAI_UtilityGameActionController` holds an `Instanced` array of pluggable
`UAI_UtilityGameActionEvaluator` objects; each scores candidate actions from a
per-`FGameplayTag` weight map (`GetEvaluatedActions()` is a `BlueprintNativeEvent`), and
results flatten into one `TArray<FGameActionEvaluated>` where `UpdateScore()` sums a
per-source `Scoring` audit map. `UAI_UtilityLibrary::SortGameActionEvaluations()` ranks
the array.

## Domain

- **Source:** `../Plugins/UnrealAIMechanics/Source/` — one Runtime module, single
  sub-area `Utility/`. No plugin-level deps. Convention: `AI_` prefix, no delegates, no
  interfaces, low comment density.
- **Status:** **dormant** — not in `ConnectIt.uproject` or any build deps.
  `UAI_UtilityGameActionController::SelectEvaluatedAction()` is an unimplemented stub
  (returns a default), so `PickNextAction()` picks nothing. Needs that method + enabling
  before use.
- **Type:** game-agnostic plugin, independently extractable.
- **Consumed by:** nothing yet.

## Sections

- **`code/`** — 2 type pages + `code/__INDEX.md`. Governed by
  [`_core/_schema/_code.md`](../_core/_schema/_code.md). No `systems/` / `recipes/` while
  dormant.
- `logs/` — created on first use.

## Start here

[code/__INDEX.md](code/__INDEX.md).

## Known rough edges

- `SelectEvaluatedAction()` is a stub — the plugin does not function end to end. The
  header carries an open design TODO (make selection virtual for C++ and Blueprint) and a
  dead commented `GetEvaluatedActionsByClass` overload.
- Not enabled in the `.uproject`.
