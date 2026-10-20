---
Date: 2026-09-20
status: Active
superseded by:
tags:
  - action-state
  - turn-end
  - loadout
  - removal
---
## Decision

Stage 3 of the class-keyed loadout migration: the legacy action system is **deleted**, not
deprecated. The new system (config arrays + `TurnEndRequirements` tree + per-player state on
`ATurnBasedPlayerState`) is the only one.

**Removed**

- `UActionLoadoutDataAsset`: `Actions` (Instanced array), `BannedActionTags`,
  `GetPermittedActions`, `GetRequiredActions`, `GetOptionalActions`, `IsActionPermitted`,
  `FilterPermittedActionsByRequired`, and the old duplicate-tag validation.
- `UTurnBasedAction`: `bIsRequired`, `bAllowsOptionalInterrupt`, `MaxCompletionsPerTurn`,
  `CooldownTurns`, `CompletionsThisTurn`, `TurnsUntilAvailable`, `IsComplete`, `TickCooldown`,
  `ResetTurnState`, `ShouldTickCooldown`, and `Complete()`'s counter/cooldown bookkeeping.
- `UTurnBasedActionsComponent`: `GetRequiredActions`, `TickCooldowns`, the per-turn action
  reset, the legacy `DuplicateObject` clone path, the `bIsRequired` AND branch of
  `CanAutoEndTurn`, and the optional-interrupt check in `TryPushActionByRef` (it only made sense
  with `bIsRequired`).
- `UConnectIt_TurnBasedActionsComponent`: `RequiredActionTagA/B` and its `CanAutoEndTurn`
  override.
- `AConnectIt_PlayerState`: `SwapUsesRemaining`, `GetSwapActionUsesRemaining`,
  `ConsumeSwapUse`, `OnSwapUsesRemainingChanged`, the rep-notify and replication.
- The Mediator's SWAP-specific budget check/consume, and the `bNewActionSystem` branches.
- The `bIsRequired` / `MaxCompletionsPerTurn` / `CooldownTurns` lines in the Place, Swap and
  BoardShift action constructors.

**Behaviour now**

- The component builds one action per `PermanentActions`/`NumberedActions` entry (`NewObject`
  from the class). Empty arrays log a warning and yield no turn actions.
- `CanAutoEndTurn`: with no tree, **true** (nothing required — matches the old "no required
  actions" default, and `CanEndTurn` uses the same function so manual end is unaffected); with a
  tree, each leaf reads `PlayerState::GetActionUsesThisTurnByTag`.
- `UTurnBasedAction::CanActivate` asks the owner's PlayerState when it has action config, and
  otherwise returns true — it is a client convenience; the server gate is the real check.
- **The Mediator gate is mandatory.** A request from a player whose PlayerState has no action
  state is rejected with an error naming the likely cause. This is deliberately loud: the old
  behaviour silently skipped gating when the state was missing, which is how "is the new system
  even live?" became unanswerable. Every request must also carry an `ActionTag` (so
  server-originated request types such as ForcePlace have no path today; none is used).
- The component logs when it seeds the PlayerState, and errors if the server-side controller has
  no PlayerState yet.

**Kept on purpose**

- `UConnectIt_TurnBasedActionsComponent` and `AConnectIt_PlayerState` remain as **empty shell
  classes** — the PlayerController's component-class override, the GameMode and Blueprints
  reference them. Delete when nothing does.
- `bAutoEndTurnOnAllRequiredActionsCompleted` keeps its name (renaming would drop its value on
  every Blueprint); it now means "auto-end when the tree is satisfied".

## Why

Stage 2 was the additive switch; leaving both systems in place meant every code path branched on
`HasActionConfig()`, two counters existed for the same thing, and a failure to seed the new state
silently fell back to the old behaviour. Removing the old paths gives one source of truth and makes
a seeding failure obvious.

## What Would Change It

If a loadout needs an action to end the turn without a tree (a quick "this action always ends
it"), a leaf-less shorthand could be added — but the tree already expresses it.
