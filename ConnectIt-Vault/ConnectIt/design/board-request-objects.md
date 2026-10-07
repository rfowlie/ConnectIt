# Board request objects, reactions and the step-list ChangeEvent

Design note (no schema, like the rest of `design/`). **Nothing is built.** Settled in a design
talk on 2026-09-20 that started from the [Crumble](tile-powers.md#crumble) power. Decisions:
[layers](../_decisions/2026-09-20-board-changes-layered-primitives-requests-reactions.md),
[step-list event](../_decisions/2026-09-20-board-change-event-is-an-ordered-step-list.md),
[Crumble threshold on replicated state](../_decisions/2026-09-20-crumble-threshold-stamped-on-replicated-board-state.md).

## Problem

`UConnectIt_BoardRequestMediator` hardcodes every request:

- Seven `Handle*Request` methods repeat the same tail — validate, copy state, mutate, score,
  win check, build a `FConnectItBoardChangeEvent`, `SetBoardState`. Only the validation, the
  mutation and the event fields differ. Adding a mechanic like Crumble (a step between
  `ApplyScoring` and `CheckWinCondition`) would mean touching every scoring handler.
- Dispatch is an `if (RequestType == ...)` ladder, and each action separately answers
  `ProducesRequestType`. Adding an action takes about six edits (tag, payload struct, dispatch
  `if`, handler, `ProducesRequestType`, a `ChangeEvent` field).
- The tags already disagree: Swap and Shift set `RequestType` to a `ConnectIt_Game_*` constant;
  PlacePiece sets it to `GetActionTag()` and carries an unused `Tag_RequestType` property.
- Five request types (ForcePlace, DestroyMultiplier, Remove, Toggle, Capture) have no action —
  they are primitive-shaped operations wrapped as full requests.
- `FConnectItBoardChangeEvent` is a flat set of bools, one slot per kind; a turn like
  place → score → two crumbles doesn't fit, and `EnqueueBoardEventTags` orders them with a
  hardcoded `if` ladder.

## Layers

```
Action (client) ──TSubclassOf──► Request object (server)        [1:1 by class]
                                     │ Validate / Apply
                                     ▼
Primitives  (RemovePiece, SetTileActive, SetMultiplier, SetPieceOwner)  — mutate only, emit nothing
                                     ▲
Reactions (Crumble, Poison, …) ──────┘  compose the same primitives
```

- **Primitives** — pure state mutations on a mutable board state. They emit no events.
- **Request objects** — one per action, paired by class (the action holds
  `TSubclassOf<RequestObject>`; the tag is derived from the class default object, the same idea as
  [action config keyed by class](../_decisions/2026-09-18-action-config-keyed-by-class-not-tag.md)).
  Each declares its payload struct, validates, composes primitives, and appends one **semantic**
  step (`PiecesSwapped`, not two ownership flips). The mediator instantiates them from the
  loadouts, in parallel with how actions are built. The tag and `ProducesRequestType` checks
  collapse into a class lookup; the seven `Payload.GetPtr<>` blocks become one generic payload-type
  check.
- **Pipeline (once, in the mediator)** — validate → copy state → `Apply` → score every touched,
  occupied position by its current occupant (what Shift already does; also removes the "one
  `ScoringFactionSlot`" simplification) → reactions → win check → finalize event →
  `SetBoardState`. Requests that must not score (remove/toggle/destroy-multiplier today) opt out
  explicitly.
- **Reactions** — an ordered `BoardReactions` array on `LevelConfig`, next to the existing
  `ScoringRule`/`WinConditionRule`/`TilePlaceableRule`; empty by default, so a mechanic is on or off
  per level. They scan the whole board (idempotent, boards are small), run to a fixpoint with a
  max-depth guard for cascades, call primitives, and append their own semantic steps. Ordering is
  the array's order.

## The ChangeEvent as an ordered step list

`EnqueueBoardEventTags` today queues one tag per set bool; the reactor for a tag reads positions
from the single per-turn `ChangeEvent`. That breaks once a turn can produce several steps of the
same kind.

- `ChangeEvent` becomes an **ordered list of steps** in the order things happened:
  `{ EventTag, SubjectPositions[], RelatedPositions[], FactionA, FactionB, Value }` — flat so
  Blueprint can read it (not `FInstancedStruct`). Swap A/B and Shift start/end are both
  Subject/Related pairs; Place/Remove/Crumble are a single subject position. The turn summary
  (`bGameWon`, points) stays separate.
- `EnqueueBoardEventTags` becomes a loop over the steps; ordering is data.
- **The queued entry carries its step index**, so a reactor reads its own step's data (needs a small
  change to the `UGameEventTaskSubsystem` queue API).
- **Steps must describe every position they touched.** Snapshot Previous/Current only bracket the
  whole turn, so a reactor that reads final state for a crumbled tile sees lava and no piece — the
  same class of bug as the
  [scoring-line decision](../_decisions/2026-09-18-scoring-line-visuals-must-cover-all-mutated-positions.md).
  Intermediate states are not replicated.
- The piece registry re-keys per step from the positions the step names.

## Crumble

Modular per level, not a tile-carried power: a level lists a Crumble reaction to get it. Effect:
`SetTileActive(false)` + `RemovePiece` on tiles whose multiplier reaches the threshold, appending a
`TileCrumbled` step.

- **Cracks are state-driven** (tile multiplier vs threshold); only the explosion is an event. The
  client can't read server-only `BoardRules`, so the threshold is **stamped onto replicated board
  state** at init, the same precedent as `TargetScore`.
- Lava vs a tile merely toggled off look identical in state today (`bIsActive = false`); if the
  visuals must tell them apart the tile needs a state tag.
- Visual reactions stay in Blueprint per the
  [Blueprint-visuals convention](../_decisions/2026-09-18-visual-reactions-in-blueprint-convention.md).

## Migration (additive, like the action-state Stage 1)

0. Interim, cheap: extract the shared tail into one `FinalizeBoardChange(...)` the five scoring
   handlers call — Crumble can land before the refactor, and the same function becomes the pipeline.
1. Add the step list beside the existing bools and dual-write.
2. Switch `EnqueueBoardEventTags` and the Blueprint reactors onto steps.
3. Remove the old bools and the dispatch ladder as request objects replace the handlers.

## Open items

- The exact primitive set — derive from what requests and reactions need, not in advance.
- The queue API change for carrying a step index.
- Lava tile state (tag vs `bIsActive` only).
- Where the reaction fixpoint's depth cap is configured.
- Whether the dormant `TurnBasedGameEventQueue`/`ExecuteGameEvents` scaffolding in the mediator is
  now redundant and can go.

## Amendment (2026-10-06): everything here must be thread-safe plain C++

Since this note was written the AI gained a background MinMax search, and on 10-06 the rules became plain thread-safe
structs shared by the game and the search ([decision](../_decisions/2026-10-06-rules-are-thread-safe-structs.md),
[note](rules-as-structs-and-shared-simulation.md)). The same applies to this design when it is built ("Phase B"):

- **Request / move types, primitives, reactions and the pipeline are plain C++ (polymorphic structs or stateless
  handlers), not UObjects** -- no BlueprintNativeEvents anywhere in the path. `Mediator::ProcessRequest` and the
  search's `ApplyMove` then run the *same* pipeline: apply the move → score touched occupied positions
  (`FConnectItRuleSet`) → reactions → win state. Today `FConnectItMinMaxRules::ApplyMove` re-implements place-piece
  only; after Phase B it is "run the pipeline on a copy of the board".
- **"How a move changes a board" belongs to the move type** (one per request type: place, swap, shift...), each with
  validate + apply, used by both callers. The search's `FMove` becomes "request type + compact payload" rather than a
  tile index.
- **Hot-path constraints** (the search runs hundreds of thousands of positions per second): no per-node heap
  allocation for moves (compact value payloads, stateless per-type handlers rather than an instanced object per move);
  the step-list change event is an *optional* output -- the Mediator asks for it, the search passes null.
- **Reactions** join `FConnectItRuleSet` as an ordered array of thread-safe structs.
- **Phase C** (separate, exploratory -- the wishlist task): per-move-type *generation* for the AI, and a search that
  understands multi-action turns and use budgets. Phase B makes it possible; it does not solve it.

## Update (2026-10-07): the first slice of Phase B exists

Built as [board operations](board-operations.md) ([decision](../_decisions/2026-10-07-board-operations-own-moves.md)):

- **Done:** a per-move-type struct (`FConnectItBoardOperation`: generate, validate, apply) for **Place** and **Swap**,
  living in `FConnectItRuleSet`; a compact move value (`FConnectItBoardMove`); the shared after-move step
  (`FConnectItRuleSet::ResolveBoardChange`, scoring only so far); one generic Mediator handler; the search's
  `ApplyMove` runs the same two steps on a copy of the board.
- **Still to do from this note:** Shift, Capture and Force Place as operations (Shift needs a move value wider than
  two tiles); reactions inside the resolve step; the step-list change event (operations still fill the flat-bool
  event through `WriteChangeEvent`).
- **Phase C unchanged:** the search is handed Place only; Swap generates no moves until turns with several actions and
  use limits are modelled.

## Update (2026-10-07, revision 2): every request type is an operation

[The operation is the request](../_decisions/2026-10-07-board-operation-is-the-request.md): Shift, Capture, Force
Place, Remove, Destroy Multiplier and Toggle are now operations too, the request payload is the operation itself, and
the Mediator has a single path. Still to do from this note: reactions inside the resolve step, and the step-list
change event (operations fill the flat-bool event from `Apply`).
