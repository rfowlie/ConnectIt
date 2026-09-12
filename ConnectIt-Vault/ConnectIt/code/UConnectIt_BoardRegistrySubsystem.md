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
  and `DuplicateObject()`s its own per-world runtime copy of each `Instanced` registry
  template, outered to itself.
- `Deinitialize()` — calls `ShutdownRegistry()` on both.
- `GetTileRegistry()` / `GetPieceRegistry()` (`BlueprintPure`) — the runtime duplicates,
  never the templates.
- `UConnectIt_TileRegistry::GetBoardState()` / `UConnectIt_PieceRegistry::GetBoardState()`
  (`BlueprintPure`) — on-demand `UConnectIt_BoardStateComponent*` resolution. Lives on
  the ConnectIt subclass, not the plugin's `UGridTileRegistryBase` /
  `UGridPieceRegistryBase`, which stay project-agnostic.

## Collaborators

- `UConnectIt_LevelConfigDataAsset` — carries the `Instanced` `TileRegistry` /
  `PieceRegistry` **template** properties a designer picks per level (concrete-class
  polymorphism with inline sub-properties like `GridSize`, matching the old
  `ABoardManagerBase` pattern — unlike the `TScriptInterface` `ScoringRule` /
  `WinConditionRule` on the same asset).
- [[AConnectIt_GameMode|AConnectIt_GameMode]]`::InitialiseBoard` pulls both registries
  from the utility library and passes them into
  [[UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]]`::InitialiseBoardState`.
- `UGridTileRegistryBase::InitialiseRegistry()` does its own `TActorIterator` world scan +
  `AddOnActorSpawnedHandler` — it only needs `GetWorld()` to resolve via the Outer chain,
  which the `DuplicateObject(Template, this)` (this = subsystem, outered to the world)
  guarantees.

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

## Cross-impact

Adding a new registry field: the `Instanced` template property on
`UConnectIt_LevelConfigDataAsset`, the `TObjectPtr` + `DuplicateObject` + accessor here,
`ShutdownRegistry()` symmetry, and a `GetXRegistry()` forwarder on
`UConnectIt_GameUtilityLibrary`.

## Changes

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

- Decision: [[ConnectIt/_decisions/2026-09-09-board-registries-to-world-subsystem|decisions/2026-09-09-board-registries-to-world-subsystem]]
- [[AConnectIt_GameMode|AConnectIt_GameMode]], [[UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]]
- Plugin bases: [[UnrealGridMechanics/code/__INDEX|UnrealGridMechanics/code/__INDEX.md]]
