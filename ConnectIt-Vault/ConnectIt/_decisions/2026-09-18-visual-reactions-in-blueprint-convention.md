---
Date: 2026-09-18
status: Active
superseded by:
tags:
  - visuals
  - blueprint
  - convention
---
## Decision

Any purely presentational reaction to a gameplay event — tile/piece animation, VFX, a UI
flourish, anything that doesn't affect game state — is authored in Blueprint, never C++.
C++'s job stops at exposing data (replicated event tags, board-state fields) and generic,
reusable tools (`UActorLerpComponent`/`FActorLerpInfo`, `BuildRaisedShiftPath`, etc.).
Framed explicitly as this project's frontend/backend split. This elevates
[2026-09-17 — Visual events live in Blueprint, not C++](2026-09-17-visual-events-live-in-blueprint-not-cpp.md)
from "the call made for the board-shift case" to a standing, named project convention.

## Why

Confirmed directly by the owner as "the standard to continue to strive for." Writing it
up as its own convention note means the next visual-reaction question (piece pooling,
scoring VFX, whatever comes after blockers) checks an established rule instead of
re-deriving the reasoning from a board-shift-specific decision each time. The reasoning
itself is unchanged from the 09-17 note: it matches the pattern already used everywhere
else a piece/tile has a visual reaction to a data-layer event
(`AGridPieceBase::ActivatePoolObject`/`DeactivatePoolObject`,
`AConnectIt_GridPiece::OnFactionVisualUpdate` — all `BlueprintImplementableEvent`/
`BlueprintNativeEvent` with empty native bodies).

## What Would Change It

Same carve-out named in the originating 09-17 note: a genuine case where a visual
reaction needs something Blueprint can't do (performance-critical, or must run before the
Blueprint VM is safe to call) would be a reason to revisit. No such case is known today.
