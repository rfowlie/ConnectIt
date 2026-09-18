---
schema: code
kind: UCLASS
role: primary
source:
  - Source/ConnectIt/Public/Framework/Subsystem/ConnectIt_BoardRegistrySubsystem.h
  - Source/ConnectIt/Private/Framework/Subsystem/ConnectIt_BoardRegistrySubsystem.cpp
  - Source/ConnectIt/Public/Board/ConnectIt_TileRegistry.h
  - Source/ConnectIt/Public/Board/ConnectIt_PieceRegistry.h
reconciled: 2026-09-10
commit: 6477d5d
---

# UConnectIt_BoardRegistrySubsystem (+ UConnectIt_TileRegistry / UConnectIt_PieceRegistry)

The single per-world owner of the board's tile and piece registries. `UWorldSubsystem` —
every machine's own world (server and each client) resolves its own instance
independently, matching the project's symmetric-resolution philosophy for level-authored
data. Replaces the previous home of these registries: per-machine properties on
`AConnectIt_PlayerController`.

## When you touch this

- Any code that needs `TileRegistry` / `PieceRegistry` — go through
  `UConnectIt_GameUtilityLibrary::GetTileRegistry()` / `GetPieceRegistry()`, which now
  just forward to this subsystem.
- Adding board-state-aware behaviour on a registry (put it on the `UConnectIt_*Registry`
  subclass — see below).
- Debugging "registry query returns null" (almost always missing level-config setup —
  see Gotchas).

## Entry points

- `OnWorldBeginPlay(UWorld&)` — **not `Initialize()`**: this hook is Epic's documented
  guarantee of running after every existing actor's `BeginPlay`. Resolves the level's
  `UConnectIt_LevelConfigDataAsset` (via `UConnectIt_GameUtilityLibrary::GetLevelConfig`)
  and `DuplicateObject()`s its own per-world runtime copy of each `Instanced` template —
  **`GridDefinition` first**, then `TileRegistry`/`PieceRegistry`, each wired with a
  reference to the duplicated `GridDefinition` before its own `InitialiseRegistry()` runs
  (`DiscoverExisting` needs it already assigned). Also binds `PieceRegistry` to
  [[UnrealGameMechanics/code/UGameEventTaskSubsystem\|UGameEventTaskSubsystem]]`::OnAnyTagComplete`
  here, once, so `PieceMap` stays in sync with board mutations.
- `Deinitialize()` — unbinds `PieceRegistry` from `OnAnyTagComplete`, then calls
  `ShutdownRegistry()` on both.
- `GetTileRegistry()` / `GetPieceRegistry()` / `GetGridDefinition()` (`BlueprintPure`) —
  the runtime duplicates, never the templates.
- `UConnectIt_TileRegistry::GetBoardState()` / `UConnectIt_PieceRegistry::GetBoardState()`
  (`BlueprintPure`) — on-demand `UConnectIt_BoardStateComponent*` resolution. Lives on
  the ConnectIt subclass, not the plugin's `UGridTileRegistryBase` /
  `UGridPieceRegistryBase`, which stay project-agnostic.
- `UConnectIt_PieceRegistry::HandleGameEventComplete(FGameplayTag)` — the
  `OnAnyTagComplete` handler; dispatches internally to the shift/swap/remove re-key on
  `PieceMap`. **Public**, not protected — `AddDynamic`/`RemoveDynamic` need compile-time
  access to `&UConnectIt_PieceRegistry::HandleGameEventComplete` from this (unrelated)
  subsystem class.

## Collaborators

- `UConnectIt_LevelConfigDataAsset` — carries the `Instanced` `TileRegistry` /
  `PieceRegistry` / **`GridDefinition`** **template** properties a designer picks per
  level (concrete-class polymorphism with inline sub-properties, matching the old
  `ABoardManagerBase` pattern — unlike the `TScriptInterface` `ScoringRule` /
  `WinConditionRule` on the same asset). `GridDefinition` (grid geometry — cell size,
  World↔Grid conversion) is a sibling template to the two registries, not a
  sub-property of either — see [[UnrealGridMechanics/code/UGridDefinition|UGridDefinition]].
- [[AConnectIt_GameMode|AConnectIt_GameMode]]`::InitialiseBoard` pulls both registries
  from the utility library and passes them into
  [[UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]]`::InitialiseBoardState`.
- `UGridTileRegistryBase::InitialiseRegistry()` does its own `TActorIterator` world scan +
  `AddOnActorSpawnedHandler` — it only needs `GetWorld()` to resolve via the Outer chain,
  which the `DuplicateObject(Template, this)` (this = subsystem, outered to the world)
  guarantees. Also needs `GridDefinition` already assigned (see Entry points) — otherwise
  discovery still populates `Tiles` but `TileMap` stays empty (logged as a warning).
- [[UnrealGameMechanics/code/UGameEventTaskSubsystem|UGameEventTaskSubsystem]]`::OnAnyTagComplete`
  — `PieceRegistry` is the only current listener; see
  [[ConnectIt/_decisions/2026-09-18-piece-registry-mapping-fix-re-keys-not-rebuilds|decisions/2026-09-18-piece-registry-mapping-fix-re-keys-not-rebuilds]]
  for why the handler re-keys from `FConnectItBoardChangeEvent` rather than rebuilding
  from actor transforms.

## Gotchas

- **The `DuplicateObject`-per-world step is load-bearing, not incidental.** A
  `UDataAsset`'s `Instanced` subobjects are shared, loaded-once objects. Using one
  directly as a live registry would mis-resolve `GetWorld()` and share mutable state
  across simultaneous worlds (editor + PIE, multiple PIE clients) — a real risk for a
  project that multi-client-tests via an `L_NetworkTest`-style setup.
- **Every registry query returns null until a level config is authored.** You must
  register a `UConnectIt_LevelConfigDataAsset` per level with `TileRegistry` set, in
  Project Settings → "ConnectIt Level Config". Without it the subsystem has nothing to
  duplicate.
- No injection into the `UConnectIt_*Registry` subclasses — `GetBoardState()` pulls on
  demand, relying on the same correct-`GetWorld()` precondition
  `UConnectIt_BoardRequestMediator::GetBoardState` already relies on.
- `OnWorldBeginPlay` timing means `AConnectIt_GameMode::InitialiseBoard` (off
  `HandleMatchHasStarted`) always finds the subsystem ready — the old
  controller-iteration scan could run before any controller connected.
- **Construction order matters now**: `GridDefinition` must be duplicated and assigned to
  both registries *before* either's `InitialiseRegistry()` runs. Reordering this in
  `OnWorldBeginPlay` silently reintroduces the null-`GridDefinition` warning path.
- `PieceRegistry->HandleGameEventComplete` reacts to *every* `GameEvent` tag completing
  (it's `OnAnyTagComplete`, not a tag-scoped `BindOnTagComplete`), and no-ops internally
  for anything it doesn't recognize — don't assume binding here means only board-shift/
  swap/remove tags ever reach it.

## Cross-impact

Adding a new registry field: the `Instanced` template property on
`UConnectIt_LevelConfigDataAsset`, the `TObjectPtr` + `DuplicateObject` + accessor here,
`ShutdownRegistry()` symmetry, and a `GetXRegistry()` forwarder on
`UConnectIt_GameUtilityLibrary`. If it needs grid geometry, wire it to the same shared
`GridDefinition` instance rather than adding another dependency on `TileRegistry`.

## Changes

- 2026-09-18 — **`GridDefinition` construction/wiring added** (duplicated before either
  registry, assigned to both); **`PieceRegistry` bound to `OnAnyTagComplete`** so its
  `PieceMap` stays in sync with shift/swap/remove (previously stale after enough board
  mutations — a confirmed bug). See
  [[ConnectIt/_decisions/2026-09-18-piece-registry-mapping-fix-re-keys-not-rebuilds|decisions/2026-09-18-piece-registry-mapping-fix-re-keys-not-rebuilds]].
  Reflects a local, uncommitted change at write time — `commit:` above still anchors to
  the last real commit; re-anchor once this lands in a commit.
- 2026-09-10 — page created; covers the 2026-09-09 registry-to-subsystem move
  (previously only in `ConnectIt/updates/`). Provenance anchored to `6477d5d` (*Create
  Connect It Registry*).
- 2026-09-09 — **tile/piece registries moved off `AConnectIt_PlayerController` onto this
  new `UWorldSubsystem`.** Concrete registry subclass + tuned values now live as
  `Instanced` template properties on `UConnectIt_LevelConfigDataAsset`;
  `DuplicateObject`'d per world at `OnWorldBeginPlay`. `UGridPieceRegistryBase::ShutdownRegistry()`
  added to the plugin (was missing — an asymmetry vs `UGridTileRegistryBase` flagged as a
  TODO in the unused `ABoardManagerBase` prototype). `GetPieceRegistry()` added to the
  utility library; `PieceRegistry` was 100% dead code before this pass.
  [[ConnectIt/_decisions/2026-09-09-board-registries-to-world-subsystem|decisions/2026-09-09-board-registries-to-world-subsystem]].

## Manual / content steps still outstanding

- Migrate `CI_PlayerController_Play.uasset`'s authored registry subclass/values onto the
  level's `UConnectIt_LevelConfigDataAsset` (in-editor).
- Confirm `CI_GridPieceRegistry.uasset` is the intended `PieceRegistry` template and
  assign it.
- Author/register at least one `UConnectIt_LevelConfigDataAsset` per level with
  `TileRegistry` set.
- PIE verification not yet run (single-client, multi-client, editor-world + PIE
  simultaneously, level-transition leak checks).

## See also

- Decisions: [[ConnectIt/_decisions/2026-09-09-board-registries-to-world-subsystem|decisions/2026-09-09-board-registries-to-world-subsystem]],
  [[ConnectIt/_decisions/2026-09-18-registry-mapping-refresh-on-game-event-complete|decisions/2026-09-18-registry-mapping-refresh-on-game-event-complete]],
  [[ConnectIt/_decisions/2026-09-18-grid-definition-extracted-from-tile-registry|decisions/2026-09-18-grid-definition-extracted-from-tile-registry]],
  [[ConnectIt/_decisions/2026-09-18-piece-registry-mapping-fix-re-keys-not-rebuilds|decisions/2026-09-18-piece-registry-mapping-fix-re-keys-not-rebuilds]]
- [[AConnectIt_GameMode|AConnectIt_GameMode]], [[UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]]
- Plugin bases: [[UnrealGridMechanics/code/__INDEX|UnrealGridMechanics/code/__INDEX.md]],
  [[UnrealGridMechanics/code/UGridDefinition|UGridDefinition]]
- [[UnrealGameMechanics/code/UGameEventTaskSubsystem|UGameEventTaskSubsystem]] (`OnAnyTagComplete`)
