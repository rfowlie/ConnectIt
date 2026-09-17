---
schema: recipes
task: Add a new player move / ability as a UTurnBasedAction subclass and make it selectable during a turn.
touches:
  - a new UTurnBasedAction subclass (C++ or Blueprint)
  - the controller's UActionLoadoutDataAsset (Actions array)
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/TurnBasedAction.h
  - GetActionTag() override (C++ or the Blueprint event), ConstructInputBindings() override
    (if it needs named inputs), project gameplay tags (FTurnActionRequest.RequestType)
  - project USTRUCT for FTurnActionRequest::Payload (only if the action changes the board)
  - UDWidget_TurnBasedActionsComponent (only if an action bar surfaces the new action)
reconciled: 2026-09-10
commit: acbfa6f
---

# Recipe: add a turn action

## Goal

A new [[UnrealTurnBasedMechanics/code/UTurnBasedAction|UTurnBasedAction]] subclass that a player can activate on their turn — with optional grid-tile selection and an optional board-change request — showing up through the existing action stack with no engine-side changes.

## Prerequisites

- The controller already has a
  [[UnrealTurnBasedMechanics/code/UTurnBasedActionsComponent|UTurnBasedActionsComponent]]
  initialised from a
  [[UnrealTurnBasedMechanics/code/UActionLoadoutDataAsset|UActionLoadoutDataAsset]].
- A gameplay tag exists (or add one) for — if it changes the board — the
  `FTurnActionRequest::RequestType`.

## Steps

1. **Create the subclass.** Derive from `UTurnBasedAction` (Blueprint child of a C++
   base is the common path). Implement `GetActionTag()` (a `BlueprintNativeEvent` — a
   C++ override, or the Blueprint event node) to return this action's identity tag; it's
   not a settable data field, each concrete action hardcodes its own. Set config:
   `bIsRequired`, `bIsCancellable`, `MaxCompletionsPerTurn`, `CooldownTurns`, and
   `bRequiresSelection` if it needs a tile.
2. **Selection behaviour (if `bRequiresSelection`).** Override the BlueprintNativeEvents:
   `IsValidHoverTile` / `IsValidSelectionTile` (predicate on `AGridTileBase`),
   `HandleValidHover` / `HandleHoverCleared` (preview), `HandleValidSelection` (commit),
   `ClearSelectionState` (teardown). `BindInput()` / `BindGridSubsystem()` are called
   automatically from `Activate_Internal` when `bRequiresSelection` is true; for a
   deferred selection leave the flag false and call them yourself when ready.
3. **Named inputs (optional).** Override `ConstructInputBindings()` (called once from
   `InitialiseAction`, before the input binder is built) to populate `InputBindings` —
   for each `FInputTagBinding`, set `InputAction`/`TriggerEvent`/`Key` and bind
   `InputActionDelegate` (e.g. `BindDynamic`) directly to whatever handler this input
   should run. No shared tag-switch consumer to extend — each binding names its own
   behaviour.
4. **Board change (if it mutates the board).** From `HandleValidSelection`, build an
   `FTurnActionRequest` (`RequestType` tag, `FactionID`, and a project `USTRUCT` wrapped
   in `Payload`) and call the protected `RequestBoardChange(Request)`. Do **not** apply
   the change locally — wait for the server. See
   [[UnrealTurnBasedMechanics/code/systems/action-stack-lifecycle|action-stack-lifecycle]] step 4–5.
5. **Completion.** Call `Complete()` on success (increments `CompletionsThisTurn`) or
   `Cancel()` if the player backs out. Override `OnCompleted` / `OnCancelled` for
   cleanup.
6. **Register it.** Add an instance to the loadout asset's `Actions` array (or, for a
   system action, a slot). Make sure its `GetActionTag()` value is not in `BannedActionTags`,
   and check the loadout's Data Validation output — it warns per-action if `GetActionTag()`
   comes back empty or duplicates another action's.
7. **Chaining (optional).** To push a follow-up action, call
   `RequestNextAction(TSubclassOf<UTurnBasedAction>)`.

## Verify

- PIE with the loadout assigned. On your turn, the action is in
  `UTurnBasedActionsComponent::GetPermittedActions()` and `TryPushAction(ActionTag)`
  activates it.
- Watch `UDWidget_TurnBasedActionsComponent` (`GetInfo()` → `TopActionTag`, `StackDepth`,
  `bAwaitingRequestConfirmation`) as you activate, select, and complete.
- For a board-change action: after `HandleValidSelection`, `IsAwaitingRequestConfirmation()`
  is true and the stack is frozen until your project glue calls
  `NotifyBoardChangeOutcome`.
- `CompletionsThisTurn` increments on `Complete()` only; confirm `MaxCompletionsPerTurn`
  and `CooldownTurns` gate re-activation via `CanActivate()`.

## Pitfalls

- **Forgetting to register in the loadout** — the class compiles but never appears.
- **Applying the board change locally** instead of routing through `RequestBoardChange`
  → desync and a stuck confirmation freeze.
- **Never calling `Complete()` / `Cancel()`** — the turn can't auto-end and the stack
  entry lingers.
- Leaving `bRequiresSelection` true but not overriding the selection hooks → input bound,
  nothing happens on click.
- A new action bar surfacing the action must bind the `OnAction*Safe` snapshot delegates,
  not the raw-pointer ones.
- If the action is `bIsRequired`, it now factors into `CanAutoEndTurn()` — an unreachable
  required action blocks turn end.

## See also

- [[UnrealTurnBasedMechanics/code/UTurnBasedAction|UTurnBasedAction]] ·
  [[UnrealTurnBasedMechanics/code/UActionLoadoutDataAsset|UActionLoadoutDataAsset]] ·
  [[UnrealTurnBasedMechanics/code/UTurnBasedActionsComponent|UTurnBasedActionsComponent]]
- In-repo: [[UnrealTurnBasedMechanics/CLAUDE|UnrealTurnBasedMechanics overview]] → *Action*.
