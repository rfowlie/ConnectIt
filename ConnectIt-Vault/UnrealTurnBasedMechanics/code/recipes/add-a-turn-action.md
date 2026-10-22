---
schema: recipes
task: Add a new player move / ability as a UTurnBasedAction subclass and make it selectable during a turn.
touches:
  - a new UTurnBasedAction subclass (C++ or Blueprint)
  - the controller's UActionLoadoutDataAsset (PermanentActions / NumberedActions entry)
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/TurnBasedAction.h
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/ActionConfig.h
  - GetActionTag() override (C++ or the Blueprint event), ConstructInputBindings() override
    (if it needs named inputs), ProducesRequestType() (if it changes the board)
  - a project USTRUCT for the request payload (only if the action changes the board)
  - UDWidget_TurnBasedActionsComponent (only if an action bar surfaces the new action)
reconciled: 2026-10-22
commit: ee45a98
---

# Recipe: add a turn action

## Goal

A new [[UnrealTurnBasedMechanics/code/UTurnBasedAction|UTurnBasedAction]] subclass that a player can activate on their turn — with optional grid-tile selection and an optional board-change request — showing up through the existing action stack with no engine-side changes.

## Prerequisites

- The controller already has a
  [[UnrealTurnBasedMechanics/code/UTurnBasedActionsComponent|UTurnBasedActionsComponent]]
  initialised from a
  [[UnrealTurnBasedMechanics/code/UActionLoadoutDataAsset|UActionLoadoutDataAsset]], and the server seeds
  the player's `ATurnBasedPlayerState` action state from that loadout (the game's setup does; the
  component doesn't).
- If it changes the board: a project `USTRUCT` describing the change (ConnectIt: a board operation).

## Steps

1. **Create the subclass.** Derive from `UTurnBasedAction` (Blueprint child of a C++
   base is the common path). Implement `GetActionTag()` (a `BlueprintNativeEvent` — a
   C++ override, or the Blueprint event node) to return this action's identity tag; it's
   not a settable data field, each concrete action hardcodes its own. Action config on the
   class is only `bIsCancellable` and `bRequiresSelection` (if it needs a tile), plus the
   presentation fields.
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
   should run.
4. **Board change (if it mutates the board).** From `HandleValidSelection`, build the
   payload struct and call the protected `RequestBoardChange(FInstancedStruct::Make(...))`:
   it wraps it in an `FTurnActionRequest` stamped with this action's tag. Override
   `ProducesRequestType` to say which request types this action may send (the server's gate
   checks it). Do **not** apply the change locally — wait for the server. See
   [[UnrealTurnBasedMechanics/code/systems/action-stack-lifecycle|action-stack-lifecycle]].
5. **Completion.** Call `Complete()` on success or `Cancel()` if the player backs out;
   override `OnCompleted` / `OnCancelled` for cleanup. Uses are counted by the server when a
   request commits, not by `Complete()`.
6. **Register it in the loadout**, keyed by class:
   - **`PermanentActions`** (`FPermanentActionConfig`): always available -- `MaxUsesPerTurn`
     (0 = unlimited), `CooldownTurns`.
   - **`NumberedActions`** (`FNumberedActionConfig`): a budget -- `StartingMatchUses`,
     `MaxUsesPerTurn`, `MaxHeldUses`, `CooldownTurns`.
   - Or a system slot (root / idle / spectator / pause / awaiting confirmation).
   - If the turn should only end once it's been used, reference it from a leaf of the
     loadout's `TurnEndRequirements` tree.
   Check the loadout's Data Validation output (empty or duplicate action tags are warned).
7. **Chaining (optional).** To push a follow-up action, call
   `RequestNextAction(TSubclassOf<UTurnBasedAction>)`.

## Verify

- PIE with the loadout assigned. On your turn, `TryPushActionByClass` / `TryPushAction(ActionTag)`
  activates it, and `CanActivate()` (asks the PlayerState) reflects its uses, per-turn cap and
  cooldown.
- Watch `UDWidget_TurnBasedActionsComponent` (`GetInfo()` → `TopActionTag`, `StackDepth`,
  `bAwaitingRequestConfirmation`) as you activate, select, and complete.
- For a board-change action: after `HandleValidSelection`, `IsAwaitingRequestConfirmation()`
  is true and the stack is frozen until your project glue calls `NotifyBoardChangeOutcome`;
  after a success the component sits in limbo until the PlayerState's updated counts arrive.

## Pitfalls

- **Forgetting to register in the loadout** — the class compiles but never appears, and the
  server refuses its requests (not in the loadout).
- **Applying the board change locally** instead of routing through `RequestBoardChange`
  → desync and a stuck confirmation freeze.
- **`ProducesRequestType` left at its default (true)** lets the action send any request type;
  override it so the server's gate can refuse the wrong one.
- **Never calling `Complete()` / `Cancel()`** — the stack entry lingers.
- Leaving `bRequiresSelection` true but not overriding the selection hooks → input bound,
  nothing happens on click.
- A new action bar surfacing the action must bind the `OnAction*Safe` snapshot delegates,
  not the raw-pointer ones.
- A `TurnEndRequirements` leaf that can never be satisfied blocks turn end.

## See also

- [[UnrealTurnBasedMechanics/code/UTurnBasedAction|UTurnBasedAction]] ·
  [[UnrealTurnBasedMechanics/code/UActionLoadoutDataAsset|UActionLoadoutDataAsset]] ·
  [[UnrealTurnBasedMechanics/code/UTurnBasedActionsComponent|UTurnBasedActionsComponent]]
- In-repo: [[UnrealTurnBasedMechanics/CLAUDE|UnrealTurnBasedMechanics overview]] → *Action*.
