---
schema: code
kind: UCLASS
role: primary
source:
  - Source/ConnectIt/Public/Framework/Subsystem/ConnectIt_BoardRegistrySubsystem.h
  - Source/ConnectIt/Private/Framework/Subsystem/ConnectIt_BoardRegistrySubsystem.cpp
  - Source/ConnectIt/Public/Board/ConnectIt_TileRegistry.h
  - Source/ConnectIt/Public/Board/ConnectIt_PieceRegistry.h
reconciled: 2026-10-23
commit: 56d6946
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
  guarantee of running after every existing actor's `BeginPlay`. Resolves the map's board -- the
  `UConnectIt_LevelConfigDataAsset` its World Settings name (`AConnectIt_WorldSettings::FindLevelConfig`) -- and
  `DuplicateObject()`s its own per-world runtime copy of each `Instanced` template —
  **`GridDefinition` first**, then `TileRegistry`/`PieceRegistry`, each wired with a
  reference to the duplicated `GridDefinition` before its own `InitialiseRegistry()` runs
  (`DiscoverExisting` needs it already assigned). Also binds **both** `TileRegistry` and `PieceRegistry` to
  [[UnrealGameMechanics/code/UGameEventTaskSubsystem\|UGameEventTaskSubsystem]]`::OnAnyTagComplete`
  here, once. Then spawns the **board visualizer** (`AConnectIt_BoardVisualizer`, the level config's Visualizer
  Class or the C++ base; `Initialise()`), which plays every board event on the actors on every machine
  ([[add-visuals-for-a-board-event]]). `GetBoardVisualizer()` returns it. No board on the map → a warning ("fine
  for a menu map") and no registries; missing template → logged error per registry.
- `Deinitialize()` — unbinds both registries from `OnAnyTagComplete`, then calls
  `ShutdownRegistry()` on both.
- `GetTileRegistry()` / `GetPieceRegistry()` / `GetGridDefinition()` (`BlueprintPure`) —
  the runtime duplicates, never the templates.
- `UConnectIt_TileRegistry::GetBoardState()` / `UConnectIt_PieceRegistry::GetBoardState()`
  (`BlueprintPure`) — on-demand `UConnectIt_BoardStateComponent*` resolution. Lives on
  the ConnectIt subclass, not the plugin's `UGridTileRegistryBase` /
  `UGridPieceRegistryBase`, which stay project-agnostic.
- `UConnectIt_TileRegistry::HandleGameEventComplete(FGameplayTag)` and
  `UConnectIt_PieceRegistry::HandleGameEventComplete(FGameplayTag)` — the `OnAnyTagComplete`
  handlers, both **`BlueprintImplementableEvent`s** (public, so `AddDynamic`/`RemoveDynamic` can name
  them from this unrelated subsystem class). The re-key of the position→actor maps on
  shift/swap/remove is therefore authored in Blueprint per the project's visuals-in-Blueprint
  convention; `PieceRegistry` keeps protected C++ `HandleBoardShifted`/`HandleBoardPiecesSwapped`/
  `HandleBoardPieceRemoved` stubs (marked TODO — undecided whether they're useful).
- **A phased level's tiles** (`UConnectIt_TileRegistry`, BlueprintCallable, called from the Tiles Added / Tiles Removed
  handlers on every machine): `SpawnLayoutTiles(Cells)` spawns the tile catalog's class for each cell's power tag
  (`UConnectIt_TileCatalogDataAsset::FindFor`; the level config's `DefaultTileClass` only when there is no catalog) at its grid
  position -- each registers itself; tiles implementing the tile data interface get `ApplyLayoutCell` -- and returns
  them in cell order; `UnregisterTilesAt(Positions)` stops tracking those tiles and returns their actors to animate
  out and destroy.
- **Builder tiles** (`UConnectIt_TileRegistry::SwapBuilderTiles`, C++): replaces every `AConnectIt_BuilderTile` with
  its catalog class, given the builder's values and look; the builder is destroyed. The board state component calls it
  once the board is known (seeded on the server, first arrival on a client). Returns how many were swapped.
- **Actors following the board** (BlueprintCallable, from the event handlers on every machine):
  `UConnectIt_TileRegistry::ShiftTiles(Start, End)` (Board Shifted: each tile actor becomes the one at its end
  position, a rotation; returns them to animate) and `UConnectIt_PieceRegistry::MovePieceActor(From, To)` (Piece Moved:
  re-keys the piece actor and returns it to slide).

## Collaborators

- `UConnectIt_LevelConfigDataAsset` — carries the `Instanced` `TileRegistry` /
  `PieceRegistry` / **`GridDefinition`** **template** properties a designer picks per
  level (concrete-class polymorphism with inline sub-properties, matching the old
  `ABoardManagerBase` pattern). The level config is the board only -- reusable by any number of maps; how a match is
  played is a match preset. `GridDefinition` (grid geometry — cell size,
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
  — both registries listen (Blueprint-implemented handlers); see
  [[ConnectIt/_decisions/2026-09-18-piece-registry-mapping-fix-re-keys-not-rebuilds|decisions/2026-09-18-piece-registry-mapping-fix-re-keys-not-rebuilds]]
  for why the handler re-keys from `FConnectItBoardChangeEvent` rather than rebuilding
  from actor transforms.

## Gotchas

- **The `DuplicateObject`-per-world step is load-bearing, not incidental.** A
  `UDataAsset`'s `Instanced` subobjects are shared, loaded-once objects. Using one
  directly as a live registry would mis-resolve `GetWorld()` and share mutable state
  across simultaneous worlds (editor + PIE, multiple PIE clients) — a real risk for a
  project that multi-client-tests via an `L_NetworkTest`-style setup.
- **Every registry query returns null on a map with no board.** A match map's World Settings must name a
  `UConnectIt_LevelConfigDataAsset` with `TileRegistry` set (World Settings > ConnectIt > Level Config); without it the
  subsystem has nothing to duplicate. (The project-settings registry is gone.)
- **Tile actors aren't replicated**: a phased level's tile changes are reconciled on each machine from the board events
  (`SpawnLayoutTiles` / `UnregisterTilesAt`), never spawned by the server for everyone.
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

- 2026-10-23 — tile powers: the subsystem creates the tile visualizer; `UConnectIt_TileRegistry` spawns layout tiles
  from the tile catalog and gained `SwapBuilderTiles` and `ShiftTiles`; `UConnectIt_PieceRegistry` gained
  `MovePieceActor`. See [tile catalog and builder tiles](../_decisions/2026-10-22-tile-catalog-and-builder-tiles.md),
  [the first tile powers](../_decisions/2026-10-23-the-first-tile-powers.md). (`process-code` sweep — commit
  `56d6946`.)
- 2026-10-22 — the board comes from the map's World Settings (`AConnectIt_WorldSettings::FindLevelConfig`; no board =
  a warning, fine for a menu map); `UConnectIt_TileRegistry` gained `SpawnLayoutTiles` / `UnregisterTilesAt` (phased
  levels). The 2026-09-18 change below has since been committed.
- 2026-09-21 — `TileRegistry` now also bound to `OnAnyTagComplete`; both registries' `HandleGameEventComplete`
  are `BlueprintImplementableEvent`s (the C++ re-key dispatch was replaced by Blueprint handlers).
- 2026-09-18 — **`GridDefinition` construction/wiring added** (duplicated before either
  registry, assigned to both); **`PieceRegistry` bound to `OnAnyTagComplete`** so its
  `PieceMap` stays in sync with shift/swap/remove (previously stale after enough board
  mutations — a confirmed bug). See
  [[ConnectIt/_decisions/2026-09-18-piece-registry-mapping-fix-re-keys-not-rebuilds|decisions/2026-09-18-piece-registry-mapping-fix-re-keys-not-rebuilds]].
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
- Name a `UConnectIt_LevelConfigDataAsset` with `TileRegistry` set in each match map's World Settings (and a
  `DefaultTileClass` for maps that change layout).
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
