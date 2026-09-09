# UnrealGameMechanics — high level

## Purpose

The reusable gameplay-scaffolding backbone the rest of the plugin suite builds on. It
has no game-specific rules. Its most-developed system is a gated **phase-barrier**:
`UGameEventTaskManager` tracks a set of tasks that must all finish before it fires
`OnManagerComplete`, and `UGameEventTaskSubsystem` (a `UWorldSubsystem`) layers a
per-gameplay-tag registry plus a serialized FIFO queue (`QueueTagContainer`) on top — so
callers fire tag-groups that run in parallel and only advance when every task in the
group completes. Around that it provides turn bookkeeping, an actor object pool, an
explicit-lifetime Enhanced Input tag binder, generic scoring/selection helpers, two
state-machine patterns, a tag-broadcast component, and a debug-widget base.

## Status

Active — enabled in `ConnectIt.uproject`. The base dependency for
[[../UnrealGridMechanics/high-level|UnrealGridMechanics]] and
[[../UnrealTurnBasedMechanics/high-level|UnrealTurnBasedMechanics]].

## Key modules

- `UnrealGameMechanics` (Runtime). Sub-areas: `GameEvent/`, `GameTurn/`, `Pooling/`,
  `Input/`, `Scoring/`, `State/`, `Broadcasters/`, `Widget/`, `Debug/`.
- Plugin dependency: `EnhancedInput`.

## Public API surface

- **GameEvent:** `UGameEventTaskSubsystem` (`QueueTagContainer`, `RegisterAsyncTask`,
  `BindOnTagBegin` / `BindOnTagComplete`, `GetTagsInQueue`); `UGameEventTaskManager`
  (`Create`, `RegisterTask` / `RegisterAsyncTask`, `InitiateAllTasks`, `OnManagerBegin` /
  `OnManagerComplete`).
- **Turns:** `UGameTurnHandler`, `UGameTurnTracker`, `UGameTurnParticipant`.
- **Pooling:** `UActorPool`, `UActorPoolSubsystem`, `IActorPoolInterface`.
- **Input:** `UInputTagBinder` (`Initialise`, `BindAll` / `UnbindAll`,
  `OnInputTagTriggered`) driven by data-only `FInputTagBinding` entries.
- **State machines:** `UGameMechanicsStateBase` / `...Simple` (single state);
  `UStackedState` / `UStackedStateMachine` (`SetBaseState` / `PushState` / `PopState` /
  `PeakState`).
- **Scoring:** `UGameMechanics_ScoringLibrary` (`GetSortedKeysByValue<K,V>`,
  `ScoreSelect_TopN`, `ScoreModify_RandomNoise`).
- **Misc:** `IGameplayTagBroadcaster` / `UGameplayTagBroadcasterComponent`;
  `UGameMechanics_Framework` (`GetGameInstance` / `GetGameMode`); `EGamePlayerType`;
  `UDWidgetBase` (debug-widget base).

## Consumers

`UnrealGridMechanics`, `UnrealTurnBasedMechanics`, and the `ConnectIt` game module.
The turn-end sequence in `UnrealTurnBasedMechanics` routes through
`UGameEventTaskSubsystem`.

## In-repo reference

- [`../../Plugins/UnrealGameMechanics/Docs/README.md`](../../Plugins/UnrealGameMechanics/Docs/README.md) — exhaustive per-class reference
- [`../../Plugins/UnrealGameMechanics/Docs/Systems.md`](../../Plugins/UnrealGameMechanics/Docs/Systems.md) — narrative walkthrough + conventions + rough edges

## Vault code docs

Overlay on the in-repo docs: per-type entry-point/gotcha pages, flow diagrams, task
recipes, git-anchored for freshness. Partial (see each index for status).

- [[UnrealGameMechanics/code/index|code/index.md]] ([[_schema/code|schema]])
- [[UnrealGameMechanics/systems/index|systems/index.md]] ([[_schema/systems|schema]])
- [[UnrealGameMechanics/recipes/index|recipes/index.md]] ([[_schema/recipes|schema]])

## Known rough edges

- `BlueprintreadWrite` casing typo in `GameTurnHandler.h`.
- Orphaned `IGameEventTaskHandler`.
- Near-duplicate `UGameTurnParticipant` vs `UGameTurnParticipantComponent`.
- `SwapState()` is an unimplemented stub.
- Inconsistent `Category` strings across UFUNCTIONs/UPROPERTYs.

_Verify against the in-repo `Docs/` before acting on this list — it reflects a past snapshot._
