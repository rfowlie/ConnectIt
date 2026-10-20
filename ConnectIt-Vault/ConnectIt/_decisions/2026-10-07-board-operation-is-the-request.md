---
Date: 2026-10-07
status: Active
superseded by:
tags:
  - architecture
  - board
  - requests
---
## Decision

A `FConnectItBoardOperation` is **one specific change to the board, carrying its own data** (who makes it, and what it
acts on), and it **is the request**: a player's action builds one and sends it as `FTurnActionRequest::Payload`; the
server validates and applies that same struct; the AI's search builds and applies them too.

- **Every kind of board change is an operation**: Place Piece, Swap Pieces, Shift, Force Place Piece, Capture Piece,
  Remove Piece, Destroy Tile Multiplier, Toggle Tile Active (`Board/Operations/ConnectIt_BoardOperations.h`). Each has
  `CanApply(Board)` and `Apply(Board, OutTouched, OutEvent)`.
- **The Mediator has one path and knows no kind of change.** `DispatchRequest`: payload must be an operation → its
  request type must equal the request's → the server stamps the requester's faction on its own copy → `CanApply` →
  `Apply` on a copy of the board → `FConnectItRuleSet::ResolveBoardChange` (scoring) → `StampWinState` → commit. All
  `Handle...Request` functions and the request-type if-chain are gone.
- **Removed:** the eight `FConnectItRequest...` payload structs; `FConnectItBoardMove` and its per-kind data structs;
  payload conversions; the rule set's per-level `BoardOperations` list. What a player may do is decided by their
  loadout's actions only.
- **The AI search's move type is a `TVariant` of the operations it can model** (`FConnectItMinMaxMove`), today Place
  Piece only.

Two things the server must do because the operation arrives from a client:
1. Check the operation's request type matches the one on the request. The loadout gate approves the request's type;
   without the check a request could claim one type and carry another kind of operation.
2. Overwrite `Faction` with the requester's faction before using the operation.

## Why

Revision 1 still matched things up in several places: a request struct, a tag lookup of an operation in the rule set,
a conversion to a move value with per-kind data, and a fall-through if-chain for the request types that had no
operation. The owner found this confusing. One struct that is the request, the validation and the change removes the
matching, and one path through the Mediator removes the fall-through.

## What Would Change It

Needing a per-level allow list on top of loadouts (it would be request-type tags, not operation instances). An
operation whose data is too large or variable to send in a request.
