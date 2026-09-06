---
schema: code
kind: UINTERFACE
role: primary
source:
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Turn/Order/TurnOrderInterface.h
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Private/Turn/Order/TurnOrderInterface.cpp
reconciled: 2026-09-06
commit: 452688c
---

# ITurnOrderInterface

`Blueprintable` / `BlueprintType` interface. The strategy contract for "whose turn is
next". All three methods are `BlueprintNativeEvent` — a whole ordering scheme can be a
Blueprint class with no C++.

## When you touch this

- Writing a custom turn order (spiral, initiative, team-based, …).
- Changing what data the manager hands a strategy.

Ships with `USequentialTurnOrderStrategy` (default, forward to next connected
non-forfeited) and `URandomTurnOrderStrategy`.

## Entry points

Implement (BlueprintNativeEvent):

- `GetFirstParticipantIndex(const TArray<FTurnParticipantInfo>&)` — once at game start.
- `GetNextParticipantIndex(const TArray<FTurnParticipantInfo>&, int32 CurrentIndex)` —
  `CurrentIndex` is who just finished.
- `GetStrategyName()` — label for logs / debug UI.

## Collaborators

- **`UTurnBasedParticipantManagerComponent`** holds one as
  `TScriptInterface<ITurnOrderInterface> TurnOrderStrategy` (`meta=(MustImplement=…)`,
  instanced inline) and calls it from `StartTurn` / `AdvanceToNextParticipant`. Unset →
  the manager assigns `USequentialTurnOrderStrategy`.
- Reads `FTurnParticipantInfo` (`bConnected`, `bForfeited`, `SlotIndex`).

## Gotchas

- Return **`INDEX_NONE` (-1)** when no valid next participant exists — the manager treats
  that as "cannot validly continue" and fires `OnInvalidNumberOfPlayers` (it does **not**
  end the match itself).
- The array passed in is the manager's replicated `Participants`; index into it by field,
  not position, if slot/array order might diverge.
- `EditInlineNew` + `DefaultToInstanced` on the concrete strategies: they're authored
  inline in the manager's Details panel, not as separate assets.

## Cross-impact

A new strategy is self-contained — just implement the interface and assign it. If it
needs extra per-participant data, that goes on `FTurnParticipantInfo` (and its population
in `UTurnBasedParticipantManagerComponent`).

## See also

- In-repo: `Docs/README.md` → *Turn/Order*; `Docs/Systems.md` → *Turn Order*.
