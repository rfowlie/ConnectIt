---
Date: 2026-09-18
status: Active
superseded by:
tags:
  - bug-fix
  - registries
  - game-event
---
## Decision

`TileRegistry`/`PieceRegistry` (via `UConnectIt_BoardRegistrySubsystem`) subscribe to
`GameEvent` completion and recompute their internal position→actor mappings for every
tile and piece whenever a relevant event fires, instead of assuming those mappings stay
correct after a board mutation.

## Why

Confirmed as a real, live bug by direct testing — not the theoretical risk it was raised
as earlier: after a run of random vertical shifts, a diagonal shift, and swaps, wrong
pieces were removed and an unrelated piece's faction color changed. Symptoms are
non-deterministic-looking (depends on the exact shift/swap history), which is consistent
with the registries' position→actor maps going stale after mutations rather than being
recomputed, not with `FConnectItBoardState` itself (the replicated snapshot scoring and
ownership actually read) being wrong — that was checked directly and is believed correct.
So this is a severe presentation-layer bug, not board-state corruption: past scoring
results aren't in question, but the registries can't be trusted to resolve
position→actor lookups correctly after enough shifts/swaps accumulate until this lands.

Confirms the direction already floated in
[`ConnectIt/_questions/action-gameplay-tags.md`](../_questions/action-gameplay-tags.md)'s
neighboring `_sessions/2026-09-17-1454.md` close note and the 2026-09-16 session's own
observation that nothing currently re-keys the registries after a game event.

## What Would Change It

If subscribing to every `GameEvent` completion and doing a full recompute turns out too
frequent/expensive once more action types exist, this may need to become a targeted,
incremental update (only re-resolving the positions a given event actually touched)
instead of a full recompute on every completion.
