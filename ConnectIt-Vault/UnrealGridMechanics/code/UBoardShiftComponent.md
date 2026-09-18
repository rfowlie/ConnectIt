---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/Board/Shift/BoardShiftComponent.h
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Private/Board/Shift/BoardShiftComponent.cpp
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/Board/Shift/GridMechanics_GridShiftLibrary.h
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/Board/Shift/GridShiftTypes.h
reconciled: 2026-09-18
commit: 9187568
---

# UBoardShiftComponent

> **Not what ConnectIt's own Board Shift action uses** — no diagonal support (see
> [[UnrealGridMechanics/code/systems/board-shift|systems/board-shift]]'s own note).

`UActorComponent`. Shifts a row or column by N cells **with edge wraparound** — split
cleanly into a **pure compute** step and a **presentational animate** step. Covers
`UGridMechanics_GridShiftLibrary` (the pure math) and the shift data types.

## When you touch this

- Implementing a "slide a row/column" board mechanic.
- Needing the post-shift position map *before* committing board state.

## Entry points

- **Config:** `ShiftDuration` (seconds).
- `ComputeShift(FShiftOperation) → FShiftResult` — pure, synchronous, no tile calls. Call
  this directly when you need an authoritative remap before mutating state (the game's
  board manager does).
- `PlayShiftAnimation(FShiftOperation, FShiftResult) → bool` — purely visual; returns
  `false` if an animation is already running; does **not** recompute or mutate state.
- `IsShifting()`.
- Delegates: `OnShiftStarted`, `OnShiftCompleted` (both carry `FShiftOperation`).
- **`UGridMechanics_GridShiftLibrary`** (static, pure): `ComputeShiftResult`,
  `ComputeShiftedPosition`, `IsWrapping`, `BuildShiftInstructions`.
- **Types:** `FShiftOperation` (`Axis`, `Index`, `Amount ≥ 1`, `Direction`;
  `GetSignedAmount()`), `FShiftResult` (`PositionRemap`, `WrappingPositions`, `IsValid()`),
  `FTileShiftInstruction` (per-tile start/end world pos + `bIsWrapping`).

## Collaborators

- On `BeginPlay`, `ResolveComponents()` caches
  [[UnrealGridMechanics/code/UGridTileRegistryComponent|UGridTileRegistryComponent]] and
  `UBoardStateComponentBase`.
- Ticks the animation (`TickComponent` → `BroadcastAlphaToTiles` → `FinaliseShift`).
- Tiles animate themselves from `FTileShiftInstruction` (`StartWorldPosition` →
  `EndWorldPosition`, `bIsWrapping` for a special wrap anim).

## Gotchas

- **Compute and animate are decoupled on purpose.** State authority = `ComputeShift`
  result committed by the caller; `PlayShiftAnimation` is cosmetic and must be handed the
  same `FShiftResult`.
- `PlayShiftAnimation` silently no-ops (returns false) if already shifting — check the
  return.
- `FShiftResult::PositionRemap` / `WrappingPositions` are plain (non-`UPROPERTY`) — don't
  rely on reflection/replication of them.
- `FShiftOperation::Amount` is clamped `≥ 1`; direction lives in `Direction`, not the sign
  of `Amount`.

## Cross-impact

The game's board manager commits `ComputeShift` results into `UBoardStateComponentBase`;
tile actors consume `FTileShiftInstruction`. Changing `FShiftResult` shape touches all
three plus `GridMechanics_GridShiftLibrary`.

## Changes

- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[UnrealGridMechanics/CLAUDE|UnrealGridMechanics overview]] → *Board/Shift*.
- [[UnrealGridMechanics/code/systems/board-shift|systems/board-shift]]
