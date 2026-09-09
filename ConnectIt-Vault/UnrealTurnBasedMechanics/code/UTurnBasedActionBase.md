---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/TurnBasedActionBase.h
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Private/Action/TurnBasedActionBase.cpp
reconciled: 2026-09-06
commit: e28339c
---

# UTurnBasedActionBase

`Abstract UObject`, `EditInlineNew`, `DefaultToInstanced`. Root of the action-stack
hierarchy: the minimal activate / deactivate / force-deactivate contract every stack
entry shares. Subclassed by
[[UnrealTurnBasedMechanics/code/UTurnBasedAction|UTurnBasedAction]] (per-turn) and
`UTurnBasedSpectatorAction` (passive views).

## When you touch this

Rarely directly — you almost always subclass one of its two children. Touch this when
changing the lifecycle contract itself, or `PostInitialiseAction` dependency-caching.

## Entry points

- **Called by the component:** `Activate(AController*)`, `ForceDeactivate()`.
- **Override (BlueprintNativeEvent):** `Activate_Internal`, `Deactivate_Internal`,
  `ForceDeactivate_Internal` (defaults to `Deactivate_Internal`), `PostInitialiseAction`
  (fires once when `OwningController` first becomes valid — cache deps here, not at
  point of use).
- **State:** `IsActive()`, protected `SetIsActive()`, `OwningController`.
- `GetWorld()` override (typed outer); `GetPlayerController()` (null for AI).

## Collaborators

- Owned & driven by
  [[UnrealTurnBasedMechanics/code/UTurnBasedActionsComponent|UTurnBasedActionsComponent]]
  (stack) — never `NewObject`'d loose.
- **Fires:** `OnActivated` / `OnDeactivated` / `OnForceDeactivated` (each with a `_Native`
  sibling), payload `UTurnBasedActionBase*`.

## Gotchas

- `ForceDeactivate()` is the *system-interrupt* path — no completion bookkeeping, safe to
  call when not active. Distinct from a subclass's natural `Complete()` / `Cancel()`.
- `PostInitialiseAction` timing differs per branch (`UTurnBasedAction`: inside
  `InitialiseAction`; spectator: inside `CreateSystemActions`) but both are early and
  well-defined — don't reach for `OwningController` before it.
- `DefaultToInstanced` + `EditInlineNew`: instances live inline in the loadout asset, not
  as separate assets.

## Cross-impact

Change the lifecycle hook set and also update: `UTurnBasedAction` and
`UTurnBasedSpectatorAction` `_Implementation` overrides, and
`UTurnBasedActionsComponent::BindActionDelegates`.

## See also

- In-repo: `old/Plugins/UnrealTurnBasedMechanics/Docs/README.md` → *Action*; `old/Plugins/UnrealTurnBasedMechanics/Docs/Systems.md` → *The Action Stack*.
