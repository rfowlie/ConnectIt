---
schema: recipes
task: Add a new kind of board change (a board operation) end to end -- the operation struct that is the request, its event that applies itself, their tags, a Blueprint getter, and the action that sends it (or a tile power or the Level director that uses it).
touches:
  - Source/ConnectIt/Public/Board/Operations/ConnectIt_BoardOperations.h (a FConnectItBoardOperation subtype; base in ConnectIt_BoardOperation.h)
  - Source/ConnectIt/Public/Board/Events/ConnectIt_BoardEvents.h (a FConnectItBoardEvent subtype with ApplyTo)
  - Source/ConnectIt/Public/ConnectIt_GameplayTags.h (a ConnectIt.BoardOperation.* operation tag + a ConnectIt.Event.* event tag)
  - Source/ConnectIt/Public/Framework/Library/ConnectIt_BoardEventLibrary.h (GetActive<Name>Event)
  - (if it concerns tiles) Source/ConnectIt/Public/Board/ConnectIt_TileVisualizer.h (the tile hooks it calls)
  - a UTurnBasedPlayerAction subclass that builds and sends it (ProducesRequestType), if players make it
reconciled: 2026-10-23
commit: 56d6946
---

# Recipe: add a board request type

## Goal

A new kind of board change (e.g. "rotate a 2×2 block") that a player action, the Level director or a tile power can
ask for; the server validates and applies it, the AI's search can simulate it, and visuals react to its event.

**A board operation is the request** ([decision](../../_decisions/2026-10-07-board-operation-is-the-request.md)): the
action sends the operation struct as the request's payload, and the Mediator validates and applies that same struct.
Nothing in the Mediator changes for a new kind.

## Prerequisites

- Read [[place-piece-request]] (the request path), [[board-state-single-source-of-truth]] (events and the display
  board) and [[tile-power-chain]] (what follows a change).
- Templates: `FConnectItBoardOperation_MovePiece` and `FConnectItBoardEvent_PieceMoved` (small, recent).

## Steps

1. **Tags** (`ConnectIt_GameplayTags.h` / `.cpp`, native): an operation tag `ConnectIt.BoardOperation.<Name>`
   (`UE_DECLARE_GAMEPLAY_TAG_EXTERN` / `UE_DEFINE_GAMEPLAY_TAG_COMMENT`) and an event tag `ConnectIt.Event.<Name>`.
2. **The event** (`ConnectIt_BoardEvents.h`): a struct deriving `FConnectItBoardOperationEvent` with `UPROPERTY` fields
   for everything it changed (the from / to, the owner, the previous and new values...). Override `GetEventTag` and
   **`ApplyTo(Board)`**: make the same change to a board from the event's data alone. Previous + every event's
   `ApplyTo` must equal the board after the change -- that's how the display board steps along.
3. **The operation** (`ConnectIt_BoardOperations.h`): a `USTRUCT(BlueprintType, meta = (DisplayName = ...))` deriving
   `FConnectItBoardOperation`, every field a `UPROPERTY` (or it won't reach the server). Override:
   - `GetOperationTag` → your operation tag;
   - `CanApply(Board, OutWhyNot)` → whether it may be made, with a reason for the log;
   - `Apply(Board, OutChanges, OutEvents)` → change the board, report what really changed in `OutChanges`
     (`AddEntered` / `AddExited` / owner and tile changes -- scoring and tile powers react to these), and append your
     event if `OutEvents` isn't null (the AI's search passes null). Clamp a multiplier through
     `OutChanges.ClampMultiplier`.
   - **The tile lock:** `GetTargetPositions` (the existing tiles it acts on, so a locked one is refused), or
     `CanOperateOnLockedTiles` → true if it may reach them ([[lock-a-tile-or-chain-tiles]]).
   - Optional: `ArrivingPiecesSurviveScoring`, `Describe`, `HasAnyLegalInstance` (lets the AI pass and the No Legal
     Move condition know when none is possible).
   - Plain data: no UObject references -- the AI's search applies it on a worker thread.
4. **The Blueprint getter** (`UConnectIt_BoardEventLibrary`): `GetActive<Name>Event(WorldContext, OutEvent)` →
   `TryGetActiveBoardEvent`, so a listener for your event tag reads the event that is playing.
5. **Tile visuals**, if it changes tiles: add its tag to `UConnectIt_TileVisualizer`'s handled tags and call the
   tile hooks it implies (e.g. `OnPieceExit` at From, `OnPieceEnter` at To).
6. **Who makes it:**
   - **A player:** a `UTurnBasedPlayerAction` subclass (like `UConnectIt_PlacePieceAction`) builds the operation and calls
     `RequestBoardChange(FInstancedStruct::Make(Operation))`, and overrides `ProducesRequestType` to say yes to your
     operation tag. Make it usable as a permanent action (a loadout's `PermanentActions`) and/or an item (the item
     inventory asset) ([[UnrealTurnBasedMechanics/code/recipes/add-a-turn-action|add-a-turn-action]]).
     A field the client could abuse (like Shift's `Steps`) needs a check in the Mediator's `ProcessRequest` against
     the sending action.
   - **The Level director:** **Apply Board Operation** with it ([[script-a-level]]).
   - **A tile power:** its effect returns it from `MakeOperations` ([[add-a-tile-power]]).

## Verify

- `ConnectIt.AI.Candidates` counts the kinds of operation (`GetAllOperationTags`, found by reflection): raise its
  expected count by one.
- Add a test like `ConnectIt.Board.MoveAndMultiplier`: allowed and refused cases (locked tiles included), what
  `OutChanges` reports, the event, and **a replay** (previous + `ApplyTo` of every event = the board after;
  `HasSameTilesAndScores`). `ConnectIt.Board.Events.Replay` shows the shape.
- PIE: the change happens on the server and replicates; the event tag fires on both machines; a refused request logs
  its reason and the action re-enables.

## Pitfalls

- **An event without `ApplyTo`** -- the display board drifts, and when the queue drains it snaps to the real board with
  a warning.
- **Reporting less than you changed in `OutChanges`** -- nothing scores or reacts to the unreported part (a moved
  piece that isn't an Enter never scores).
- **A field that isn't a `UPROPERTY`** doesn't travel with the request.
- **Mutating the board outside `Apply`** -- the Mediator works on a copy and commits it with `SetBoardState`; never
  write `GetCurrentState()`.
- **`FInstancedStruct::Make(Operation)` with a base-typed variable** slices it -- make it from the derived type.
- **Faction is client-supplied in a request** -- the server overwrites it; don't trust it in `CanApply` before then.

## See also

- [[UConnectIt_BoardRequestMediator|UConnectIt_BoardRequestMediator]] ·
  [[UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]] ·
  [[ConnectItStructs|ConnectItStructs]]
- Decisions: [board operation is the request](../../_decisions/2026-10-07-board-operation-is-the-request.md),
  [board events are structs carried by the queue](../../_decisions/2026-10-07-board-events-are-structs-carried-by-the-queue.md),
  [the tile lock](../../_decisions/2026-10-23-tile-lock-per-operation-rule.md)
- In-repo: [[ConnectIt/CLAUDE|ConnectIt overview]].
