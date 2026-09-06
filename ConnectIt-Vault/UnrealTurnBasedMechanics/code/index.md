---
schema: code
reconciled: 2026-09-06
commit: 452688c
---

# UnrealTurnBasedMechanics — code index

Per-type help pages for the plugin's public API. Governed by [[_schema/code|_schema/code.md]].
These are an **overlay** on the authoritative in-repo reference
([`Docs/README.md`](../../../Plugins/UnrealTurnBasedMechanics/Docs/README.md),
[`Docs/Systems.md`](../../../Plugins/UnrealTurnBasedMechanics/Docs/Systems.md)) — entry
points, collaborators, gotchas, cross-impact, freshness. They do not restate it.

Source root: `Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/`
Provenance: pages are anchored to a commit; refresh with
`git log <source> <commit>..HEAD`.

`status`: **current** = page written & reconciled · **stub** = no page yet · **stale** = drift detected.

| Type | Kind | Area | Role | Source (`Public/…`, `Private/…`) | Status |
|---|---|---|---|---|---|
| [[UnrealTurnBasedMechanics/code/UTurnBasedActionsComponent\|UTurnBasedActionsComponent]] | UCLASS (component) | Action | primary | `Action/TurnBasedActionsComponent.{h,cpp}` | current @ 452688c |
| [[UnrealTurnBasedMechanics/code/UTurnBasedAction\|UTurnBasedAction]] | UCLASS (abstract) | Action | primary | `Action/TurnBasedAction.{h,cpp}` | current @ 452688c |
| [[UnrealTurnBasedMechanics/code/UTurnBasedActionBase\|UTurnBasedActionBase]] | UCLASS (abstract) | Action | primary | `Action/TurnBasedActionBase.{h,cpp}` | current @ 452688c |
| [[UnrealTurnBasedMechanics/code/UActionLoadoutDataAsset\|UActionLoadoutDataAsset]] | UCLASS (UDataAsset) | Action | primary | `Action/ActionLoadoutDataAsset.{h,cpp}` | current @ 452688c |
| [[UnrealTurnBasedMechanics/code/UTurnBasedParticipantManagerComponent\|UTurnBasedParticipantManagerComponent]] | UCLASS (component) | Turn/Participant | primary (core) | `Turn/Participant/TurnBasedParticipantManagerComponent.{h,cpp}` | current @ 452688c |
| [[UnrealTurnBasedMechanics/code/UTurnBasedParticipantComponent\|UTurnBasedParticipantComponent]] | UCLASS (component) | Turn/Participant | primary | `Turn/Participant/TurnBasedParticipantComponent.{h,cpp}` | current @ 452688c |
| [[UnrealTurnBasedMechanics/code/ITurnOrderInterface\|ITurnOrderInterface]] | UINTERFACE | Turn/Order | primary (contract) | `Turn/Order/TurnOrderInterface.{h,cpp}` | current @ 452688c |
| UTurnBasedSpectatorAction | UCLASS (abstract) | Action | primary | `Action/TurnBasedSpectatorAction.{h,cpp}` | stub |
| FTurnActionSnapshot / FTurnBasedActionsComponentInfo | USTRUCT | Action | internal (debug) | `Action/TurnBasedActionsComponent.h` | covered in UTurnBasedActionsComponent |
| UTurnBasedPauseAction | UCLASS | Action/Spectator | primary (default) | `Action/Spectator/TurnBasedPauseAction.{h,cpp}` | stub |
| ATurnBasedGameMode | UCLASS | Framework/GameMode | primary | `Framework/GameMode/TurnBasedGameMode.{h,cpp}` | stub |
| ATurnBasedGameState | UCLASS | Framework/GameState | primary | `Framework/GameState/TurnBasedGameState.{h,cpp}` | stub |
| ATurnBasedPlayerState | UCLASS | Framework/PlayerState | primary (thin) | `Framework/PlayerState/TurnBasedPlayerState.{h,cpp}` | stub |
| ATurnBasedPlayerControllerBase | UCLASS (abstract) | Framework/Controller | primary | `Framework/Controller/TurnBasedPlayerControllerBase.{h,cpp}` | stub |
| ATurnBasedAIController | UCLASS (abstract) | Framework/Controller | primary | `Framework/Controller/TurnBasedAIController.{h,cpp}` | stub |
| UTurnBasedControllerCoordinatorComponent | UCLASS (component) | Framework/Controller | primary (plumbing) | `Framework/Controller/TurnBasedControllerCoordinatorComponent.{h,cpp}` | stub |
| UTurnBasedGameEvent | UCLASS (abstract) | GameEvent | primary (extension point) | `GameEvent/TurnBasedGameEvent.{h,cpp}` | stub |
| USequentialTurnOrderStrategy | UCLASS | Turn/Order | primary (default) | `Turn/Order/SequentialTurnOrderStrategy.{h,cpp}` | stub |
| URandomTurnOrderStrategy | UCLASS | Turn/Order | internal (alt) | `Turn/Order/RandomTurnOrderStrategy.{h,cpp}` | stub |
| FTurnBasedParticipantManagerInfo | USTRUCT | Turn/Participant | internal (debug) | `Turn/Participant/TurnBasedParticipantManagerComponent.h` | covered in UTurnBasedParticipantManagerComponent |
| Enums (`EMatchPhase`, `ETurnPhase`, `ETurnEndReason`, `EParticipantType`, `ETurnBasedActionState`, `ETurnModifierType`) | UENUM ×6 | Top-level | primary | `TurnBasedMechanicsEnums.h` | stub → `Enums.md` |
| Structs (`FTurnParticipantInfo`, `FTurnStartContext`, `FTurnNotification`, `FTurnBasedActionRecord`, `FTurnActionRequest`, `FTurnModifier`) | USTRUCT ×6 | Top-level | primary | `TurnBasedMechanicsStructs.h` | stub → `Structs.md` |
| UTurnBasedMechanicsLibrary | UCLASS (UBlueprintFunctionLibrary) | Top-level | internal | `TurnBasedMechanicsLibrary.{h,cpp}` | stub |
| UDWidget_TurnBasedActionsComponent | UCLASS (abstract) | Debug | internal (tooling) | `Debug/DWidget_TurnBasedActionsComponent.h`, `Private/Debug/…cpp` | stub |
| UDWidget_TurnBasedParticipantManagerComponent | UCLASS (abstract) | Debug | internal (tooling) | `Debug/DWidget_TurnBasedParticipantManagerComponent.h`, `Private/Debug/…cpp` | stub |
| FUnrealTurnBasedMechanicsModule | class (IModuleInterface) | Top-level | internal | `UnrealTurnBasedMechanics.h`, `Private/UnrealTurnBasedMechanics.cpp` | stub |

`TurnBasedMechanicsDelegates.h` holds two native-only multicast delegates
(`FOnTurnNotification_Native`, `FOnOpponentTurnStarted_Native`) reused as the `_Native`
siblings on `UTurnBasedParticipantComponent` — not a type, no page.

## Related

- Flows: [[UnrealTurnBasedMechanics/systems/index|systems/index.md]]
- Recipes: [[UnrealTurnBasedMechanics/recipes/index|recipes/index.md]]
