---
schema: code
kind: UINTERFACE
role: primary
source:
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Public/Utility/Interface/UtilityScoreHandler.h
reconciled: 2026-09-06
commit: 0ed7243
---

# IUtilityScoreHandler

A tiny `BlueprintNativeEvent` interface for on-screen "utility score" debug display —
get/set a float and show/hide its readout. Unrelated to the InfluenceMap and MinMax
halves of the plugin.

## When you touch this

- Putting a numeric AI/heuristic score above an actor as a debug aid.

## Entry points

Implement (all `BlueprintNativeEvent`, `Category = "GameIntelligence | Utility"`):

- `float GetUtilityScore()`
- `void SetUtilityScore(const float InScore)`
- `void ShowUtilityScore()` / `void HideUtilityScore()`

## Collaborators

None in the plugin — a bare contract. The implementer owns the widget/text-render and the
score's meaning.

## Gotchas

- Header comment is the stock UHT boilerplate ("does not need to be modified") — that's
  the wrapper `UUtilityScoreHandler`, not guidance about `IUtilityScoreHandler`.
- Purely display — nothing computes or stores the score for you.

## Cross-impact

Self-contained.

## See also

- In-repo: `Docs/README.md` → *Utility*.
