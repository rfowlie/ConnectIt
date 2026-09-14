---
Date: 2026-09-09
status: Active
superseded by:
tags:
  - board
  - registries
  - subsystem
---

## Decision

Move `TileRegistry` / `PieceRegistry` off `AConnectIt_PlayerController` (their brief home
from the [[ConnectIt/_decisions/2026-09-08-board-architecture-overhaul|board-architecture overhaul]])
onto a new `UConnectIt_BoardRegistrySubsystem` (`UWorldSubsystem`) — exactly one instance
per world, resolved identically on server and every client, no controller-existence
dependency.

Because a `UWorldSubsystem` isn't level-placed or Blueprint-configurable, the concrete
registry subclass + tuned values (`GridSize`, …) a designer picks per level now live as
`Instanced` **template** properties on `UConnectIt_LevelConfigDataAsset`. The subsystem
resolves that asset once at `OnWorldBeginPlay` and `DuplicateObject()`s its own per-world
runtime instance from each template, outered to itself (itself outered to the `UWorld`).

Scope also closed `PieceRegistry`, which was 100% dead code: added
`UGridPieceRegistryBase::ShutdownRegistry()` to `UnrealGridMechanics` (was missing —
asymmetric vs `UGridTileRegistryBase`, already a TODO in the unused `ABoardManagerBase`),
added `UConnectIt_GameUtilityLibrary::GetPieceRegistry()`, and `AConnectIt_GameMode::InitialiseBoard`
now passes the real registry into `InitialiseBoardState` instead of `nullptr`. This was
an explicit owner choice ("Full symmetric fix") over a game-module-only patch, accepting
the plugin edit.

## Why

Registries are level-authored, deterministic, world-scoped **singletons**, not
per-machine or per-player state. On a controller, every reader had to guess or iterate
which controller's instance was "the" valid one, and `AConnectIt_GameMode::InitialiseBoard`
would silently fail if it ran before any controller connected. A `UWorldSubsystem`
resolved via `OnWorldBeginPlay` (Epic's documented "after every actor's `BeginPlay`"
hook) fixes both.

The `DuplicateObject`-per-world step is **load-bearing**: a `UDataAsset`'s `Instanced`
subobjects are shared, loaded-once objects. Using one directly as a live registry would
mis-resolve `GetWorld()` and share mutable state across simultaneous worlds (editor +
PIE, multi-client PIE) — a real risk for a project that multi-client-tests via an
`L_NetworkTest`-style setup.

## What Would Change It

- Outstanding manual/content steps: migrate the authored registry subclass + values from
  `CI_PlayerController_Play.uasset` onto the level's `UConnectIt_LevelConfigDataAsset`;
  confirm + assign `CI_GridPieceRegistry.uasset` as the `PieceRegistry` template;
  author/register a `UConnectIt_LevelConfigDataAsset` per level with `TileRegistry` set.
  **Until this is done every registry query returns null.**
- PIE verification (single-client, multi-client, editor-world + PIE simultaneously,
  level-transition / restart leak checks) is not yet run.
- If registries ever need to differ per client (they shouldn't — they're deterministic
  from level layout), this design would need revisiting.
