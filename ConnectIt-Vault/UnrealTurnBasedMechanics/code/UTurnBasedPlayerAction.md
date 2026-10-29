---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Action/TurnBasedPlayerAction.h
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Private/Action/TurnBasedPlayerAction.cpp
reconciled: 2026-10-29
commit: cb9f405
---

# UTurnBasedPlayerAction

`Abstract`, extends [[UnrealTurnBasedMechanics/code/UTurnBasedActionBase|UTurnBasedActionBase]]. A **Player Action**:
something a participant chooses to do on their turn (place a piece, swap two...). It has a grid-tile
hover → validate → select pipeline and data-driven named input bindings, builds a board-change request for the
server, and can chain into another action. This is the class you subclass most often (usually in Blueprint). It was
`UTurnBasedAction`; a class redirect keeps old assets loading. Spectator actions (`UTurnBasedSpectatorAction`) share
the base but aren't player actions.

## When you touch this

- Adding a new player move: [[UnrealTurnBasedMechanics/code/recipes/add-a-turn-action|recipes/add-a-turn-action]].
- Changing the selection pipeline, how board changes are requested, or how an item use is marked.

## Entry points

- **Config:**
  - `bIsCancellable`, `bRequiresSelection`;
  - `InputBindings` (`TArray<FInputTagBinding>`, filled by `ConstructInputBindings`);
  - presentation fields `DisplayName`, `Description`, `Icon` (UI only).
- **No uses on the action.** How often it may be used belongs to the set-up: a permanent action list
  ([[UnrealTurnBasedMechanics/code/ActionConfig|FPermanentActionConfig]]) or an item in the inventory. The live
  state is on [[UnrealTurnBasedMechanics/code/ATurnBasedPlayerState|ATurnBasedPlayerState]]; the action only reads
  it.
- **Call:**
  - `Complete()`, `Cancel()`, `RequestNextAction(TSubclassOf<UTurnBasedPlayerAction>)`;
  - `CanActivate()`: a client-side convenience. For a permanent use it asks the PlayerState about uses, the per-turn
    cap and cooldown; for an item use, whether an item is held and the budget has a use left. The server re-checks.
- **Item use:** `IsItemUse()` is set by the actions component's `ActivateItemAction` (`SetItemUse`) for that
  activation, until the action completes or is cancelled. A refused request that is retried stays an item use. Its
  request carries `bIsItemUse`.
- **Read the PlayerState** (every call; 0 / false with none):
  - `GetPermanentRuntimeState(out)`, `GetUsesThisTurn()`;
  - `GetMaxUsesPerTurn()` (the effective cap, at least 1);
  - `GetMatchUsesRemaining()` (-1 = unlimited);
  - static `GetTagForClass(TSubclassOf)`, which reads the tag off the class default object.
- **Override (BlueprintNativeEvent):**
  - `OnCompleted` / `OnCancelled`;
  - `ProducesRequestType(FGameplayTag)`: default true. The Mediator checks a request's operation tag against the
    action named by its `ActionTag`, so one action can't spend its uses sending another's request;
  - selection hooks `IsValidHoverTile`, `IsValidSelectionTile`, `HandleValidHover`, `HandleHoverCleared`,
    `HandleValidSelection`, `HandleTileClicked` (no-op by default), `ClearSelectionState`;
  - `ConstructInputBindings()`.
- **Protected helpers:**
  - `RequestBoardChange(const FInstancedStruct& Payload)`: builds the `FTurnActionRequest` (this action's tag, the
    payload, `bIsItemUse`) and refuses an empty payload;
  - `BindInput()` / `UnbindInput()`, `BindGridSubsystem()` / `UnbindGridSubsystem()`;
  - `CurrentHoveredTile`, `GetOwnerPlayerState()`.

## Collaborators

- **Enhanced Input:** owns a [[UnrealGameMechanics/code/UInputTagBinder|UInputTagBinder]] hosting the
  `InputBindings` mapping context. Each `FInputTagBinding` dispatches straight to its own delegate.
  `bRequiresSelection` calls `BindInput()` from `Activate_Internal`.
- **Grid:** `UGridHoverSubsystem` relays reach the selection hooks:
  - `OnGridTileHoverChanged` → `HandleValidHover`;
  - `OnGridTileHoverEnded` → `HandleHoverCleared` (the tile it holds as hovered);
  - `OnGridTileClicked` → `HandleTileClicked`.

  See [[UnrealGridMechanics/code/systems/hover-relay|hover-relay]].
- **Fires:**
  - `OnChangeRequested(+_Native)`;
  - `OnActionCompleted` / `OnActionCancelled` (typed, + `_Native`);
  - `OnNextActionRequested(+_Native)`, which the component turns into a push.

## Gotchas

- Uses are counted **by the server**, when the request commits (the Mediator's `ConsumeActionUse` or
  `ConsumeItemUse`), not by `Complete()`.
- An item use never touches the action's permanent counters, and works for an action the player has no permanent uses
  of.
- An action that defers selection should leave `bRequiresSelection = false` and call `BindInput()` when ready.
- `GetActionTag()` (on the base) is the identifier; the presentation fields are display strings.
- Board change goes `RequestBoardChange` → `OnChangeRequested` → the component's `HandleBoardChangeRequested`, which
  freezes the stack. See [[UnrealTurnBasedMechanics/code/systems/action-stack-lifecycle|action-stack-lifecycle]].

## Cross-impact

- A new subclass is listed in a loadout's `PermanentActions`, or in the item inventory asset, keyed by class.
- If it should gate turn end, a `TurnEndRequirements` leaf references it.
- A new kind of request is a new project `USTRUCT` handed to `RequestBoardChange` (ConnectIt: a board operation; the
  operation tag comes from the payload).

## Changes

- 2026-10-28 -- `HandleTileClicked`; hover end clears the hovered tile (hover component plan).
- 2026-10-24 -- **renamed `UTurnBasedPlayerAction`** (was `UTurnBasedAction`, with a class redirect). Item uses
  added: `IsItemUse` / `SetItemUse`, and `RequestBoardChange` stamps `bIsItemUse`. `GetUsesRemaining` became
  `GetMatchUsesRemaining`; numbered state is gone (one permanent list). See
  [player actions, tile powers, board operations](../../ConnectIt/_decisions/2026-10-23-player-actions-tile-powers-board-operations.md).
- 2026-10-22 -- `RequestBoardChange` takes the payload only and builds the request.
- 2026-09-21 -- budget / cap / cooldown state removed from the action (to the loadout and PlayerState).
- 2026-09-18 -- input binding rewritten (per-binding delegates).
- 2026-09-10 -- re-ingested to the `_code` schema.

## See also

- [[UnrealTurnBasedMechanics/code/UTurnBasedActionsComponent|UTurnBasedActionsComponent]],
  [[UnrealTurnBasedMechanics/code/ATurnBasedPlayerState|ATurnBasedPlayerState]]
- [[UnrealTurnBasedMechanics/CLAUDE|UnrealTurnBasedMechanics overview]]
