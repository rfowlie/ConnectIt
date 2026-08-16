# Runtime State Access Reference

A reference for building UI (in-game or debug/dev) that reads ConnectIt's
live game state — board, turn/participant, match, and action-stack state.
For what each class *is*, see [ConnectItModule.md](ConnectItModule.md); this
doc is about where to read a given piece of *state* from, whether it's safe
to read from Blueprint, and where the gaps are.

Current as of the `TurnEndEventTag`/`QueueTagContainer` refactor that retired
`UConnectIt_BoardSequencerComponent` and the old resolution-hold mechanism
(see [Turn-End & Resolution Sequencing](#turn-end--resolution-sequencing)
below) — this replaced an earlier design and this doc reflects the current
one only.

`UConnectIt_DebugStateWidget` (`Source/ConnectIt/Public/UI/ConnectIt_DebugStateWidget.h`)
is a prototype that wires up every accessor and delegate documented below
into one cached-state widget — a working, cross-referenced example rather
than just a table to read.

**BP Access column key:** `Pure` = `BlueprintPure` function. `Callable` =
`BlueprintCallable` (has side effects, or plugin marks a query this way
anyway — noted per row). `ReadOnly` = plain `UPROPERTY(BlueprintReadOnly)`
(or `EditAnywhere, BlueprintReadOnly` for config). `— (C++ only)` = no
`UFUNCTION`/`UPROPERTY` at all; not reachable from Blueprint as written.

## Board State

Source of truth: `UConnectIt_BoardStateComponent` (one per `AConnectIt_BoardManager`, the single replicated property `BoardSnapshot`).

| What | Accessor | Where | BP Access |
|---|---|---|---|
| Full current board (tiles, scores, game-over) | `GetCurrentState()` → `const FConnectItBoardState&` | `UConnectIt_BoardStateComponent` | Pure |
| Board state before the most recent change | `GetPreviousState()` | same | Pure |
| What just changed (piece placed/scored/won/shifted) | `GetChangeEvent()` → `const FConnectItBoardChangeEvent&` | same | Pure |
| Both states + change event together | `GetBoardSnapshot()` → `const FConnectItBoardStateSnapshot*` | same | — (C++ only, returns a raw pointer) |
| Per-tile queries (occupied/active/valid-for-placement, score) | `FConnectItBoardState::IsTileOccupied/IsTileActive/IsTileValidForPlacement/GetScore` | inline on the struct (`ConnectIt_Structs.h`) | — (C++ only, no loose BP nodes) |
| High-level board queries (all tiles, empty tiles, tiles by faction, random empty tile, board full, has-faction-won) | `UConnectIt_GameUtilityLibrary::Get*GridTiles* / IsTileEmpty / GetTileAtPosition / GetRandomEmptyGridTile / IsGameBoardFull / HasFactionWon` (static, `WorldContextObject`) | `Library/ConnectIt_GameUtilityLibrary.h` | Pure |
| Board manager / board state component lookup | `UConnectIt_GameUtilityLibrary::GetBoardManager` / `GetBoardStateComponent` | same | Pure |
| Grid position ↔ world position, rows/columns, board dimensions | `UGridTileRegistryComponent::GridPositionToWorld/WorldToGridPosition/GetRow/GetColumn/GetMinRow.../GetRowCount/GetColumnCount` | `AConnectIt_BoardManager::GetTileRegistry()` | Pure |
| Piece actor at a grid position | `UGridPieceRegistryComponent::GetPiece(Position)` | `AConnectIt_BoardManager::GetPieceRegistry()` | Pure |
| Config (loadouts, piece class, pool size) | `AConnectIt_BoardManager::GetConfigComponent()` → `UConnectIt_ConfigComponent` properties | same | Getter: Pure. Properties: ReadOnly (`EditAnywhere, BlueprintReadOnly`) |
| Scoring/win-condition strategy in use | `AConnectIt_BoardManager::GetBoardRulesComponent()` → `UConnectIt_BoardRulesComponent` | same | Getter: Pure. No BP query for "which strategy" — see [Gaps](#gaps--recommendations) |

`GetBoardStateComponent()` on the board manager (`BlueprintPure`) is the
one-hop way to reach all of the above from anywhere that already has (or can
resolve) the board manager.

## Turn / Participant / Match State

Source of truth: `UTurnBasedParticipantManagerComponent` (lives on GameState).

| What | Accessor | Where | BP Access |
|---|---|---|---|
| Current turn phase | `ATurnBasedGameState::GetCurrentTurnPhase()`, or `UConnectIt_GameUtilityLibrary::GetCurrentTurnPhase` | convenience wrapper (preferred over reaching into the manager directly) | Pure |
| Active participant slot index | `ATurnBasedGameState::GetActiveParticipantIndex()` | same | Pure |
| Match-wide turn number | `ATurnBasedGameState::GetActiveTurnNumber()` | same | Pure |
| Full participant list (name, slot, connected, turns taken, type) | `ATurnBasedGameState::GetParticipants()` → `const TArray<FTurnParticipantInfo>&` | same | Pure |
| Turn duration config | `ATurnBasedGameState::GetTurnDuration()` | same | Pure |
| Match phase (WaitingForParticipants/InProgress/Paused/GameOver) | `ATurnBasedGameState::GetMatchPhase()` | same | Pure |
| Per-player forfeits/turns-missed/ready state | `ATurnBasedPlayerState::GetTurnsMissed/IsForfeited/IsReady/GetSlotIndex/GetParticipantType` | on each participant's PlayerState (`FTurnParticipantInfo::PlayerState`) | Pure |
| "Is it my turn" (local player) | `UConnectIt_GameUtilityLibrary::IsLocalPlayerTurn` / `GetLocalPlayerSlotIndex` | static utility | Pure |
| Match result (winner, final scores, end reason) | `AConnectIt_GameState::MatchResult` (property) | ConnectIt-specific | ReadOnly (`ReplicatedUsing`) |
| Board-derived UI convenience (score, tile occupied, active participant name) | `AConnectIt_GameState::GetFactionScore/GetAllScores/IsTileOccupied/IsTileValidForPlacement/GetActiveParticipantName/IsLocalPlayerTurn` | ConnectIt-specific | Pure |
| Raw controller at a slot index | `UTurnBasedParticipantManagerComponent::GetControllerAtIndex(int32)` | — | — (C++ only, no `UFUNCTION`) — also returns null on clients per its own doc comment, so not useful to a client-side debug widget anyway |

Prefer the `ATurnBasedGameState`/`UConnectIt_GameUtilityLibrary` wrappers over
reaching directly into `UTurnBasedParticipantManagerComponent`'s replicated
properties — same data, but the wrappers are null-safe (return a sane default
if the manager or game state isn't resolved yet) where the raw properties
aren't guarded at all.

## Action / Stack State

Source of truth: `UTurnBasedActionsComponent` (one per PlayerController — reach it via `ATurnBasedPlayerControllerBase::ActionsComponent`, `BlueprintReadOnly`).

| What | Accessor | Where | BP Access |
|---|---|---|---|
| What's currently active (top of stack) | `GetTopAction()`, `GetRootAction()`, `IsRootOnTop()`, `GetStackDepth()` | `UTurnBasedActionsComponent` | Pure |
| All configured actions / which are required | `GetAllRuntimeActions()`, `GetRequiredActions()`, `FindActionByTag()` | same | Pure |
| Is a board-change request in flight | `IsAwaitingRequestConfirmation()` | same | Pure |
| Per-action active state | `IsActive()` | on the `UTurnBasedAction`/`UTurnBasedActionBase` instance itself | Pure |
| Per-action completions/cooldown/config (completions this turn, turns until available, required/cancellable, tag) | `CompletionsThisTurn`, `TurnsUntilAvailable`, `bIsRequired`, `bIsCancellable`, `ActionTag` | same | ReadOnly |

**Caveat carried over from the safety review below:** `GetTopAction()` /
`GetRootAction()` hand back the *live* action object — `Complete()`/`Cancel()`
are `BlueprintCallable` on that same pointer, so it's not a pure read-only
handle even though everything listed above it is.

## Turn-End & Resolution Sequencing

This is the mechanism that was reworked most recently — worth its own
section since it's easy to reach for a stale mental model here.

**What used to exist (removed):** `UTurnBasedParticipantManagerComponent`
used to expose `BeginResolutionHold()`/`EndResolutionHold()` and a private
`ResolutionHoldCount`, with `AConnectIt_BoardManager` mediating via
`HandleTurnResolutionStarted`/`BeginTurnEndSequence`/
`HandleTurnEndSequenceComplete` and a dedicated `UConnectIt_BoardSequencerComponent`
subobject. **All of this is gone.** `BoardSequencerComponent` is deprecated
in place (kept only so old serialized BP data referencing the class name
doesn't break — never instantiated anymore, `AConnectIt_BoardManager`
dropped the subobject entirely) and `ResolutionHoldCount` no longer exists
anywhere in the codebase.

**What replaced it:**
- `UConnectIt_BoardStateComponent::EnqueueBoardEventTags()` (private) reads
  the just-recorded `ChangeEvent` and calls `UGameEventTaskSubsystem::
  QueueTagContainer()` once per relevant tag (Shift, or PiecePlaced → then
  conditionally LineScored/PlayerWin), called symmetrically from both
  `SetBoardState` (server) and `OnRep_BoardSnapshot` (client) — right after
  `BroadcastChange()`.
- `UTurnBasedParticipantManagerComponent::EndTurn` enqueues its own
  `TurnEndEventTag` the same way, via the same `QueueTagContainer` call.
- `AdvanceToNextParticipant` is bound *once*, in `BeginPlay`, as
  `TurnEndEventTag`'s `OnManagerComplete` listener (`GameEventSubsystem->
  BindOnTagComplete(TurnEndEventTag, this, ...AdvanceToNextParticipant)`) —
  it fires automatically whenever that tag's manager finishes, no manual
  hold/release bookkeeping involved.

**Why ordering still works without a hold counter:** `GameEventTaskSubsystem`
has one global `ContainerQueue` — `QueueTagContainer` calls are strictly
FIFO, and the next queued container can't even *start* firing until every
tag in the current one has fully completed (`OnManagerComplete` for all of
them). Since the board-event tags get enqueued first (from `SetBoardState`)
and `TurnEndEventTag` gets enqueued after (from `EndTurn`, which itself only
runs after the request that changed the board has already finished being
processed), `TurnEndEventTag`'s manager literally cannot begin executing
until every board-event tag ahead of it in the same queue is done. The old
hold-counter was manually recreating a guarantee the single shared queue now
provides for free.

**Debug visibility today:** "is a turn-end/resolution sequence currently
pending" has no direct equivalent to the old `ResolutionHoldCount`, but the
subsystem now exposes exactly this purpose-built for UI (added after the
gap below was first flagged — no longer a gap):

| What | Accessor | Where | BP Access |
|---|---|---|---|
| Tags currently firing/in-flight, right now | `GetTagsInQueue()` → `TArray<FGameplayTag>` | `UGameEventTaskSubsystem` | Callable |
| Fires whenever that set changes (a tag completes) | `OnActiveManagerTagsChanged` (no params — re-call `GetTagsInQueue()` on fire to get the new set) | same | Assignable |

`GetTagsInQueue()` — despite the name — returns `ActiveManagerTags`, the set
of tags still outstanding for the *currently executing* container, not the
full backlog of not-yet-started `ContainerQueue` entries; "is `TurnEndEventTag`
in this array" is the direct answer to "is turn-end sequencing active right
now." `OnActiveManagerTagsChanged` broadcasts once per tag completion (i.e.
potentially several times while a multi-tag container drains), not once per
container — a listener that only cares about "idle vs. not" should check
`GetTagsInQueue().IsEmpty()` on each fire rather than assume one call means
the whole container finished.

(The subsystem no longer implements `IGameEventTaskHandler`/
`GetGameEventTaskManagerByTag` — that per-tag-manager route documented in an
earlier version of this doc is gone. Use the table above instead.)

Still not reachable: the not-yet-started backlog in `ContainerQueue` itself
(how many containers are queued *behind* the active one, and what tags
they'll fire) — `GetTagsInQueue()` only covers what's active right now.

## Gaps & Recommendations

1. ~~`GameEventTaskSubsystem`'s own queue state has no BP surface at all.~~
   **Resolved** — `GetTagsInQueue()` + `OnActiveManagerTagsChanged` (see
   [Turn-End & Resolution Sequencing](#turn-end--resolution-sequencing))
   now cover the active-tag half of this, which was the debug-relevant half.
   Still open, lower priority: the not-yet-started `ContainerQueue` backlog
   itself has no accessor (how many containers are queued *behind* the
   active one). Worth a `GetQueuedContainerCount()` if the debug UI ever
   needs to show "N more things about to happen" rather than just "this is
   happening right now."
2. **No single-call "active participant's full info."** BP has to pull
   `GetParticipants()` + `GetActiveParticipantIndex()` and index in itself.
   Minor convenience gap — a `GetActiveParticipant()` wrapper on
   `ATurnBasedGameState` (mirroring its existing passthrough accessors)
   would close it cheaply.
3. **No BP-facing query for which scoring/win-condition strategy is active**
   on `UConnectIt_BoardRulesComponent` — low priority, mostly a config-time
   concern rather than a runtime-debug one.

## Safety — reading this state from Blueprint

Confirmed safe by construction for the vast majority of the surface above:

- `BlueprintPure` functions returning `const FStruct&` (board state, change
  event, participant list) — Blueprint always copies struct data out to the
  pin; there's no way for a widget to hold a live reference back into
  replicated state and mutate it through a read node.
- Plain `UPROPERTY(BlueprintReadOnly)` value-type properties (`CurrentPhase`,
  `TurnNumber`, `ScoreBoard`, etc.) — BP `Get` nodes on non-pointer
  properties copy the value out the same way.

**One real caveat:** several accessors return *live, mutable UObject
pointers* — `GetTopAction()`/`GetRootAction()` (actions), `GetBoardManager()`,
component getters (`GetTileRegistry()`, `GetConfigComponent()`, etc.),
`GetPiece()`. None of the checked components expose unintended
`BlueprintCallable` mutators alongside their query surface (`UGridTileRegistryComponent`/
`UGridPieceRegistryComponent` are pure-query; `UConnectIt_ConfigComponent`/
`UConnectIt_BoardRulesComponent` have no BP-callable mutators at all) — but
`UTurnBasedAction::Complete()`/`Cancel()` genuinely are `BlueprintCallable` on
the exact object `GetTopAction()` hands back. A debug widget built around
that pointer should be deliberate about which nodes it wires off it — the
framework itself isn't unsafe, the risk is purely "this read-only-looking
pointer also has mutating functions on it, don't wire those in by accident
under a 'read only' debug panel."
