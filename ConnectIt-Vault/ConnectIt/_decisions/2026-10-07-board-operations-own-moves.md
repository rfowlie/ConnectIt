---
Date: 2026-10-07
status: Partly superseded
superseded by: 2026-10-07-board-operation-is-the-request (rule-set list, move value, Mediator scope)
tags:
  - architecture
  - board
  - ai
---
## Decision

A kind of move on the board is one thread-safe struct, **`FConnectItBoardOperation`** (Place Piece, Swap Pieces so
far). It owns exactly two things: **which moves of its kind exist** (`GenerateMoves`, `IsMoveValid`) and **how one
changes the board** (`ApplyMove`, which reports the tiles it touched). It does not score, check for a win, or pass the
turn.

What follows from a board change is a **separate step**: `FConnectItRuleSet::ResolveBoardChange` scores every touched
tile that now holds a faction's piece. Win state is a third step (`StampWinState` / `GetWinningFaction`).

- A match's operations live **in its rule set** (`FConnectItRuleSet::BoardOperations`, default Place + Swap), matched
  to requests by request type. A player can use the ones their loadout grants.
- **The Mediator and the AI's search run the same operations and the same resolve step.** The Mediator has one generic
  handler (`HandleBoardOperationRequest`) for any request type that has an operation; the hand-written place and swap
  handlers are gone. `FConnectItMinMaxRules` no longer knows any move: `GenerateMoves` asks each of the side-to-move's
  operations, `ApplyMove` is "operation applies → rule set resolves → turn passes".
- A move is a compact value: the operation's gameplay tag plus that operation's own data struct,
  `FConnectItBoardMove { RequestType, TVariant<FConnectItPlaceMove, FConnectItSwapMove> Data }` (revised the same day
  from an index and two generic tile slots); operations convert to and from the
  request payload structs.
- **The AI's search is handed Place Piece only for now**, even when a loadout grants Swap. Shift, Capture and Force
  Place stay hand-written in the Mediator.

## Why

`FConnectItMinMaxRules` hardcoded "empty tiles" as the only moves and its `ApplyMove` both placed the piece and scored
it, while the Mediator hand-wrote the same validate → change → score → win sequence per request type. Two copies of
"what a move does" can drift apart, and adding a move kind meant touching both. The name is "operation" because
"action" already means the player-side input classes (`UTurnBasedAction`).

The search stays Place-only because it assumes every move ends the turn and has no use limit; that is false for a
swap. Generating swap moves without modelling that would make the AI plan with moves it may not have.

## What Would Change It

A move whose effect can't be expressed as "change tiles, then score the touched ones" (the planned reactions /
step-list event in [board-request-objects](../design/board-request-objects.md) extend the resolve step rather than
replace it). Operations needing more than two tiles (Shift) will need a wider move value.

## Update (2026-10-07, later the same day)

Still true: an operation owns how a board change is made, scoring is a separate `ResolveBoardChange` step, and the
Mediator and the AI's search run the same code. Replaced by
[the operation is the request](2026-10-07-board-operation-is-the-request.md): operations are no longer "kinds" listed
in the rule set and matched by tag, there is no separate move value, and every request type (not only Place and Swap)
is an operation.
