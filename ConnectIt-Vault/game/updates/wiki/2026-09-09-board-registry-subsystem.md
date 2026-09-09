---
date: 2026-09-09
slug: board-registry-subsystem
unit: game
raw: "[[raw/2026-09-09-board-registry-subsystem]]"
---

# 2026-09-09 — Tile/piece registries move to a world subsystem

## Summary

`TileRegistry`/`PieceRegistry` moved off `AConnectIt_PlayerController` (their home since the
previous "board architecture overhaul" pass) onto a new `UConnectIt_BoardRegistrySubsystem`
(`UWorldSubsystem`) — the concrete registry subclass + tuned values a designer picks per
level now live as `Instanced` template properties on `ConnectIt_LevelConfigDataAsset`
instead.

## What changed

- New `UConnectIt_BoardRegistrySubsystem` (`Source/ConnectIt/Public/Framework/Subsystem/`):
  one canonical instance per world. At `OnWorldBeginPlay()` — not `Initialize()`, since this
  hook is Epic's documented guarantee of running after every actor's own `BeginPlay` — it
  resolves the level's `ConnectIt_LevelConfigDataAsset` (via the existing
  `UConnectIt_GameUtilityLibrary::GetLevelConfig()`) and `DuplicateObject()`s its own
  runtime copy of each `Instanced` registry template, outered to itself. `Deinitialize()`
  calls `ShutdownRegistry()` on both.
- `ConnectIt_LevelConfigDataAsset` gained `Instanced` `TileRegistry`/`PieceRegistry`
  template properties (concrete-class polymorphism, matching `ABoardManagerBase`'s old
  pattern — unlike the interface-based `TScriptInterface` `ScoringRule`/`WinConditionRule`
  fields already on this asset).
- `AConnectIt_PlayerController` no longer owns either registry at all.
- `UConnectIt_GameUtilityLibrary::GetTileRegistry` simplified from a
  local-controller-fast-path-then-iterate-every-controller scan down to a straight
  subsystem lookup; `GetPieceRegistry` added (didn't exist before).
- `AConnectIt_GameMode::InitialiseBoard` now calls the utility library instead of
  duplicating its own controller-iteration scan, and passes a real `PieceRegistry` into
  `InitialiseBoardState` instead of an explicit `nullptr`.
- Plugin fix, folded in per the user's explicit scope decision:
  `UGridPieceRegistryBase::ShutdownRegistry()` added (`Plugins/UnrealGridMechanics`) —
  previously didn't exist at all, an asymmetry versus `UGridTileRegistryBase` already
  flagged as a TODO in the unused `ABoardManagerBase` prototype.

## Why it matters

Registries are level-authored, deterministic, world-scoped singletons, not per-machine or
per-player state — living on a controller forced every reader to guess/iterate which
controller's instance was "the" valid one, and made `AConnectIt_GameMode::InitialiseBoard`
fragile (it would silently fail if it ran before any controller had connected). The
subsystem gives exactly one instance per world, resolved identically on server and every
client, no controller-existence dependency.

The `DuplicateObject`-per-world mechanic is the load-bearing design choice, not incidental:
a `UDataAsset`'s `Instanced` subobjects are shared, loaded-once objects. Using one directly
as a live registry would mis-resolve `GetWorld()` and share mutable state across
simultaneous worlds (editor + PIE, multiple PIE clients) — a real risk for this project,
which explicitly multi-client-tests via an `L_NetworkTest`-style setup.

## Follow-ups

- [ ] Migrate `CI_PlayerController_Play.uasset`'s authored registry subclass/values onto
      the corresponding level's `ConnectIt_LevelConfigDataAsset` (manual, in-editor).
- [ ] Confirm `CI_GridPieceRegistry.uasset` is the intended `PieceRegistry` template and
      assign it (manual, in-editor).
- [ ] Author/register at least one `ConnectIt_LevelConfigDataAsset` with `TileRegistry` set
      per level, in Project Settings → "ConnectIt Level Config" — without this,
      `UConnectIt_BoardRegistrySubsystem` has nothing to duplicate and every registry query
      returns null.
- [ ] PIE verification not yet run: single-client, multi-client (`L_NetworkTest`-style),
      editor-world + PIE simultaneously, and level-transition/PIE-restart leak checks — see
      the approved plan's Verification section for the exact scenarios.

## Links

- Raw: [[raw/2026-09-09-board-registry-subsystem]]
- Related: [`old/Source/ConnectIt/Docs/ConnectItModule.md#board-registry-subsystem-second-pass`](../../old/Source/ConnectIt/Docs/ConnectItModule.md#board-registry-subsystem-second-pass), [[game/high-level|game/high-level.md]], [[game/code/AConnectIt_GameMode|game/code/AConnectIt_GameMode.md]]
