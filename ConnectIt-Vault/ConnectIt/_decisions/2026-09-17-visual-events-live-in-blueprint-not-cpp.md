---
Date: 2026-09-17
status: Active
superseded by:
tags:
  - visuals
  - blueprint
  - game-event
---

## Decision

Reacting to a gameplay event to drive a visual (animating a tile, playing VFX, anything
purely presentational) belongs in Blueprint, not C++. C++ exposes the data and the
reusable tools (the replicated event tags on `FConnectItBoardChangeEvent`, the generic
`UActorLerpComponent`/`FActorLerpInfo` lerp engine, `BuildRaisedShiftPath`'s waypoint
helper) — Blueprint owns binding to the event, deciding what should animate, and driving
those tools.

This reverses work done earlier in the same session: `AConnectIt_HUD` had been given a
full C++ implementation (constructor spawning a `UActorLerpComponent` subobject,
`BeginPlay` binding `ConnectIt_Event_BoardShifted` via
`UGameEventTaskSubsystem::BindOnTagComplete`, a `HandleBoardShiftVisualEvent` UFUNCTION
resolving `ShiftStartPositions`/`ShiftEndPositions` to tile actors and starting the lerp
batch) as a demonstration of how the shift-visual system would plug into the real event
pipeline. That wiring was removed in full; `ConnectIt_HUD.h`/`.cpp` are back to their bare
pre-session state.

## Why

Matches the pattern already used everywhere else pieces/tiles have a visual reaction to a
data-layer event: `AGridPieceBase::ActivatePoolObject`/`DeactivatePoolObject` and
`AConnectIt_GridPiece::OnFactionVisualUpdate` are all `BlueprintImplementableEvent`/
`BlueprintNativeEvent` with empty native bodies — C++ declares the hook, Blueprint fills
in what it actually looks like. The HUD wiring broke that convention by hardcoding the
event-reaction *decision-making* (which tag to bind, which positions resolve to which
actors, when to start the batch) directly in C++, not just exposing a callable tool.

## What Would Change It

- `UActorLerpComponent`/`FActorLerpInfo` (`UnrealGameMechanics`) and
  `BuildRaisedShiftPath` (`UnrealGridMechanics`'s `UGridMechanics_GridShiftLibrary`) are
  explicitly unaffected — both stay as generic, `BlueprintCallable`/`BlueprintPure`
  infrastructure for Blueprint to call directly. Only the ConnectIt-specific C++ glue
  code was removed, not the underlying tools.
- If a genuine case shows up where a visual reaction needs something Blueprint can't do
  (performance-critical, needs to run before Blueprint VM is safe to call, etc.), that
  would be a reason to revisit — no such case is known today.
