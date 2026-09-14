---
Date: 2026-09-08
status: Active
superseded by:
tags:
  - board
  - architecture
  - networking
---

## Decision

Retire the single world-placed `AConnectIt_BoardManager` actor (which hosted board state,
config, rules, and the tile/piece registries all as actor components) and split its jobs
across purpose-fit owners:

- **Board state** → `UConnectIt_BoardStateComponent` on `AConnectIt_GameState` — one
  `CreateDefaultSubobject`, the single replicated source of truth, genuinely singular the
  way `UTurnBasedParticipantManagerComponent` already is on the same GameState.
- **Request handling + rules** → `UConnectIt_BoardRequestMediator` +
  `UConnectIt_BoardRules`, plain `UObject`s constructed on `AConnectIt_GameMode` in
  `HandleMatchHasStarted()` — **server-only by construction** (`GetAuthGameMode()` is null
  on clients by engine design), so no `HasAuthority()` guard is needed or present.
- **Per-level config** → `UConnectIt_LevelConfigDataAsset` + `UConnectIt_LevelConfigSettings`
  (level-name lookup), replacing `UConnectIt_ConfigComponent` and in-level Blueprint
  configuration.
- **Registries** → first relocated onto `AConnectIt_PlayerController` (per-machine); this
  was **superseded four days later** — see
  [[ConnectIt/_decisions/2026-09-09-board-registries-to-world-subsystem|2026-09-09-board-registries-to-world-subsystem]].
- **The whole shift pipeline** (`UConnectIt_BoardShiftComponent`, `UConnectIt_ShiftAction`,
  `FConnectItRequestBoardShift`, the `Shift` tags, …) was confirmed fully dead
  (`HandleShiftRequest` always returned `false`, body commented out) and **removed
  outright**, not migrated.
- `UConnectIt_BoardManagerSubsystem` (the world-actor discovery cache) removed with the
  actor.

Construct the mediator/rules in `HandleMatchHasStarted`, **not the constructor** —
`NewObject` in the ctor runs before Blueprint-child property overrides are applied
(a CDO/archetype-timing pitfall this project has hit before).

## Why

The old model had no natural replication boundary: a single actor holding everything,
mutated in place, assuming it was the only copy. Network play needs one authoritative
replicated property clients converge on — which the board-state component now is — with
request handling that is unreachable from clients by structure rather than by a runtime
check that could be bypassed. Splitting by concern also makes each piece independently
testable and removes a placeable actor that could be destroyed or interfered with.

## What Would Change It

- Outstanding manual editor/content steps (this decision's code change can't make them):
  remove `AConnectIt_BoardManager` placements from `ConnectIt_Level_Sandbox.umap`,
  `L_NetworkTest.umap`, `L_Sandbox.umap`; the two Blueprint children now have a deleted
  parent class and need removal/replacement; author + register at least one
  `UConnectIt_LevelConfigDataAsset` per level; strip the SCS-added
  `UConnectIt_BoardShiftComponent` reference and any loadout references to the deleted
  shift actions. Until these are done, levels won't run cleanly.
- The in-progress game-event queue on the mediator
  (`CreateGameEventsFromBoardUpdate` / `ExecuteGameEvents` — `ExecuteGameEvents` has no
  body yet) becoming functional, which finishes replacing the removed tag-reactive
  interpreter.
