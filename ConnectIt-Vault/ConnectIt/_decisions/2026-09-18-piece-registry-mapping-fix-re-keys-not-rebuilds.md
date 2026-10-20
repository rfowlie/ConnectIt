---
Date: 2026-09-18
status: Active
superseded by:
tags:
  - bug-fix
  - registries
  - gameevent
---
## Decision

`UConnectIt_PieceRegistry::HandleGameEventComplete` (bound once to
[[UnrealGameMechanics/code/UGameEventTaskSubsystem|UGameEventTaskSubsystem]]`::OnAnyTagComplete`
by `UConnectIt_BoardRegistrySubsystem`) re-keys `PieceMap` directly from
`FConnectItBoardChangeEvent`'s own authoritative grid positions —
`ShiftStartPositions`/`ShiftEndPositions` (index-aligned, two-pass snapshot-then-write
since a rotation's later pair can read a position an earlier pair just wrote),
`SwapPositionA`/`SwapPositionB`, `RemovedPosition` — **never** from `GetActorLocation()`
or `GridDefinition`'s conversion functions. Bound tags: `ConnectIt_Event_BoardShifted`,
`ConnectIt_Event_PiecesSwapped`, `ConnectIt_Event_PieceRemoved`. Deliberately **not**
bound: `ConnectIt_Event_PiecePlaced` (needs an insertion, but the change event carries no
actor pointer for what got spawned — that's the separate, already-flagged
`ActivatePieceAt`-never-positions-the-piece gap, not something a re-key can fix) and
`ConnectIt_Event_PieceCaptured` (only flips `FactionPiece` ownership in place — no actor
moves, nothing to re-key). Only `PieceRegistry` is touched — `TileRegistry`/`TileMap`
never goes stale, confirmed by reading `UConnectIt_BoardRequestMediator::HandleBoardShiftRequest`/
`HandleSwapPiecesRequest`/`HandleCapturePieceRequest` directly: all three rotate/swap
`FConnectItTileData` (abstract per-position data), never move or reassign a tile actor.

## Why

The obvious fix — wire the plugin's existing `UGridPieceRegistryBase::UpdateMappings()`
(rebuilds the map from each piece's *current* `GetActorLocation()`) up to fire on the
relevant tags — is wrong, not just less elegant. `GameEventTaskSubsystem::BindOnTagComplete`
fires **synchronously**, immediately when
`UConnectIt_BoardStateComponent::EnqueueBoardEventTags()` calls `QueueTagContainer` — i.e.
before any Blueprint-driven visual lerp (`UActorLerpComponent`) has moved an actor at
all, per
[2026-09-18 — Visual reactions in Blueprint, convention](2026-09-18-visual-reactions-in-blueprint-convention.md).
A rebuild at that instant reads pre-move transforms and silently reproduces a version of
the same staleness bug at a new point in time — nothing would ever correct it once the
lerp actually finished, since no second signal fires afterward. `ChangeEvent`'s own
positions have no such dependency: they're the Mediator's own authoritative
before/after, known synchronously, independent of whether or when any visual plays.

## What Would Change It

If `ConnectIt_Event_BoardShifted`/`PiecesSwapped`/`PieceRemoved` ever get gated async
tasks registered against them (`RegisterAsyncTask`), firing becomes deferred rather than
synchronous, and a second unrelated mutation could overwrite `BoardSnapshot.ChangeEvent`
before this handler runs — would need revisiting then. None registered today (confirmed).
Placement registration (the `ConnectIt_Event_PiecePlaced` gap above) is a real follow-up,
not designed here — needs whatever drives the placement visual to tell `PieceMap` which
actor it spawned, which the change event alone can't supply.
