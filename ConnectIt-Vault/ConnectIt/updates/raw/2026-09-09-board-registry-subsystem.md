---
date: 2026-09-09
slug: board-registry-subsystem
unit: game
---

# 2026-09-09 — Tile/piece registries move to a world subsystem

## Source

- Type: design conversation + implementation, same session
- Reference: n/a (no PR/commit range captured at write time)

## Verbatim

User's opening prompt for this session's change:

> I am reviewing the recent code changes we made. I am struggling with having the Tile
> Registry and the Piece Registry living on the Player Controller. This feels bizarre to
> me. Both of these objects should only exist one time in the world. We don't want them as
> actors because that could lead to potential destruction or interference by other actors.
> Should we consider having them live on a subsystem? We could name it
> ConnectItGameSubsystem. The issue is how they get set. The interface for Registries is
> fairly sound, I can't see sublclasses altering too much. Realistically there be a BP
> subclass for each registry in the project to enable designer tweaking. Should we include
> registries in the level instantiation data asset setup we just recently looked at? On
> level start the new subsystem can simply read from the Map<Level,
> ConnectItGameLevelDataAsset> to find the correct data asset to load and then instantiate
> the correct registries. What do you think about this?

Investigation findings that shaped the design (via Explore/Plan subagents):

- `UGridTileRegistryBase`/`UGridPieceRegistryBase` are `Instanced` UObjects
  (`EditInlineNew, DefaultToInstanced`) with `InitialiseRegistry()`/`ShutdownRegistry()`.
  `UGridTileRegistryBase::InitialiseRegistry()` does its own `TActorIterator<AGridTileBase>`
  world-scan + `AddOnActorSpawnedHandler` hook internally — it doesn't matter what owns the
  instance, only that `GetWorld()` resolves correctly via the Outer chain.
- `UWorldSubsystem::OnWorldBeginPlay(UWorld&)` is Epic's documented hook for "runs once,
  after every already-existing actor's `BeginPlay` has fired" — no class in this codebase
  or `UnrealGridMechanics` previously overrode it (new ground for this project).
- Pre-existing gaps found and folded into scope: `PieceRegistry` was fully dead code
  (accessor never called, `InitialiseBoardState`'s param unused, `PieceRegistry` had no
  `ShutdownRegistry()` method at all on `UGridPieceRegistryBase` — an asymmetry versus
  `UGridTileRegistryBase`, already flagged as a `// TODO: shutdown piece registry` comment
  in the plugin's own unused `ABoardManagerBase` prototype).
- Critical correctness constraint identified: a registry instance living as an `Instanced`
  subobject directly on `ConnectIt_LevelConfigDataAsset` would be a shared, loaded-once
  asset object — unsafe to use live (wrong `GetWorld()`, shared mutable state across
  simultaneous worlds e.g. editor+PIE or multi-client PIE). Resolved via
  `DuplicateObject<T>(Template, this)` per world, outered to the subsystem.

User's scope decision (AskUserQuestion, single question, three options presented):

> "PieceRegistry is currently 100% dead code today... How much of it should this pass
> touch, given the fix also means editing the UnrealGridMechanics plugin (not just the game
> module) to add a missing ShutdownRegistry() method?"
>
> Answer selected: **"Full symmetric fix (Recommended)"** — subsystem owns+inits both
> registries, `UGridPieceRegistryBase::ShutdownRegistry()` added to the plugin, `GetPieceRegistry()`
> added to the utility library, GameMode passes the real `PieceRegistry` into
> `InitialiseBoardState` instead of `nullptr`.

## Files touched

New:
- `Source/ConnectIt/Public/Framework/Subsystem/ConnectIt_BoardRegistrySubsystem.h`/`.cpp`

Modified:
- `Source/ConnectIt/Public/Framework/Data/ConnectIt_LevelConfigDataAsset.h`/`.cpp` — added
  `Instanced` `TileRegistry`/`PieceRegistry` template properties + `IsDataValid` warning.
- `Source/ConnectIt/Public/Framework/Controller/ConnectIt_PlayerController.h`/`Private/.../AConnectIt_PlayerController.cpp`
  — removed `TileRegistry`/`PieceRegistry` properties, accessors, and their
  `BeginPlay`/`EndPlay` init/shutdown calls entirely.
- `Source/ConnectIt/Public/Library/ConnectIt_GameUtilityLibrary.h`/`.cpp` — `GetTileRegistry`
  repointed to the subsystem; new `GetPieceRegistry` added.
- `Source/ConnectIt/Private/Framework/GameMode/ConnectIt_GameMode.cpp` — `InitialiseBoard`
  repointed to `UConnectIt_GameUtilityLibrary::GetTileRegistry`/`GetPieceRegistry` instead
  of an inlined `FConstPlayerControllerIterator` scan.
- `Source/ConnectIt/Public/Action/ConnectIt_PlacePieceAction.h`/`.cpp` — `TileRegistry`
  resolution repointed to `UConnectIt_GameUtilityLibrary::GetTileRegistry`.
- `Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/Piece/GridPieceRegistryBase.h`/`.cpp`
  — added `ShutdownRegistry()`.
- `Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Private/Board/BoardManagerBase.cpp`
  — filled in the `// TODO: shutdown piece registry` in `EndPlay`.

## Manual/content steps flagged, not yet done

- Migrate whatever `TileRegistry`/`PieceRegistry` concrete subclass + tuned values were
  authored on `Content/_ConnectItNetworked/Framework/Controller/CI_PlayerController_Play.uasset`
  onto the corresponding level's `ConnectIt_LevelConfigDataAsset`.
- `Content/_ConnectItNetworked/Game/Board/CI_GridPieceRegistry.uasset` is very likely the
  concrete `PieceRegistry` subclass to assign — confirm in-editor.
- Author/register a `ConnectIt_LevelConfigDataAsset` per level with `TileRegistry` set, in
  Project Settings → "ConnectIt Level Config".
