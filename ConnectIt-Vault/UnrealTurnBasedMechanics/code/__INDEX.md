---
schema: code
reconciled: 2026-09-10
commit: 3086271
---

# UnrealTurnBasedMechanics — code index

Inventory + module map. Governed by [[_core/_schema/_code|_core/_schema/_code.md]].
Domain overview: [[UnrealTurnBasedMechanics/CLAUDE|CLAUDE.md]].

`status`: **current** = page written & reconciled · **stub** = no page yet · **stale** =
drift detected · **covered in X** = documented inside another type's page.

## Inventory

| Type | Kind | Area | Role | Source (`Public/…`, `Private/…`) | Status |
|---|---|---|---|---|---|
| [[UnrealTurnBasedMechanics/code/UTurnBasedActionsComponent\|UTurnBasedActionsComponent]] | UCLASS (component) | Action | primary | `Action/TurnBasedActionsComponent.{h,cpp}` | current |
| [[UnrealTurnBasedMechanics/code/UTurnBasedAction\|UTurnBasedAction]] | UCLASS (abstract) | Action | primary | `Action/TurnBasedAction.{h,cpp}` | current |
| [[UnrealTurnBasedMechanics/code/UTurnBasedActionBase\|UTurnBasedActionBase]] | UCLASS (abstract) | Action | primary | `Action/TurnBasedActionBase.{h,cpp}` | current |
| [[UnrealTurnBasedMechanics/code/UActionLoadoutDataAsset\|UActionLoadoutDataAsset]] | UCLASS (UDataAsset) | Action | primary | `Action/ActionLoadoutDataAsset.{h,cpp}` | current |
| [[UnrealTurnBasedMechanics/code/UTurnBasedParticipantManagerComponent\|UTurnBasedParticipantManagerComponent]] | UCLASS (component) | Turn/Participant | primary (core) | `Turn/Participant/TurnBasedParticipantManagerComponent.{h,cpp}` | current |
| [[UnrealTurnBasedMechanics/code/UTurnBasedParticipantComponent\|UTurnBasedParticipantComponent]] | UCLASS (component) | Turn/Participant | primary | `Turn/Participant/TurnBasedParticipantComponent.{h,cpp}` | current |
| [[UnrealTurnBasedMechanics/code/ITurnOrderInterface\|ITurnOrderInterface]] | UINTERFACE | Turn/Order | primary (contract) | `Turn/Order/TurnOrderInterface.{h,cpp}` | current |
| UTurnBasedSpectatorAction | UCLASS (abstract) | Action | primary | `Action/TurnBasedSpectatorAction.{h,cpp}` | stub |
| FTurnActionSnapshot / FTurnBasedActionsComponentInfo | USTRUCT | Action | internal (debug) | `Action/TurnBasedActionsComponent.h` | covered in UTurnBasedActionsComponent |
| UTurnBasedPauseAction | UCLASS | Action/Spectator | primary (default) | `Action/Spectator/TurnBasedPauseAction.{h,cpp}` | stub |
| ATurnBasedGameMode / GameState / PlayerState | UCLASS | Framework | primary | `Framework/{GameMode,GameState,PlayerState}/*.{h,cpp}` | stub |
| ATurnBasedPlayerControllerBase / ATurnBasedAIController | UCLASS (abstract) | Framework/Controller | primary | `Framework/Controller/TurnBased{PlayerControllerBase,AIController}.{h,cpp}` | stub |
| UTurnBasedControllerCoordinatorComponent | UCLASS (component) | Framework/Controller | primary (plumbing) | `Framework/Controller/TurnBasedControllerCoordinatorComponent.{h,cpp}` | stub |
| UTurnBasedGameEvent | UCLASS (abstract) | GameEvent | primary (extension point) | `GameEvent/TurnBasedGameEvent.{h,cpp}` | stub |
| USequentialTurnOrderStrategy / URandomTurnOrderStrategy | UCLASS | Turn/Order | `Sequential` = default; `Random` = alt | `Turn/Order/{Sequential,Random}TurnOrderStrategy.{h,cpp}` | stub |
| FTurnBasedParticipantManagerInfo | USTRUCT | Turn/Participant | internal (debug) | `Turn/Participant/TurnBasedParticipantManagerComponent.h` | covered in UTurnBasedParticipantManagerComponent |
| Enums (`EMatchPhase`, `ETurnPhase`, `ETurnEndReason`, `EParticipantType`, `ETurnBasedActionState`, `ETurnModifierType`) | UENUM ×6 | Top-level | primary | `TurnBasedMechanicsEnums.h` | stub |
| Structs (`FTurnParticipantInfo`, `FTurnStartContext`, `FTurnNotification`, `FTurnBasedActionRecord`, `FTurnActionRequest`, `FTurnModifier`) | USTRUCT ×6 | Top-level | primary | `TurnBasedMechanicsStructs.h` | stub |
| UTurnBasedMechanicsLibrary | UCLASS (UBlueprintFunctionLibrary) | Top-level | internal | `TurnBasedMechanicsLibrary.{h,cpp}` | stub |
| UDWidget_TurnBasedActionsComponent / …ParticipantManagerComponent | UCLASS (abstract) | Debug | internal (tooling) | `Debug/DWidget_TurnBased*.h` | stub |
| FUnrealTurnBasedMechanicsModule | class (IModuleInterface) | Top-level | internal | `UnrealTurnBasedMechanics.h` | stub |

`TurnBasedMechanicsDelegates.h` holds two native-only multicast delegates
(`FOnTurnNotification_Native`, `FOnOpponentTurnStarted_Native`) reused as the `_Native`
siblings on `UTurnBasedParticipantComponent` — not a type, no page.

## Map

`Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/` — one Runtime module
(`LogTurnBasedMechanics`). Sub-areas:

- **`Action/`** — the per-controller **action stack**
  ([[UnrealTurnBasedMechanics/code/UTurnBasedActionsComponent|UTurnBasedActionsComponent]])
  fed from a
  [[UnrealTurnBasedMechanics/code/UActionLoadoutDataAsset|UActionLoadoutDataAsset]] with
  five system slots. `UTurnBasedActionBase` → `UTurnBasedAction` (per-turn, with the grid
  hover→select pipeline + board-change confirmation gate) / `UTurnBasedSpectatorAction`
  (passive views). See
  [[UnrealTurnBasedMechanics/code/systems/action-stack-lifecycle|systems/action-stack-lifecycle]].
- **`Turn/Participant/`** — the replicated match state machine
  ([[UnrealTurnBasedMechanics/code/UTurnBasedParticipantManagerComponent|…ManagerComponent]]
  on the GameState): ready-check, timers, forfeit/reconnect, and the **tag-gated turn-end
  sequence**. See
  [[UnrealTurnBasedMechanics/code/systems/turn-end-tag-gate|systems/turn-end-tag-gate]].
- **`Turn/Order/`** — pluggable [[UnrealTurnBasedMechanics/code/ITurnOrderInterface|ITurnOrderInterface]]
  strategy (default sequential).
- **`Framework/`** — `ATurnBased*` GameMode/State/PlayerState/Controller bases; the
  coordinator component wires participant + actions components identically onto player and
  AI controllers.
- **`GameEvent/` (`UTurnBasedGameEvent`), `Debug/`** — extension point + debug widgets.

**Start at:** `UTurnBasedActionsComponent` → the two `systems/` flows.

## Related

- Flows: [[UnrealTurnBasedMechanics/code/systems/__INDEX|code/systems/__INDEX.md]]
- Recipes: [[UnrealTurnBasedMechanics/code/recipes/__INDEX|code/recipes/__INDEX.md]]
