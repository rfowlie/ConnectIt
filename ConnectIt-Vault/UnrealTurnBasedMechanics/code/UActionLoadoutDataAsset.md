---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/ActionLoadoutDataAsset.h
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Private/Action/ActionLoadoutDataAsset.cpp
reconciled: 2026-09-06
commit: 452688c
---

# UActionLoadoutDataAsset

`UDataAsset`. The designer-authored seam between content and the action system: five
system-action class slots, an inline-instanced `Actions` array, and a ban list.
[[UnrealTurnBasedMechanics/code/UTurnBasedActionsComponent|UTurnBasedActionsComponent]]
reads exactly one of these in `InitialiseFromLoadout`.

## When you touch this

- Authoring a loadout asset for a controller.
- Adding, removing, or renaming a system-action slot.
- Changing what "permitted / required / optional" means for turn actions.

## Entry points

- **Slots (`TSubclassOf`):** `RootActionClass` (mandatory),
  `IdleViewerActionClass`, `SpectatorViewerActionClass`, `PauseViewerActionClass`,
  `AwaitingConfirmationActionClass`.
- **Turn actions:** `Actions` (`TArray<TObjectPtr<UTurnBasedAction>>`, `Instanced`),
  `BannedActionTags`.
- **Vending (each `NewObject(Outer)`, null if class unset):** `GetRootAction`,
  `GetIdleViewerAction`, `GetSpectatorAction`, `GetPauseAction`,
  `GetAwaitingConfirmationAction`.
- **Queries:** `GetPermittedActions()`, `GetRequiredActions()`, `GetOptionalActions()`,
  `IsActionPermitted(FGameplayTag)`.
- `IsDataValid` (`WITH_EDITOR`) — validation entry point.

## Collaborators

- Consumed by `UTurnBasedActionsComponent` (`InitialiseFromLoadout` →
  `CloneActionsFromLoadout` + `CreateSystemActions`).
- Holds instances of
  [[UnrealTurnBasedMechanics/code/UTurnBasedAction|UTurnBasedAction]] /
  `UTurnBasedSpectatorAction` subclasses.

## Gotchas

- **`RootActionClass` is mandatory** — an unset root leaves the stack with nothing to
  fall back to. `IsDataValid` should catch it; author a do-nothing action rather than
  leave it blank.
- Vending getters return **null**, not a default, when a slot is unset — the component
  logs via `WarnIfViewerActionMissing` and carries on degraded.
- `Actions` entries are *instanced on the asset* and cloned per component — editing the
  asset at runtime does not retroactively change live components.
- "Required" = `UTurnBasedAction::bIsRequired`; drives `CanAutoEndTurn`'s default.

## Cross-impact

A new slot must be added in four places together:
this asset (property + `Get*Action` + `IsDataValid`),
`UTurnBasedActionsComponent` (matching `TSubclassOf` property, `CreateSystemActions`,
the `Notify*` that pushes it), the loadout assets themselves, and — if surfaced —
`UDWidget_TurnBasedActionsComponent`.

## See also

- In-repo: `Docs/README.md` → *Action*; `Docs/Systems.md` → *The Action Stack*.
- [[UnrealTurnBasedMechanics/recipes/add-a-turn-action|recipes/add-a-turn-action]]
