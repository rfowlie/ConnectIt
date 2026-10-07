---
Date: 2026-10-07
status: Active
superseded by:
tags:
  - architecture
  - board
  - events
---
## Decision

What happens to the board is described by **one small struct per event** (`Board/Events/ConnectIt_BoardEvents.h`),
each holding only its own fields and deriving from `FConnectItBoardEvent`. `FConnectItBoardChangeEvent` is now just
**an ordered list of them** (`TArray<FInstancedStruct> Events`), in the order they happened, replicated with the
board snapshot as before.

- **Two categories, by base struct.** `FConnectItBoardOperationEvent`: what a board operation did (Piece Placed,
  Pieces Swapped, Board Shifted, Piece Captured, Piece Removed, Tile Multiplier Destroyed, Tile Active Toggled).
  `FConnectItBoardResultEvent`: what followed (Scored, Game Won). Board Seeded is in neither. "Operation" rather than
  "action", which already means the player-side input classes.
- **Whoever causes an event appends it.** The operation's `Apply` appends its operation event; the scoring rule
  appends one Scored event per thing that scored; the Mediator appends Game Won on the transition into game over.
  `FConnectItBoardEvent_Scored` replaces `FConnectItScoringConfiguration`.
- **Events are played one at a time, each carrying its own data.** The board state component queues one entry per
  event on `UGameEventTaskSubsystem`, with the event's gameplay tag and the event itself as the entry's payload
  (`QueueTagContainerWithPayload`, new in the UnrealGameMechanics plugin). A listener for the tag reads the event
  being played from `UConnectIt_BoardEventLibrary` (static, so no component reference is needed -- the data comes
  from the queue, not the board state): one typed function per kind of event (`GetActivePiecePlacedEvent`,
  `GetActiveScoredEvent`, ...), each with True/False pins in Blueprint and a logged error if that kind of event is
  not the one being played. There is deliberately no untyped Blueprint getter: matching a struct type to a tag by
  hand would fail quietly.
- Event gameplay tags are unchanged.

Supersedes the flat-step part of
[board change event is an ordered step list](2026-09-20-board-change-event-is-an-ordered-step-list.md): the list is
built, but its entries are polymorphic structs rather than one generic flat struct.

## Why

The flat change event had a bool and a few fields for every kind of event, grew with every new operation, mixed what
an operation did with what resulted, and could describe only one of each per change. Its tags carried no data, so
every listener read the single latest change event: two events of one kind couldn't be told apart, and a second board
change arriving while the first was still animating would be read by the first one's listeners. A payload on the queue
entry fixes both, and is usable by anything else that queues events.

The 2026-09-20 note chose a flat generic step "so Blueprint can read it". In UE 5.5 Blueprint reads an instanced
struct directly (Get Instanced Struct Value), at the cost of one extra node per read.

## What Would Change It

Blueprint handling of instanced structs proving too awkward for the visual handlers (then typed Blueprint accessors
per event on the component, not a return to a flat struct). Needing intermediate board states per event for visuals:
events must still describe every position they touched, since only the before and after boards replicate.

## Considered for Blueprint access (2026-10-07)

- A second, bespoke event system on the board state: rejected, it would duplicate the queue and leave two queues
  with no defined order between them.
- Typed delegates re-broadcast by the board state (a facade over the subsystem): workable, not chosen. Data would
  arrive when the tag begins, while visual listeners read it when their gated task runs.
