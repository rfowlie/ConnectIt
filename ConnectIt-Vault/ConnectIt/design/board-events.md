# Board events: one struct per event, in an ordered list, each carried through the event queue

## Context
`FConnectItBoardChangeEvent` is one flat struct with a bool and a few fields for every kind of thing that can happen
to the board. It grows with every new operation, mixes what a player's operation did (piece placed, board shifted)
with what followed (scored, game won), and can only describe one of each per board change. The board state component
queues one gameplay tag per set bool, but the tags carry no data: every listener reads the single latest change
event, so two events of one kind can't be told apart and a second board change arriving mid-animation is read by the
first one's listeners.

Owner's direction: separate events into individual polymorphic structs, categorise them (what an operation did /
what resulted), and process them one at a time. Owner's choices: an event's data is **carried by the event queue**
(small plugin change); the flat fields are **replaced outright**.

Supersedes the 2026-09-20 decision "board change event is an ordered step list" in one respect: steps are
polymorphic structs, not one flat generic struct (UE 5.5 Blueprint can read an instanced struct directly).

Step 0 after approval: move this file to `ConnectIt-Vault/ConnectIt/design/board-events.md` and delete the staging copy.

## 1. Event structs (new `Public/Board/Events/ConnectIt_BoardEvents.h` + `.cpp`)
Plain data, all fields `UPROPERTY(BlueprintReadOnly)` (they replicate and are read in Blueprint).

```cpp
USTRUCT(BlueprintType) struct FConnectItBoardEvent            { virtual FGameplayTag GetEventTag() const; };
USTRUCT(BlueprintType) struct FConnectItBoardOperationEvent : FConnectItBoardEvent {};  // what an operation did
USTRUCT(BlueprintType) struct FConnectItBoardResultEvent    : FConnectItBoardEvent {};  // what followed from it
```

| Event | Category | Fields (taken from today's flat fields) | Tag (existing) |
| --- | --- | --- | --- |
| `_BoardSeeded` | (none) | -- | `ConnectIt.Event.BoardSeeded` |
| `_PiecePlaced` | Operation | `Position`, `Faction` | `PiecePlaced` |
| `_PiecesSwapped` | Operation | `PositionA`, `PositionB` | `PiecesSwapped` |
| `_BoardShifted` | Operation | `Direction`, `AnchorPosition`, `StartPositions`, `EndPositions` | `BoardShifted` |
| `_PieceCaptured` | Operation | `Position`, `CapturingFaction`, `PreviousFaction` | `PieceCaptured` |
| `_PieceRemoved` | Operation | `Position`, `RemovedFaction` | `PieceRemoved` |
| `_TileMultiplierDestroyed` | Operation | `Position` | `TileMultiplierDestroyed` |
| `_TileActiveToggled` | Operation | `Position`, `bNowActive` | `TileActiveToggled` |
| `_Scored` | Result | `Faction`, `Points`, `Positions` | `Scored` |
| `_GameWon` | Result | `WinningFaction` | `PlayerWin` |

"Operation" rather than "action" because "action" already means the player-side input classes. `_Scored` replaces
`FConnectItScoringConfiguration` (same three fields), which is deleted. Tags are unchanged, so tag-only listeners
(`CI_GridTile`, the game modes, `CI_Action_PlacePiece`) are unaffected.

## 2. The change event becomes the ordered list
`FConnectItBoardChangeEvent` (`Public/ConnectIt_Structs.h`) keeps its name and its place in the replicated snapshot,
but its content becomes:
```cpp
UPROPERTY(BlueprintReadOnly) TArray<FInstancedStruct> Events;   // in the order they happened
template<typename T> void Add(const T& Event);                  // C++ helper
```
All flat bools/fields and the scoring helpers are removed.

## 3. Who appends what (order = what happened)
- **Operations** (`ConnectIt_BoardOperation.h`, `ConnectIt_BoardOperations.h/.cpp`): `Apply(Board, OutTouched,
  FConnectItBoardChangeEvent* OutEvents)` appends its own Operation event (null from the AI's search, as today).
- **Scoring** (`ConnectIt_ScoringRule.h`, `ConnectIt_LineScoringRule.*`, `ConnectIt_RuleSet.*`): `ApplyScoring` /
  `ResolveBoardChange` take the same optional `FConnectItBoardChangeEvent*` and append one `_Scored` per thing that
  scored (replacing the `TArray<FConnectItScoringConfiguration>*` output).
- **Mediator** (`ConnectIt_BoardRequestMediator.cpp`): after `StampWinState`, appends `_GameWon` on the transition
  into game over. Its dormant `CreateGameEventsFromBoardUpdate` check for "a piece was placed" reads the list.
- **Board seeding** (`ConnectIt_BoardStateComponent.cpp`): the initial snapshot's list holds `_BoardSeeded`.

## 4. Event queue carries a payload (plugin `UnrealGameMechanics`, `GameEvent/GameEventTaskSubsystem.h/.cpp`)
- Queue entries become `{ FGameplayTagContainer Tags; FInstancedStruct Payload; }`.
- New `QueueTagContainerWithPayload(Tags, Payload)`; existing `QueueTagContainer(Tags)` queues an empty payload, so
  current callers and Blueprints are unchanged.
- New `BlueprintPure GetActivePayload()`: the payload of the entry being processed now (empty when idle or none).
  Set when an entry becomes active, cleared when it completes.
- Generic: the plugin knows nothing about board events.

## 5. Processing one at a time (`ConnectIt_BoardStateComponent.*`)
- `EnqueueBoardEventTags` becomes a loop: for each event in the snapshot's list, in order, queue its tag with the
  event as payload. The hardcoded if-ladder goes. The queue already waits for one entry's visuals before the next.
- New `BlueprintPure GetActiveBoardEvent()` (returns the queue's active payload) so Blueprint listeners have one
  obvious place to ask; they then use Get Instanced Struct Value for the event type they handle.
- `GetChangeEvent()` stays (whole list of the latest change: debug widgets, tests).

## 6. C++ consumers
- `ConnectIt_PieceRegistry.cpp`: `HandleBoardShifted` / `HandleBoardPiecesSwapped` / `HandleBoardPieceRemoved` read
  the active event (`_BoardShifted` / `_PiecesSwapped` / `_PieceRemoved`) instead of the latest change event.
- Debug widgets (`ConnectIt_DebugStateWidget`, `DWidget_ConnectIt_BoardStateComponent`) keep passing the struct through.

## Behaviour changes to know
- **Each completed line is its own Scored event**, played one after another, instead of one Scored event covering
  all of them.
- An event's listeners see that event's data even if another board change has arrived since.
- **`CI_PieceVisualHandler` breaks on every change-event pin** until rewired: each tag handler calls
  `GetActiveBoardEvent` → Get Instanced Struct Value (its event type) → the same fields under their new names.

## Tests (`Private/Tests/ConnectIt_MinMaxTests.cpp`)
- Port the operation tests: assert the appended event's type and fields instead of flat fields.
- Scoring tests: `_Scored` events instead of configurations (two crossing lines → two events, in order after the
  placement event).
- New `ConnectIt.Board.Events.Order`: apply a placement that completes a line through the same calls the Mediator
  makes; the list is `[PiecePlaced, Scored]`, and category checks by base type hold.
- The queue payload and the component's enqueue loop need a world; not unit-tested.

## Vault
Step 0; decision note "board events are polymorphic structs carried by the queue" (supersedes the flat-step part of
2026-09-20 board-change-event-is-an-ordered-step-list; mark it partly superseded); update
`design/board-request-objects.md` (step list now built); log + indexes in both `ConnectIt` and `UnrealGameMechanics`
domains (plugin change). Task rows left to the owner.

## Verification
- Build (editor closed); `Automation RunTests ConnectIt` all pass; Throughput unchanged (search passes no event list).
- Owner, editor: rewire `CI_PieceVisualHandler`; confirm `CI_GridTile`, the game modes and `CI_Action_PlacePiece`
  still fire on their tags.
- Owner, PIE 2-player: place, swap, shift animate; a move completing two lines plays two score events in turn; win
  still ends the match on both machines; vs AI unchanged.

## Status (2026-10-07): built, not yet played

- Implemented as planned. Build clean; `ConnectIt` tests 20/20; Throughput 392k nodes/s (unchanged).
- Beyond the plan: `FConnectItBoardChangeEvent::FindFirst<T>()`; a C++ `GetActiveBoardEventAs<T>()` on the board
  state component; and in the plugin the trigger loop iterates a copy of the active tags (a tag whose sequence
  completes at once used to remove itself from the container being iterated).
- `UnrealGameMechanics` has no `_logs/` in the vault, so the plugin change is logged under `ConnectIt` only.
- Open: rewire `CI_PieceVisualHandler`; the PIE checks under Verification.
- Decision: [board events are structs carried by the queue](../_decisions/2026-10-07-board-events-are-structs-carried-by-the-queue.md).

## Follow-up (2026-10-07): typed Blueprint getters for the board event being played

### Context
Board events are now polymorphic structs played one at a time, and a Blueprint listener reads the event being played
through the untyped `GetActiveBoardEvent` + a Get Instanced Struct Value node. The listener has to know which struct
type goes with which gameplay tag; picking the wrong one fails quietly (the node's Not Valid pin, usually unwired).
Alternatives discussed: a bespoke event system on the board state (rejected: duplicates the queue), typed delegates
(a facade; data would arrive at tag begin rather than when a gated task runs). Owner's choice: **typed getters**.

Step 0 after approval: append this as a section to `ConnectIt-Vault/ConnectIt/design/board-events.md` and delete the
staging copy.

### Change (`Public/Board/ConnectIt_BoardStateComponent.h`, `Private/Board/ConnectIt_BoardStateComponent.cpp`)
- One Blueprint function per event type that carries data (nine; Board Seeded has none):
  `GetActivePiecePlacedEvent`, `GetActivePiecesSwappedEvent`, `GetActiveBoardShiftedEvent`,
  `GetActivePieceCapturedEvent`, `GetActivePieceRemovedEvent`, `GetActiveTileMultiplierDestroyedEvent`,
  `GetActiveTileActiveToggledEvent`, `GetActiveScoredEvent`, `GetActiveGameWonEvent`.
  ```cpp
  UFUNCTION(BlueprintCallable, Category = "Board State|Events", meta = (ExpandBoolAsExecs = "ReturnValue"))
  bool GetActivePiecePlacedEvent(FConnectItBoardEvent_PiecePlaced& OutEvent) const;
  ```
  In Blueprint each is a node with **True / False** execution pins and the typed struct as output -- no conversion
  node, and the failure path is visible on the node.
- All nine go through one private template helper. On a mismatch it returns false and logs an error naming the
  function called and the event actually being played (or "no board event is being played").
- The untyped `GetActiveBoardEvent()` Blueprint function is removed (nothing uses it yet); C++ keeps
  `GetActiveBoardEventAs<T>()`. The plugin's generic `GetActivePayload` is untouched.
- Header comment on `ConnectIt_BoardEvents.h`: a new event with data also gets a getter here.

### Not changed
No new queue, no delegates, no plugin change. Tag-only listeners keep using the subsystem.

### Vault
Step 0; short status line in `design/board-events.md`; update the "For Blueprint" wording in today's board-events
decision note; log + index. No new decision note (a refinement of today's decision).

### Verification
- Build (editor closed); `Automation RunTests ConnectIt` still 20/20 (the getters need a running world, so no new
  unit test).
- Owner, editor: the nine nodes appear under **Board State > Events** with True/False pins; rewire
  `CI_PieceVisualHandler` with them. Calling one from the wrong tag's handler logs the mismatch error in PIE.

### Status: built

Implemented as planned. Build clean; `ConnectIt` tests 20/20. The getters need a running world, so they are not
unit-tested; to be confirmed in the editor while rewiring `CI_PieceVisualHandler`.

## Follow-up 2 (2026-10-07): the typed getters move into a Blueprint function library

### Context
The nine `GetActive...Event` functions were added to `UConnectIt_BoardStateComponent`, but they read the game event
queue's active payload and use nothing from the board state. In Blueprint that forces a "get board state component"
step before every call. Owner's request: move them to a ConnectIt Blueprint function library (static functions).

Step 0 after approval: append this as a section to `ConnectIt-Vault/ConnectIt/design/board-events.md` and delete the
staging copy.

### Change
- **New** `UConnectIt_BoardEventLibrary` (`Public/Framework/Library/ConnectIt_BoardEventLibrary.h`,
  `Private/Framework/Library/ConnectIt_BoardEventLibrary.cpp`), next to the existing `ConnectIt_BoardStateLibrary`
  and `ConnectIt_GameUtilityLibrary`:
  - The same nine functions, static, with the same names and behaviour (True/False pins, typed struct output, error
    logged on a mismatch):
    ```cpp
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board Events",
        meta = (WorldContext = "WorldContextObject", ExpandBoolAsExecs = "ReturnValue"))
    static bool GetActivePiecePlacedEvent(const UObject* WorldContextObject, FConnectItBoardEvent_PiecePlaced& OutEvent);
    ```
    The world-context pin is filled in automatically in Blueprint, so the node needs no target and no extra wiring.
  - C++ form `GetActiveBoardEventAs<T>(WorldContextObject)` (no error logged), plus the private shared helper and
    mismatch logger, moved over unchanged.
- **`UConnectIt_BoardStateComponent`** (`.h` / `.cpp`): the nine getters, `GetActiveBoardEventAs`, the private
  helpers and the events-header include are removed. `GetChangeEvent()` stays.
- **`ConnectIt_PieceRegistry.cpp`**: the shift / swap / remove handlers call
  `UConnectIt_BoardEventLibrary::GetActiveBoardEventAs<T>(this)`; they no longer fetch the board state component,
  which they only used for this.
- Comments in `ConnectIt_BoardEvents.h` and the component point at the library.

No behaviour change, no plugin change. Nothing in Blueprint uses the component versions yet, so nothing breaks.

### Vault
Step 0; update the function location in today's board-events decision note; log + index.

### Verification
- Build (editor closed); `Automation RunTests ConnectIt` still 20/20.
- Owner, editor: in any Blueprint graph, the nine nodes appear under **ConnectIt > Board Events** with no target
  pin; use them when rewiring `CI_PieceVisualHandler`.

### Status: built

Built and tested later the same day (log 2026-10-07-1657): build clean, `ConnectIt` tests 20/20.

Earlier note, kept for the record:

The edits are in the working tree, but the build was refused because the editor was open with Live Coding active.
The automation run that followed used the previous binaries, so it says nothing about this change. Needs a build
with the editor closed, then the test run.
