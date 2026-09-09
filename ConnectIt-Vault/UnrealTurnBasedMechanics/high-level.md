# UnrealTurnBasedMechanics — high level

## Purpose

A full network-replicated turn-based match framework with no knowledge of grids or game
rules. Each controller's moment-to-moment behaviour is an entry on an **action stack**
(`UTurnBasedActionsComponent`) fed from a designer `UActionLoadoutDataAsset` with five
system slots (root / idle / spectator / pause / awaiting-confirmation). `UTurnBasedAction`
is the per-turn action, with cooldowns and an Enhanced-Input grid-tile
hover -> validate -> select pipeline. `UTurnBasedParticipantManagerComponent` (on the game
state) is the replicated match state machine: ready-check, turn timers,
forfeit-on-disconnect, reconnect handling, and a gameplay-tag-gated turn-end sequence that
routes through `UnrealGameMechanics`' `UGameEventTaskSubsystem`. Turn order is a pluggable
strategy. A coordinator component wires the participant and actions components identically
onto both the player controller and the AI controller.

## Status

Active — enabled in `ConnectIt.uproject`. The best-documented plugin in the project.
Depends on [[../UnrealGameMechanics/high-level|UnrealGameMechanics]] and
[[../UnrealGridMechanics/high-level|UnrealGridMechanics]].

## Key modules

- `UnrealTurnBasedMechanics` (Runtime). Sub-areas: `Action/`, `Action/Spectator/`,
  `Framework/` (Controller / GameMode / GameState / PlayerState), `Turn/Order/`,
  `Turn/Participant/`, `GameEvent/`, `Debug/`.
- Own log category: `LogTurnBasedMechanics`.
- Plugin dependencies: `EnhancedInput`, `UnrealGameMechanics`, `UnrealGridMechanics`.
- Convention: dual dynamic + `_Native` delegate declarations throughout.

## Public API surface

- **Actions:** `UTurnBasedActionBase`, `UTurnBasedAction`, `UTurnBasedSpectatorAction`,
  `UTurnBasedPauseAction`, `UTurnBasedActionsComponent` (~25 UFUNCTIONs incl. `PushAction`,
  `SafePopAction`, `ClearAndPush`, `RequestTurnEnd`, `NotifyBoardChangeOutcome`,
  `IsAwaitingRequestConfirmation`), `UActionLoadoutDataAsset`.
- **Framework actors:** `ATurnBasedGameMode`, `ATurnBasedGameState`,
  `ATurnBasedPlayerState`, `ATurnBasedPlayerControllerBase` (implements
  `IGridFactionInterface`), `ATurnBasedAIController`,
  `UTurnBasedControllerCoordinatorComponent`.
- **Turn:** `UTurnBasedParticipantComponent` (`ServerNotifyReady`, `ServerSubmitTurnEnd`,
  `IsMyTurn`), `UTurnBasedParticipantManagerComponent`, `ITurnOrderInterface` with
  `USequentialTurnOrderStrategy` (default) and `URandomTurnOrderStrategy`.
- **Data:** `FTurnActionRequest` / `FTurnModifier` (carry `FInstancedStruct Payload` — the
  project-extension hook); enums `EMatchPhase`, `ETurnPhase`, `ETurnEndReason`,
  `EParticipantType`.
- **Debug:** `UDWidget_TurnBasedActionsComponent`,
  `UDWidget_TurnBasedParticipantManagerComponent`.

## Consumers

The `ConnectIt` game module (its GameMode / GameState / controllers derive from the
`ATurnBased*` bases).

## In-repo reference

- [`../../old/Plugins/UnrealTurnBasedMechanics/Docs/README.md`](../../old/Plugins/UnrealTurnBasedMechanics/Docs/README.md) — authoritative per-class reference
- [`../../old/Plugins/UnrealTurnBasedMechanics/Docs/Systems.md`](../../old/Plugins/UnrealTurnBasedMechanics/Docs/Systems.md) — authoritative systems narrative + conventions

## Vault code docs

This section has adopted the code-ingestion schemas — an overlay on the in-repo docs
above adding per-type entry-point/gotcha pages, flow diagrams, and task recipes, each
git-anchored for freshness. Ingest is partial (see each index for status).

- [[UnrealTurnBasedMechanics/code/index|code/index.md]] — per-type help pages ([[_schema/code|schema]])
- [[UnrealTurnBasedMechanics/systems/index|systems/index.md]] — per-flow narratives with diagrams ([[_schema/systems|schema]])
- [[UnrealTurnBasedMechanics/recipes/index|recipes/index.md]] — step-by-step task procedures ([[_schema/recipes|schema]])

## Known rough edges

- `.Build.cs` lists grid / game-mechanics in both Public and Private dependency lists
  (redundant).
- Likely-broken source resolution in
  `UDWidget_TurnBasedParticipantManagerComponent::BindDelegates`.
- Retired `BeginResolutionHold` / `EndResolutionHold` API still referenced in places.

_Verify against the in-repo `old/Plugins/UnrealTurnBasedMechanics/Docs/` before acting on this list — it reflects a past snapshot._
