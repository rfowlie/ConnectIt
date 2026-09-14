---
schema: code
reconciled: 2026-09-14
commit: efc8052
---

# ConnectIt (game module) — code index

Per-type help pages for the `ConnectIt` game module (`Source/ConnectIt/`). Governed by
[[_core/_schema/_code|_core/_schema/_code.md]]. This is the **overlay** — short "what /
why / what changed" notes over the code. The exhaustive per-member reference is the
code itself (`../../Source/ConnectIt/`).

Source root: `Source/ConnectIt/`. Anchor commit `efc8052` = *Implement SWAP*, the last
commit to touch this module (`/process-code` re-ran 2026-09-14 — 4 pages refreshed, see
their own `## Changes`; the SWAP action work is a newer phase layered on top of the
board-architecture work below, not yet folded into this paragraph's own framing). The
game is mid–**board-architecture overhaul**:
the `AConnectIt_BoardManager` actor is retired, replaced by server-only UObjects on
[[AConnectIt_GameMode|AConnectIt_GameMode]]; the tile/piece registries have moved again,
onto [[UConnectIt_BoardRegistrySubsystem|UConnectIt_BoardRegistrySubsystem]]; several
game-event bodies are commented out pending rewiring. `status`: **current** = written &
reconciled · **stub** = listed, no page yet.

## Inventory

| Type | Kind | Area | Role | Source (`Public/…`) | Status |
|---|---|---|---|---|---|
| [[AConnectIt_GameMode\|AConnectIt_GameMode]] (`EConnectItMatchType`) | UCLASS (AGameMode) | Framework/GameMode | primary (server-only owner) | `Framework/GameMode/ConnectIt_GameMode.{h,cpp}` | current |
| [[AConnectIt_GameState\|AConnectIt_GameState]] (`FConnectItMatchResult`, `EMatchEndReason`) | UCLASS (AGameState) | Framework/GameState | primary | `Framework/GameState/ConnectIt_GameState.{h,cpp}` | current |
| [[UConnectIt_BoardStateComponent\|UConnectIt_BoardStateComponent]] | UCLASS (component) | Board | primary (single source of truth) | `Board/ConnectIt_BoardStateComponent.{h,cpp}` | current |
| [[UConnectIt_BoardRequestMediator\|UConnectIt_BoardRequestMediator]] | UCLASS (UObject) | Board | primary (server-only) | `Board/ConnectIt_BoardRequestMediator.{h,cpp}` | current |
| [[UConnectIt_BoardRules\|UConnectIt_BoardRules]] (`IConnectIt_ScoringRule`, `IConnectIt_WinCondition`, `UConnectIt_LineScoringRule`, `UConnectIt_ScoreThresholdWinCondition`) | UCLASS / UINTERFACE | Board/Rules | primary (pluggable strategies) | `Board/Rules/*.{h,cpp}` | current |
| [[UConnectIt_BoardRegistrySubsystem\|UConnectIt_BoardRegistrySubsystem]] (`UConnectIt_TileRegistry`, `UConnectIt_PieceRegistry`) | UCLASS (UWorldSubsystem) | Framework/Subsystem, Board | primary (per-world registry owner) | `Framework/Subsystem/ConnectIt_BoardRegistrySubsystem.{h,cpp}`, `Board/ConnectIt_TileRegistry.{h,cpp}`, `Board/ConnectIt_PieceRegistry.{h,cpp}` | current |
| [[ConnectItStructs\|ConnectItStructs]] (`FConnectItBoardState`, `FConnectItTileData`, `FConnectItBoardChangeEvent`, `FConnectItBoardStateSnapshot`, `FConnectItRequest*` payloads) | USTRUCT ×~12 | Top-level | primary (data) | `ConnectIt_Structs.h` | current |
| [[UConnectIt_State_Game\|UConnectIt_State_Game]] (`UConnectIt_State_Base`, `…_SelectTile`, `…_PlacePiece`, `…_UpdateGameBoard`) | UCLASS | Framework/Game State Machine | **retired legacy** — do not build on | `Framework/Game State Machine/*.{h,cpp}` | current (documented as legacy) |
| [[UConnectIt_GameFacade\|UConnectIt_GameFacade]] (`UConnectIt_GameViewModel`) | UCLASS (UObject) | Framework/Data | **retired legacy** — superseded by `UConnectIt_GameUtilityLibrary` | `Framework/Data/ConnectIt_GameFacade.{h,cpp}`, `.../ConnectIt_GameViewModel.{h,cpp}` | current (documented as legacy) |
| AConnectIt_PlayerController | UCLASS (APlayerController) | Framework/Controller | primary | `Framework/Controller/ConnectIt_PlayerController.{h,cpp}` | stub — covered in systems/place-piece-request |
| AConnectIt_AIController | UCLASS (AAIController) | Framework/Controller | primary | `Framework/Controller/ConnectIt_AIController.{h,cpp}` | stub |
| AConnectIt_GameMode_Play | UCLASS (AGameModeBase) | Framework/GameMode | **retired legacy** — empty body, implements the two dead facade interfaces | `Framework/GameMode/ConnectIt_GameMode_Play.h` | stub |
| AConnectIt_PlayerState | UCLASS | Framework/PlayerState | primary | `Framework/PlayerState/ConnectIt_PlayerState.h` | stub |
| UConnectIt_PlacePieceAction / UConnectIt_DefaultViewerAction | UCLASS (`UTurnBasedAction` / `…SpectatorAction`) | Action | primary | `Action/ConnectIt_PlacePieceAction.{h,cpp}`, `Action/ConnectIt_DefaultViewerAction.h` | stub — covered in systems/place-piece-request |
| UConnectIt_PlacePieceGameEvent / UConnectIt_LineScoreGameEvent | UCLASS (`UTurnBasedGameEvent`) | GameEvent | primary (mid-rewrite — bodies stubbed) | `GameEvent/ConnectIt_*GameEvent.{h,cpp}` | stub |
| UConnectIt_PieceRegistryComponent | UCLASS (component) | Board | primary | `Board/ConnectIt_PieceRegistryComponent.{h,cpp}` | stub |
| UConnectIt_BlackboardSubsystem | UCLASS (UWorldSubsystem) | Framework/Subsystem | primary (turn modifiers) | `Framework/Subsystem/ConnectIt_BlackboardSubsystem.{h,cpp}` | stub |
| UConnectIt_LevelConfigDataAsset / UConnectIt_LevelConfigSettings / UConnectIt_PlayerData | UCLASS (UDataAsset / config / UObject) | Framework/Data | primary (config); `UConnectIt_PlayerData` is **retired legacy** | `Framework/Data/ConnectIt_LevelConfig*.h`, `.../ConnectIt_PlayerData.h` | stub |
| Libraries (`UConnectIt_BoardStateLibrary`, `…_GameRulesLibrary`, `…_GameUtilityLibrary`, `…_GridFloodLibrary`, `…_GridRulesLibrary`) | UCLASS (UBlueprintFunctionLibrary) | Framework/Library | primary | `Framework/Library/ConnectIt_*Library.{h,cpp}` | stub |
| AConnectIt_GridPiece | UCLASS (`AGridPieceBase`) | Grid | primary | `Grid/ConnectIt_GridPiece.{h,cpp}` | stub |
| Influence map (`UPieceControlInfluenceMap`, `UPieceControlMapVisualizer`) | UCLASS | GameIntelligence/InfluenceMap | primary | `GameIntelligence/InfluenceMap/*.{h,cpp}` | stub |
| MinMax (`UConnectIt_MinMaxManager`, `UConnectIt_MinMaxTreeBuilder`, `ConcreteMinMaxExample`) | UCLASS / templates | MinMax | primary (game's own AI search) | `MinMax/*.{h,cpp}` | stub |
| UI (`UConnectIt_DebugStateWidget`, `UDWidget_ConnectIt_BoardStateComponent`, `UDWidget_ConnectIt_GameState`) | UCLASS (UUserWidget / `UDWidgetBase`) | UI | internal (tooling) | `UI/*.{h,cpp}` | stub |
| Interfaces (`IConnectIt_FacadeHandlerInterface`, `IConnectIt_GameStateHandlerInterface`) | UINTERFACE | Framework/Interface | **retired legacy** — only `AConnectIt_GameMode_Play` implements them | `Framework/Interface/*.h` | stub |
| GameplayTags (`ConnectIt_GameplayTags.h`) | native tag decls | Top-level | primary | `ConnectIt_GameplayTags.h` | stub |

## Map

The suite, seen from the game domain. ConnectIt is a **layered stack of game-agnostic
plugins with the game module on top** — each plugin is reusable in another project and
knows nothing about ConnectIt's rules. The game module (`Source/ConnectIt/`) is the only
place game-specific logic lives.

### Dependency graph

```
                 ConnectIt  (game module — Source/ConnectIt)
                    |
   +----------------+----------------+----------------+
   |                |                |                |
UnrealTurnBasedMechanics   UnrealGameIntelligence   UnrealCodingUtils
   |        |                (engine-only)            (engine-only)
   |        +--------> UnrealGridMechanics
   |                        |
   +------------------------+--------> UnrealGameMechanics  (base)
                                          ^
                                          |  (EnhancedInput)

UnrealAIMechanics  — standalone, DORMANT (not enabled)
UnrealUIMechanics  — standalone, STUB   (not enabled)
AdvancedSessions-5-5 — vendored third-party, not integrated
```

- **[[UnrealGameMechanics/CLAUDE|UnrealGameMechanics]]** — base layer: the gated
  phase-barrier / tag queue (`UGameEventTaskSubsystem`), turn bookkeeping, actor pooling,
  Enhanced-Input tag binder, scoring / state-machine helpers, `UDWidgetBase`.
- **[[UnrealGridMechanics/CLAUDE|UnrealGridMechanics]]** — depends on GameMechanics; adds
  the 2D board: coords + directions, grid/shape math, tile/piece actors, registries,
  hover subsystems, replicated board state, row/col shift, editor validator.
- **[[UnrealTurnBasedMechanics/CLAUDE|UnrealTurnBasedMechanics]]** — depends on both;
  adds the replicated match: per-controller action stack,
  `UTurnBasedParticipantManagerComponent` state machine (ready-check, timers,
  forfeit/reconnect), pluggable turn order, `ATurnBased*` framework bases. Its turn-end
  sequence routes through GameMechanics' `UGameEventTaskSubsystem`.
- **[[UnrealGameIntelligence/CLAUDE|UnrealGameIntelligence]]**,
  **[[UnrealCodingUtils/CLAUDE|UnrealCodingUtils]]** — standalone, engine-only deps.
  ConnectIt uses only GameIntelligence's influence-map / utility-score **debug**
  interfaces; the plugin's MinMax templates are unused (the game carries its own).
- **[[UnrealAIMechanics/CLAUDE|UnrealAIMechanics]]**,
  **[[UnrealUIMechanics/CLAUDE|UnrealUIMechanics]]** — not enabled in `ConnectIt.uproject`
  (dormant / stub). `AdvancedSessions-5-5` — vendored, its `OnlineSubsystem` dependency
  commented out pending online-session work.

### Where to start reading

1. **Board authority** — [[AConnectIt_GameMode|AConnectIt_GameMode]] owns
   [[UConnectIt_BoardRequestMediator|UConnectIt_BoardRequestMediator]] +
   [[UConnectIt_BoardRules|UConnectIt_BoardRules]] (server-only). All board mutation
   enters through `AConnectIt_GameMode::ProcessBoardRequest`.
2. **Board truth** — [[UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]] on
   the GameState holds one replicated snapshot (prev + current + change event) that every
   visual system converges on: [[board-state-single-source-of-truth|systems/board-state-single-source-of-truth]].
3. **A turn** — [[place-piece-request|systems/place-piece-request]]: player picks a tile →
   server validates → board state commits → gated visual sequence → outcome back to client.
4. **Registries** — [[UConnectIt_BoardRegistrySubsystem|UConnectIt_BoardRegistrySubsystem]]:
   one per world, `DuplicateObject`'d at `OnWorldBeginPlay` from `Instanced` templates on
   `UConnectIt_LevelConfigDataAsset`.
5. **Cross-cutting conventions** — gameplay tags drive sequencing
   (`../../Config/DefaultGameplayTags.ini`); `UnrealTurnBasedMechanics` carries
   `FInstancedStruct Payload` on its request/modifier structs so the game module can
   extend turn data without changing the plugin; no `Client_`/`Multicast_` RPCs exist in
   `Source/ConnectIt` — every "tell everyone" need is a replicated-property change
   instead. See [[ConnectIt/_decisions/2026-09-06-suite-conventions|decisions/2026-09-06-suite-conventions]].

### Do not document as live

The **legacy MVVM pipeline** — [[UConnectIt_State_Game|UConnectIt_State_Game]] + its four
sub-states, [[UConnectIt_GameFacade|UConnectIt_GameFacade]] / `UConnectIt_GameViewModel`,
`UConnectIt_PlayerData`, `AConnectIt_GameMode_Play`, and the two `IConnectIt_*Handler`
interfaces — is **unreferenced dead code**, superseded by the replicated board pipeline.
Kept in the tree, not built on. See
[[ConnectIt/_decisions/2026-09-08-retire-legacy-mvvm-pipeline|decisions/2026-09-08-retire-legacy-mvvm-pipeline]].
Also retired: `AConnectIt_BoardManager` and its subsystem, the whole ConnectIt shift
pipeline, the tag-reactive interpreter (its game-event-queue replacement on the mediator
is an empty stub).

## Known issues

Structural overlap and confirmed defects surfaced while writing the in-repo docs (now
folded here). Full evidence/hypothesis/recommendation for the structural set lived in
`Source/ConnectIt/Docs/Duplication.md`.

### Confirmed correctness bugs

- **FactionID spoof** — `AConnectIt_PlayerController::ServerRouteBoardChangeRequest_Implementation`
  only stamps the server-authoritative `FactionID` when the client-sent value is
  negative/unset. Every current action sends a valid `FactionID`, so the server today
  trusts whatever a client sends as long as it's `>= 0`, checking only that it's *that
  participant's turn* — not that the `FactionID` belongs to them. A modified client could
  submit another faction's ID. Fix: stamp `FactionID` from the server-side participant
  unconditionally.
- **Influence map reads a never-populated field** —
  `PieceControlInfluenceMap::Update()` scores neighbouring tiles by comparing against
  `Snapshot.FactionTurn`. `FConnectItBoardState::FactionTurn` defaults to `-1` and is set
  only in MinMax/AI hypothetical-state code, never on the live board — where `-1` is also
  the empty-tile sentinel. Net effect: every empty tile scores as the active faction's,
  every occupied tile as opposing. Needs `FactionTurn` populated from the real active
  participant before the comparison means anything.

### Structural overlap / unfinished refactors

- **Two parallel MinMax implementations** — `UConnectIt_MinMaxManager` (namespaced
  `ConnectIt::FMinMaxNode`, synchronous) vs the more complete `UConnectIt_MinMaxTreeBuilder`
  (global `FConnectItMinMaxNode`, async build/solve, `FCriticalSection`), no shared base;
  plus a fully commented-out `ConcreteMinMaxExample.h` that tried to route through
  UnrealGameIntelligence's template solvers. Neither is called by `AConnectIt_AIController`
  (its `BeginMakeDecision()` is a Blueprint stub). Pick one, delete the other.
- **`AConnectIt_GridPiece` dead replication setup** — replicates `FactionID` /
  `OccupiedPosition` and sets `bReplicates = true`, but nothing spawns it server-side any
  more (the tag-reactive interpreter that did is removed; its replacement doesn't spawn
  pieces yet). Re-confirm which side spawns it once the replacement pipeline works, then
  strip or justify the scaffolding.
- **Parked scaffolds** — `AConnectIt_GameMode_Play` (empty `AGameModeBase`, implements the
  two dead facade interfaces; the live `CI_GameMode_Play` Blueprint reparents to
  `AConnectIt_GameMode`, not this) and the whole `UnrealUIMechanics` plugin (zero classes,
  not enabled). Owner decision: delete or keep parked with a stated reason.
- Unprefixed classes: `UPieceControlInfluenceMap` / `UPieceControlMapVisualizer` carry
  only the `U` prefix, no `ConnectIt_` — the only two live ConnectIt classes without it.
- See also [[UnrealGridMechanics/CLAUDE|UnrealGridMechanics]] /
  [[UnrealGameMechanics/CLAUDE|UnrealGameMechanics]] rough edges for the plugin-side
  overlap entries (`UGridTrackerSubsystem` vs `UGridHoverSubsystem`;
  `UGameMechanicsStateBase` vs `…Simple`; `IGridTileSelector` deprecating
  `UGridTileSelectorComponent`; dead commented-out blocks).

## Related

- Reference: [[ConnectIt/code/runtime-state|runtime-state.md]] — which accessor to call to
  read live board / turn / action / sequencing state
- Flows: [[ConnectIt/code/systems/__INDEX|systems/__INDEX.md]]
- Recipes: [[ConnectIt/code/recipes/__INDEX|recipes/__INDEX.md]]
- Decisions: [[ConnectIt/_decisions/__INDEX|decisions/__INDEX.md]]
- Domain router: [[ConnectIt/CLAUDE|ConnectIt/CLAUDE.md]]
