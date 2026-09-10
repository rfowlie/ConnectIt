---
schema: systems
spans:
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/Board/Shift/BoardShiftComponent.h
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Private/Board/Shift/BoardShiftComponent.cpp
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/Board/Shift/GridMechanics_GridShiftLibrary.h
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/Board/Shift/GridShiftTypes.h
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/Board/BoardStateComponentBase.h
reconciled: 2026-09-10
commit: ded6143
---

# Board shift

## What happens

A shift is described by an `FShiftOperation` (row/column, index, amount ≥ 1, direction).
[[UnrealGridMechanics/code/UBoardShiftComponent|UBoardShiftComponent]]`::ComputeShift`
turns it into an `FShiftResult` — a `PositionRemap` of every old→new cell plus the set of
cells that **wrapped** the edge — using pure math in `UGridMechanics_GridShiftLibrary`,
with no tile or world calls. The caller (the game's board manager) commits that remap
into `UBoardStateComponentBase` as the authoritative state, then hands the same
`FShiftResult` to `PlayShiftAnimation`, which builds a per-tile `FTileShiftInstruction`
(start/end world position, `bIsWrapping`), drives an alpha over `ShiftDuration`, and calls
`FinaliseShift` → `OnShiftCompleted`. Compute is truth; animate is cosmetic.

## Diagram

```mermaid
sequenceDiagram
    participant Caller as Board manager (game module)
    participant BSC as UBoardShiftComponent
    participant Lib as UGridMechanics_GridShiftLibrary (pure)
    participant State as UBoardStateComponentBase
    participant Tiles as AGridTileBase (each)

    Caller->>BSC: ComputeShift(FShiftOperation)
    BSC->>Lib: ComputeShiftResult(positions, op)
    Lib-->>BSC: FShiftResult { PositionRemap, WrappingPositions }
    BSC-->>Caller: FShiftResult
    Caller->>State: commit remap (authoritative) → BroadcastChange() → OnBoardStateChanged
    Caller->>BSC: PlayShiftAnimation(op, result)
    alt already shifting
        BSC-->>Caller: false
    else
        BSC->>BSC: BuildShiftInstructions (+ world position map from tile registry)
        BSC-->>Tiles: FTileShiftInstruction (start→end, bIsWrapping)
        loop each tick over ShiftDuration
            BSC->>Tiles: BroadcastAlphaToTiles(alpha)
        end
        BSC->>BSC: FinaliseShift() → OnShiftCompleted
    end
```

## Steps

1. **Compute:** `ComputeShift(Operation)` → `FShiftResult`
   (`UGridMechanics_GridShiftLibrary::ComputeShiftResult`; `ComputeShiftedPosition` /
   `IsWrapping` per cell).
2. **Commit:** the caller writes the remap into `UBoardStateComponentBase` (server), which
   calls `BroadcastChange()` → `OnBoardStateChanged` (and `OnRep` on clients).
3. **Animate:** `PlayShiftAnimation(Operation, Result)` — resolves tile actors + world
   positions from
   [[UnrealGridMechanics/code/UGridTileRegistryComponent|UGridTileRegistryComponent]],
   `BuildShiftInstructions`, fires `OnShiftStarted`.
4. **Tick:** `TickComponent` advances `ShiftAlpha`; `BroadcastAlphaToTiles` — each tile
   lerps `Start→End`, using `bIsWrapping` for a special wrap animation.
5. **Finalise:** `FinaliseShift()` → `OnShiftCompleted`.

## Gotchas

- **State is committed by the caller from the compute result**, not by the animation. If
  you skip step 2, the board desyncs from the visuals.
- `PlayShiftAnimation` returns `false` and does nothing if `IsShifting()` — don't fire a
  second shift mid-animation.
- `FShiftResult::PositionRemap` / `WrappingPositions` aren't `UPROPERTY` — no reflection.
- Tile animation is entirely tile-side; the component only broadcasts alpha + instructions.

## Cross-impact

Changing `FShiftResult` / `FTileShiftInstruction` shape touches
`UGridMechanics_GridShiftLibrary`, the game board manager (commit), and every tile actor's
animation code. This shift often runs *inside* a
[[UnrealGameMechanics/code/systems/gated-event-tag-queue|gated tag sequence]] so a turn waits
for it.

## See also

- [[UnrealGridMechanics/code/UBoardShiftComponent|UBoardShiftComponent]]
