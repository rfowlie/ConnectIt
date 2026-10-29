---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/ActionLoadoutDataAsset.h
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Private/Action/ActionLoadoutDataAsset.cpp
reconciled: 2026-10-29
commit: efc2c27
---

# UActionLoadoutDataAsset

`UDataAsset`. The designer-authored seam between content and the action system: five
system-action class slots, the permanent action list, and a turn-end requirement tree.
[[UnrealTurnBasedMechanics/code/UTurnBasedActionsComponent|UTurnBasedActionsComponent]]
reads exactly one of these in `InitialiseFromLoadout`, and each player's `ATurnBasedPlayerState` is
seeded from it (`InitialiseActionState`).

## When you touch this

- Authoring a loadout asset for a controller.
- Adding, removing, or renaming a system-action slot.
- Adding an action to a loadout, changing its uses/cooldown, or authoring the turn-end tree.

## Entry points

- **Slots (`TSubclassOf`):** `RootActionClass` (mandatory),
  `IdleViewerActionClass`, `SpectatorViewerActionClass`, `PauseViewerActionClass`,
  `AwaitingConfirmationActionClass`.
- **`PermanentActions`** (`TArray<FPermanentActionConfig>`, keyed by `ActionClass`; the tag is derived via
  `UTurnBasedPlayerAction::GetTagForClass`): each action once, with `MatchUses` (-1 = unlimited), `TurnUses` (>= 1)
  and `CooldownTurns` -- see [[UnrealTurnBasedMechanics/code/ActionConfig|ActionConfig]]. The PlayerState is seeded
  from it unless the game hands it its own list (`InitialiseActionState(Loadout, List)`). Live state is on the
  PlayerState. **Item actions are not in the loadout** -- they are the player's inventory.
- **Turn end:** `TurnEndRequirements` (`Instanced` `UTurnEndRequirementNode` tree): **Group** (Any / All),
  **Action Used** (`ActionClass` + `RequiredUsesThisTurn`; permanent uses only) and **Item Used**
  (`RequiredItemUses`; any item action). `bIsEnabled == false` nodes are skipped. Unset ⇒ the turn can be ended at any
  time. `HasTurnEndRequirements()`.
- **Vending (each `NewObject(Outer)`, null if class unset):** `GetRootAction`,
  `GetIdleViewerAction`, `GetSpectatorAction`, `GetPauseAction`,
  `GetAwaitingConfirmationAction`.
- `IsDataValid` (`WITH_EDITOR`) — errors on an entry with no `ActionClass`, a class listed twice, `TurnUses` below
  1, an action with no tag, a turn-end leaf naming a class not in the list, or a leaf needing more uses than the
  action's `TurnUses`; warns on `MatchUses` 0, a leaf with no class, no root, no actions, no tree.

## Collaborators

- Consumed by `UTurnBasedActionsComponent` (`InitialiseFromLoadout` → `CreateSystemActions`; the turn actions are
  built from the PlayerState's actions) and by
  [[UnrealTurnBasedMechanics/code/ATurnBasedPlayerState|ATurnBasedPlayerState]]`::InitialiseActionState`.
- References [[UnrealTurnBasedMechanics/code/UTurnBasedPlayerAction|UTurnBasedPlayerAction]] /
  `UTurnBasedSpectatorAction` subclasses by class.

## Gotchas

- **`RootActionClass` is mandatory** — an unset root leaves the stack with nothing to
  fall back to. `IsDataValid` should catch it; author a do-nothing action rather than
  leave it blank.
- Vending getters return **null**, not a default, when a slot is unset — the component
  logs via `WarnIfViewerActionMissing` and carries on degraded.
- Entries are *class references*, not instances: the component builds its own actions from them and the
  PlayerState copies the numbers once at seed time — editing the asset at runtime does not change live
  components or already-seeded state.
- Turn end is decided **only** by the `TurnEndRequirements` tree (`CanAutoEndTurn`); the old
  `bIsRequired` / `RequiredActionTagA/B` mechanism is removed.

## Cross-impact

A new slot must be added in four places together:
this asset (property + `Get*Action` + `IsDataValid`),
`UTurnBasedActionsComponent` (matching `TSubclassOf` property, `CreateSystemActions`,
the `Notify*` that pushes it), the loadout assets themselves, and — if surfaced —
`UDWidget_TurnBasedActionsComponent`.

## Changes

- 2026-10-24 — **one permanent list**: `NumberedActions` removed; `PermanentActions` entries carry `MatchUses` /
  `TurnUses` / `CooldownTurns`; the **Item Used** turn-end leaf added; validation updated. Player-actions plan,
  Stages 2-3.
- 2026-09-21 — **redesigned**: `Actions` (instanced), `BannedActionTags` and the permitted/required/optional
  queries removed; class-keyed `PermanentActions` / `NumberedActions` and the `TurnEndRequirements` tree added.
  See [legacy removal](../../ConnectIt/_decisions/2026-09-20-legacy-action-system-removed-stage-3.md),
  [class-keyed config](../../ConnectIt/_decisions/2026-09-18-action-config-keyed-by-class-not-tag.md),
  [turn-end tree](../../ConnectIt/_decisions/2026-09-18-turn-end-tree-as-instanced-uobject-nodes.md).
- 2026-09-18 — internal-only: `Action->ActionTag` field reads became
  `Action->GetActionTag()` calls (see
  [[UnrealTurnBasedMechanics/code/UTurnBasedActionBase|UTurnBasedActionBase]]'s own
  Changes) — no change to this page's public surface or behavior. (`process-code` sweep
  — commit `9187568`.)
- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[UnrealTurnBasedMechanics/CLAUDE|UnrealTurnBasedMechanics overview]] → *Action*.
- [[UnrealTurnBasedMechanics/code/recipes/add-a-turn-action|recipes/add-a-turn-action]]
