# UI Value Catalogue

A catalogue of every value production in-game UI (scoreboard, turn indicator,
results screen, board overlay, action bar, connection/lobby state) would want
to display, organized by **UI value** rather than by system. For *why* a
value is safe to read from Blueprint, or the sequencing model behind it, see
[RuntimeStateAccess.md](RuntimeStateAccess.md) — that doc stays organized by
*system* (board/turn/action) and owns those semantics; this doc **links to
it** for shared facts rather than restating them. Where this doc and that one
overlap, this one adds only the columns that matter for building a
player-facing widget: net availability and what signals a change.

This doc does **not** cover the `DWidget_*` debug-widget family — see
[Workflows/DebugWidgets.md](Workflows/DebugWidgets.md) for that pattern.
**Production widgets should not follow it.** `GetInfo()` is a debug-seeding
convention — one struct mirroring everything a single class holds, so a
debug widget can never silently go stale. A production widget composes a
handful of values from *several* classes at once (a scoreboard row is
participant + score + faction palette + local-player-ness) — exactly the
case DebugWidgets.md's own rule says never to fold into one `GetInfo()`.
Production widgets should read the narrow, purpose-built `BlueprintPure`
accessors below directly, so reshaping a debug `F*Info` struct (as this
catalogue's own Gap 5 fix does) never ripples into production UI.

## Column key

| Column | Meaning |
|---|---|
| **UI Value** | What a player sees, not a field name. |
| **Owning Class** | Source of truth. |
| **C++ Accessor** | Exact signature. |
| **BP Accessor** | Exact node, `none`, or `GAP #N` (see [Gaps](#gaps-closed-by-this-catalogue) below — `[fixed]` marks ones already closed while writing this doc). |
| **Net Availability** | `All` (every client) · `Active client only` · `Server only` · `Local only` (per-machine, not networked at all) · `Config` (identical everywhere, authored not replicated). |
| **Change Signal** | The delegate/tag that fires on change, or `poll only`. |
| **Recommended Read-Only Home** | Where a widget should route the read. Closed vocabulary: `GameState` · `ParticipantManager` · `BoardStateComponent` · `PlayerController registry` · `ActionsComponent` · `ParticipantComponent` · `PlayerState` · `UtilityLibrary` · `BoardStateLibrary` · `FactionVisualsSubsystem` · `GameEventTaskSubsystem`. |

---

## §A — Scoreboard / participant roster

*One row per participant.*

| UI Value | Owning Class | C++ Accessor | BP Accessor | Net Availability | Change Signal | Home |
|---|---|---|---|---|---|---|
| Faction slot index | `FTurnParticipantInfo` | `.SlotIndex` | `BlueprintReadOnly` | All | `Participants` changes | ParticipantManager |
| Display name | `FTurnParticipantInfo` | `GetDisplayName()` → delegates to `UTurnBasedMechanicsLibrary::GetParticipantDisplayName` | `UTurnBasedMechanicsLibrary::GetParticipantDisplayName(Participant, Fallback)` (`BlueprintPure`) — `GAP 2 [fixed]` | All | `PlayerState`'s own name replication | UtilityLibrary |
| Participant type (Human/AI/Observer) | `FTurnParticipantInfo` | `.ParticipantType` | `BlueprintReadOnly` | All | `Participants` changes | ParticipantManager |
| Connected | `FTurnParticipantInfo` | `.bConnected` | `BlueprintReadOnly` | All | `Participants` changes (no dedicated delegate) | ParticipantManager |
| Ready | `ATurnBasedPlayerState` | `IsReady()` | `BlueprintPure` | All | `OnReadyChanged` | PlayerState |
| Forfeited | `ATurnBasedPlayerState` | `IsForfeited()` | `BlueprintPure` | All | `OnForfeited` | PlayerState |
| Turns missed | `ATurnBasedPlayerState` | `GetTurnsMissed()` | `BlueprintPure` | All | `OnTurnsMissedChanged` | PlayerState |
| Turns taken (**this** participant's own count — not match-wide `TurnNumber`) | `FTurnParticipantInfo` | `.TurnsTaken` | `BlueprintReadOnly` | All | `Participants` changes | ParticipantManager |
| Current score | `FConnectItBoardState` | `GetScore(FactionSlot)` (inline, not BP-callable) | `AConnectIt_GameState::GetFactionScore(int32)` (`BlueprintPure`) | All | `OnBoardStateChanged` (board manager) | GameState |
| Score delta vs. previous state | `FConnectItBoardState` (current vs. previous) | manual diff | none | All | `OnBoardStateChanged` | `GAP 4` → BoardStateLibrary |
| Is active participant | `FTurnParticipantInfo` | `.IsActiveParticipant()` (plain C++) | none directly — derivable via `ActiveParticipantIndex == SlotIndex` | All | `OnParticipantIndexChanged` | ParticipantManager (compare indices) |
| Is local player | `UConnectIt_GameUtilityLibrary` | `IsLocalPlayerTurn` / `GetLocalPlayerSlotIndex` | `BlueprintPure` | Local only (each client answers for itself) | poll only | UtilityLibrary |
| Faction colour / icon / short label | — | — | — | Config | n/a | `GAP 1` (deferred — see below) → FactionVisualsSubsystem |
| Progress toward win (score / target) | `AConnectIt_GameState` | `GetFactionScoreProgress(int32)` | `BlueprintPure` — `GAP 3 [fixed]` | All | `OnBoardStateChanged` | GameState |

## §B — Turn indicator / turn HUD

| UI Value | Owning Class | C++ Accessor | BP Accessor | Net Availability | Change Signal | Home |
|---|---|---|---|---|---|---|
| Match-wide turn number | `ATurnBasedGameState` | `GetActiveTurnNumber()` | `BlueprintPure` | All | `OnTurnChanged` (`ParticipantManager`) | GameState |
| Turn phase | `ATurnBasedGameState` | `GetCurrentTurnPhase()` | `BlueprintPure` | All | `OnTurnPhaseChanged` | GameState |
| Active participant slot index | `ATurnBasedGameState` | `GetActiveParticipantIndex()` | `BlueprintPure` | All | `OnParticipantIndexChanged` | GameState |
| Active participant, full row, one call | `ATurnBasedGameState` | `GetActiveParticipant(bool&)` | `BlueprintPure` — `GAP 6 [fixed]` | All | `OnParticipantIndexChanged` | GameState |
| Active participant name | `AConnectIt_GameState` | `GetActiveParticipantName()` | `BlueprintPure` | All | `OnParticipantIndexChanged` | GameState |
| Is it my turn | `UConnectIt_GameUtilityLibrary` | `IsLocalPlayerTurn(WorldContextObject)` | `BlueprintPure` | Local only | `OnParticipantIndexChanged` (poll on fire) | UtilityLibrary — **not** `AConnectIt_GameState::IsLocalPlayerTurn()`; see [Fixed bugs](#fixed-while-writing-this-catalogue), both now agree |
| Configured turn duration | `UTurnBasedParticipantManagerComponent` | `.TurnDuration` (config) / `.ReplicatedTurnDuration` (per-turn) | `BlueprintReadOnly` | All | replicates with turn start | ParticipantManager |
| Seconds remaining, any participant's turn | `ATurnBasedGameState` | `GetTurnTimeRemaining(bool&)` / `GetTurnTimeRemainingFraction()` | `BlueprintPure` — `GAP 5 [fixed]` — derived from a replicated turn-start timestamp, no per-tick replication | All | poll (recompute each tick/frame — this is a continuously-changing value, not event-driven) | GameState |
| Last turn end reason | `FTurnNotification` | `.EndReason` | `BlueprintReadOnly` | Active client only (see above) | delivered with the notification | ParticipantComponent |
| Is a turn-end sequence running | `UGameEventTaskSubsystem` | `GetTagsInQueue()` — check whether `TurnEndEventTag` is in the array | `BlueprintCallable` | All | `OnActiveManagerTagsChanged` | GameEventTaskSubsystem |

## §C — Match / results screen

| UI Value | Owning Class | C++ Accessor | BP Accessor | Net Availability | Change Signal | Home |
|---|---|---|---|---|---|---|
| Match phase | `ATurnBasedGameState` | `GetMatchPhase()` | `BlueprintPure` | All | `OnMatchPhaseChanged` | GameState |
| Match over | `FConnectItMatchResult` | `.bMatchOver` | `BlueprintReadOnly` | All | `OnMatchResultUpdated` | GameState |
| Winning faction slot | `FConnectItMatchResult` | `.WinningFactionSlot` | `BlueprintReadOnly` | All | `OnMatchResultUpdated` | GameState |
| Final scores | `FConnectItMatchResult` | `.FinalScores` | `BlueprintReadOnly` | All | `OnMatchResultUpdated` | GameState |
| Total turns played | `FConnectItMatchResult` | `.TotalTurnsPlayed` | `BlueprintReadOnly` | All | `OnMatchResultUpdated` | GameState |
| Match end reason | `FConnectItMatchResult` | `.EndReason` | `BlueprintReadOnly` | All | `OnMatchResultUpdated` | GameState — **caveat:** `AConnectIt_GameMode::HandleGameOver` (`ConnectIt_GameMode.cpp:174`) hardcodes `EMatchEndReason::ScoreThresholdReached` on every win. `OpponentForfeited`/`OpponentDisconnected` exist on the enum but nothing in the live path ever writes them. Pre-existing gap, not part of this catalogue's fix list — a GameMode change, not a UI-bridge one. |
| Win score target | `FConnectItBoardState` / `AConnectIt_GameState` | `GetTargetScore()` | `BlueprintPure` — `GAP 3 [fixed]` | All | `OnBoardStateChanged` | GameState |
| Winner's name + colour | derived | `Participants[Winner].GetDisplayName()` + palette lookup by slot (colour half deferred, `GAP 1`) | see §A rows | All / Config | `OnMatchResultUpdated` | ParticipantManager + FactionVisualsSubsystem |

## §D — Board overlay *(drawn over the grid)*

*Prerequisite for this whole section:* `AConnectIt_PlayerController::GetTileRegistry()`/`GetPieceRegistry()` (`BlueprintPure`, relocated from the retired `AConnectIt_BoardManager`/`ABoardManagerBase` — see [ConnectItModule.md](ConnectItModule.md#board-architecture-overhaul)) return `UGridTileRegistryBase*`/`UGridPieceRegistryBase*` — **`Instanced` properties with no constructor default**, so they are null unless assigned per-Blueprint. Check validity before use. `UConnectIt_GameUtilityLibrary::GetTileRegistry()` is the preferred entry point — it resolves the local controller first, falling back to scanning every connected controller server-side.

| UI Value | Owning Class | C++ Accessor | BP Accessor | Net Availability | Change Signal | Home |
|---|---|---|---|---|---|---|
| Board dimensions (row/column counts, min/max) | `UGridTileRegistryBase` | `GetRowCount/GetColumnCount/GetMinRow/GetMaxRow/GetMinColumn/GetMaxColumn` | `BlueprintPure` | All (static per level) | none needed | PlayerController registry |
| All tile positions | `UGridTileRegistryBase` / `UConnectIt_GameUtilityLibrary` | `GetAllTilePositions()` / `GetAllGridTiles(WorldContextObject)` | `BlueprintPure` | All | none needed | UtilityLibrary |
| Grid ↔ world conversion | `UGridTileRegistryBase` | `GridPositionToWorld/WorldToGridPosition` | `BlueprintPure` | All | n/a | PlayerController registry |
| Tile actor at position | `UGridTileRegistryBase` / `UConnectIt_GameUtilityLibrary` | `GetTileAtPosition(Position)` | `BlueprintPure` | All | n/a | UtilityLibrary |
| Piece actor at position | `UGridPieceRegistryBase` | `GetPiece(Position)` | `BlueprintPure` | All | n/a | PlayerController registry |
| **Per-tile occupying faction** | `FConnectItBoardState` | `GetTileData(Position)->FactionPiece` (inline, not BP-callable) | none | All | `OnBoardStateChanged` | `GAP 4` |
| **Per-tile multiplier** | `FConnectItBoardState` | same struct, not BP-callable | none | All | `OnBoardStateChanged` | `GAP 4` |
| **Per-tile is-active** | `FConnectItBoardState` | same struct, not BP-callable | none | All | `OnBoardStateChanged` | `GAP 4` |
| Per-tile valid-for-placement | `AConnectIt_GameState` | `IsTileValidForPlacement(Position)` | `BlueprintPure` | All | `OnBoardStateChanged` | GameState |
| Scoring line positions (last score) | `FConnectItBoardChangeEvent` | `.ScoringLinePositions` (only when `.bLineScored`) | `BlueprintReadOnly` | All | rides `OnBoardStateChanged` | BoardStateComponent (`GetChangeEvent()`) |
| Last placed position + faction | `FConnectItBoardChangeEvent` | `.PlacedPosition` / `.PlacingFactionSlot` (when `.bPiecePlaced`) | `BlueprintReadOnly` | All | rides `OnBoardStateChanged` | BoardStateComponent |
| Removed / swapped / captured / toggled positions | `FConnectItBoardChangeEvent` | `.RemovedPosition` / `.SwapPositionA`+`B` / `.CapturedPosition` / `.ToggledPosition` (each gated by its own `bool`) | `BlueprintReadOnly` | All | rides `OnBoardStateChanged` | BoardStateComponent |
| The whole "what just happened" payload | `UConnectIt_BoardStateComponent` | `GetChangeEvent()` | `BlueprintPure` | All | `OnBoardStateChanged` | BoardStateComponent |

## §E — Action / input affordances *(local player only)*

| UI Value | Owning Class | C++ Accessor | BP Accessor | Net Availability | Change Signal | Home |
|---|---|---|---|---|---|---|
| Available actions this turn | `UTurnBasedActionsComponent` | `GetAllRuntimeActions()` | `BlueprintPure` | Local only | `OnActionPushedSafe`/`OnActionPoppedSafe` | ActionsComponent |
| Per-action required / cancellable / requires-selection | `UTurnBasedAction` | `.bIsRequired/.bIsCancellable/.bRequiresSelection` | `BlueprintReadOnly` | Config | n/a | ActionsComponent → action instance |
| Top-of-stack action tag | `UTurnBasedActionsComponent` | `GetTopAction()->ActionTag` | `BlueprintPure` (**caveat**: live, mutable object — `Complete()`/`Cancel()` are `BlueprintCallable` on it, see `OnActionPushedSafe`/`FTurnActionSnapshot` for the read-only sibling) | Local only | `OnActionPushedSafe` | ActionsComponent |
| Stack depth | `UTurnBasedActionsComponent` | `GetStackDepth()` | `BlueprintPure` | Local only | `OnActionPushedSafe`/`OnActionPoppedSafe` | ActionsComponent |
| Is root on top | `UTurnBasedActionsComponent` | `IsRootOnTop()` | `BlueprintPure` | Local only | same | ActionsComponent |
| Completions this turn / max per turn | `UTurnBasedAction` | `.CompletionsThisTurn` / `.MaxCompletionsPerTurn` | `BlueprintReadOnly` | Local only | `OnActionCompletedSafe` | ActionsComponent → action instance |
| Cooldown turns / turns until available | `UTurnBasedAction` | `.CooldownTurns` / `.TurnsUntilAvailable` | `BlueprintReadOnly` | Local only | ticks per turn start | ActionsComponent → action instance |
| Can end turn | `UTurnBasedActionsComponent` | `CanEndTurn()` | `BlueprintPure` | Local only | poll (no dedicated delegate for the boolean itself; `OnTurnEndReady` fires the moment it becomes true) | ActionsComponent |
| Awaiting server confirmation (input lock / spinner) | `UTurnBasedActionsComponent` | `IsAwaitingRequestConfirmation()` | `BlueprintPure` | Local only | resolved by `NotifyBoardChangeOutcome` | ActionsComponent |
| Action history | `UTurnBasedActionsComponent` | `.ActionHistory` | `BlueprintReadOnly` | Local only | appended per action | ActionsComponent |
| Action display name / icon / description | `UTurnBasedAction` | `.DisplayName`/`.Description`/`.Icon` | `BlueprintReadOnly` — `GAP 7 [fixed]` | Config | n/a | ActionsComponent → action instance |

## §F — Connection / lobby / readiness

| UI Value | Owning Class | C++ Accessor | BP Accessor | Net Availability | Change Signal | Home |
|---|---|---|---|---|---|---|
| Match waiting for participants / ready | `ATurnBasedGameState` | `GetMatchPhase()` (`EMatchPhase::WaitingForParticipants`/`WaitingForReady`) | `BlueprintPure` | All | `OnMatchPhaseChanged` | GameState |
| Per-participant connected | `FTurnParticipantInfo` | `.bConnected` | `BlueprintReadOnly` | All | `Participants` changes | ParticipantManager |
| Per-participant ready | `ATurnBasedPlayerState` | `IsReady()` | `BlueprintPure` | All | `OnReadyChanged` | PlayerState |
| Local player's own ready state | `ATurnBasedPlayerState` (local) | `IsReady()` | `BlueprintPure` | Local only | `OnReadyChanged` | PlayerState |
| Reconnect timeout (config) | `UTurnBasedParticipantManagerComponent` | `.ReconnectTimeout` | `BlueprintReadOnly` | Config | n/a | ParticipantManager |
| All participants ready | `UTurnBasedParticipantManagerComponent` | — (no query, only the delegate) | — | All | **`poll only`** — `OnAllParticipantsReady` is fire-and-forget with no queryable counterpart; a widget added to the viewport after it already fired cannot recover the fact | ParticipantManager |

## §G — Sequencing / "game is busy" *(animation gating)*

| UI Value | Owning Class | C++ Accessor | BP Accessor | Net Availability | Change Signal | Home |
|---|---|---|---|---|---|---|
| Tags currently in flight | `UGameEventTaskSubsystem` | `GetTagsInQueue()` | `BlueprintCallable` | All | `OnActiveManagerTagsChanged` | GameEventTaskSubsystem |
| Idle vs. busy | `UGameEventTaskSubsystem` | `GetTagsInQueue().IsEmpty()` | derived from above | All | same (check emptiness on **each** fire — a multi-tag container can fire this delegate more than once before actually idling, per `RuntimeStateAccess.md`) | GameEventTaskSubsystem |
| Not-yet-started queue backlog | `UGameEventTaskSubsystem` | — | — | — | — | none — open gap, carried over unchanged from `RuntimeStateAccess.md` §Gaps #1, out of scope here |

## §H — Faction visual identity (deferred)

**Status: deliberately not built.** A working implementation (a
`UGameInstanceSubsystem` + `UDataAsset` palette + `UDeveloperSettings`
config) was built and then removed at the project owner's request — the
project is in a white-box phase and this isn't needed yet; keeping the
codebase lean took priority over having the accessor ready early. The rows
below record the *design intent* only (what a later pass should build, and
where), not a current API.

Whenever this is picked back up: deliberately **not** replicated and **not**
on `PlayerState` — a faction's colour/icon is a local presentation choice,
not game state. Recommended shape: a `UGameInstanceSubsystem` (not
`UWorldSubsystem` — the palette is immutable config, not per-world data;
not `ULocalPlayerSubsystem` — that would be unreachable from a board/piece
actor, which has no local player), reading a `UDataAsset` palette keyed by
faction slot index, configured via a `UDeveloperSettings` Project Settings
page, and skipping dedicated servers entirely (`ShouldCreateSubsystem`).

| UI Value | Owning Class | C++ Accessor | BP Accessor | Net Availability | Change Signal | Home |
|---|---|---|---|---|---|---|
| Primary / secondary colour | — | — | — | Config (Local only if overridden) | — | `GAP 1` → FactionVisualsSubsystem |
| Icon brush | — | — | — | Config | — | `GAP 1` → FactionVisualsSubsystem |
| Display label / short label | — | — | — | Config | — | `GAP 1` → FactionVisualsSubsystem |
| Piece / tile-highlight material | — | — | — | Config | — | `GAP 1` → FactionVisualsSubsystem — board/piece layer, not wired to `AConnectIt_GridPiece` even when built |

---

## Gaps closed by this catalogue

Numbered so C++ change lists and PRs can reference them by number. `[fixed]`
means the accessor now exists as of this catalogue's own change set;
otherwise still open at time of writing.

1. **Open, deferred on purpose — faction visual identity.** No colour/icon/
   label exists anywhere on the networked path. A full implementation
   (`UConnectIt_FactionVisualsSubsystem` + `UConnectIt_FactionVisualsDataAsset`
   `UGameInstanceSubsystem`/`UDataAsset` pair, presentation-only, skipping
   dedicated servers, configured through a `UDeveloperSettings` page) was
   built during this catalogue's own change set and then **deliberately
   removed** — the project is in a white-box phase and the project owner
   asked to keep the codebase lean rather than carry this ahead of need.
   See [§H](#h--faction-visual-identity-deferred) for the design intent to
   pick back up when the time comes. Do not
   migrate the legacy `PDA_PlayerColours`/`ConnectIt_PlayerColours` assets
   when it is — they belong to the dead branch (see `Duplication.md` #1);
   copy colour values by hand instead if matching the old look is wanted.
2. **`[fixed]` Participant display name has no 1-node BP path** — `FTurnParticipantInfo::GetDisplayName()`
   is a plain C++ struct method (UHT does not allow `UFUNCTION` on `USTRUCT`
   members). Closed by a static `UTurnBasedMechanicsLibrary::GetParticipantDisplayName`,
   which `GetDisplayName()` itself now delegates to (one implementation).
3. **`[fixed]` Win score target was unreachable, and existed in three
   uncoordinated places** (`UConnectIt_ScoreThresholdWinCondition::WinScoreThreshold`,
   `UConnectIt_GameRulesLibrary::ConnectIt_Score_Max` — itself a
   `static constexpr`, so not even Blueprint-visible, feeding
   `IsGameOver(ScoreBoard)`). Closed for the UI-facing half by replicating
   `FConnectItBoardState::TargetScore`, authored by whichever
   `IConnectIt_WinCondition` is active via a new `GetTargetScore()` on the
   interface, surfaced through `UConnectIt_BoardRules::GetTargetScore()`
   (renamed/converted from `UConnectIt_BoardRulesComponent` since this was
   written — see [ConnectItModule.md](ConnectItModule.md#board-architecture-overhaul))
   and `AConnectIt_GameState::GetTargetScore()`/`GetFactionScoreProgress(int32)`.
   **Deliberately did not** collapse `ConnectIt_Score_Max` into this — on
   inspection it isn't a duplicate UI accessor at all, it's an internal
   normalisation bound used throughout the MinMax AI heuristic
   (`ConnectIt_MinMaxManager`/`ConnectIt_MinMaxTreeBuilder`), including
   against detached hypothetical board states that were never on a live
   `UConnectIt_BoardRules`. Coupling it to a runtime, potentially
   BP-mutated `TargetScore` would be an AI-behaviour change, not a UI-bridge
   one — left in place with a comment explaining the two are allowed to
   diverge, rather than silently refactored under this plan.
4. **No per-tile-by-position BP accessor** — `FConnectItBoardState`'s inline
   helpers (`GetTileData`/`IsTileOccupied`/`IsTileActive`/`GetScore`/etc.)
   cannot carry `UFUNCTION`. Closed by `UConnectIt_BoardStateLibrary`, a
   `UBlueprintFunctionLibrary` taking `const FConnectItBoardState&` — works
   on the live state, the previous state, or a detached/hypothetical one
   (MinMax), which ruled out wrapping on `UConnectIt_BoardStateComponent`
   instead (that class's own commented-out `TODO` block debated exactly
   this; resolved here).
5. **`[fixed]` Turn countdown unreadable by non-active clients** — `ReplicatedTurnDuration`
   is duration only; `FTurnNotification.TurnDuration` reaches only the
   active participant via `ClientReceiveTurnNotification`; the actual
   `TurnTimerHandle` is server-private. Closed by a replicated
   `ReplicatedTurnStartServerTime` (a timestamp, not a per-tick value) plus
   `ATurnBasedGameState::GetTurnTimeRemaining`/`GetTurnTimeRemainingFraction`
   derived from `GetServerWorldTimeSeconds()`.
6. **`[fixed]` No single-call "active participant" lookup** — BP had to pull
   `GetParticipants()` + `GetActiveParticipantIndex()` and index in itself
   (`RuntimeStateAccess.md` gap #2). Closed by `GetActiveParticipant(bool&)`
   / `GetParticipantBySlot(int32, bool&)` on `UTurnBasedParticipantManagerComponent`
   and `ATurnBasedGameState`.
7. **`[fixed]` Actions have no display metadata** — `UTurnBasedAction`'s
   config block (`bIsRequired`, `MaxCompletionsPerTurn`, etc.) has no
   name/icon/description; an action bar has nothing to label a button with.
   Closed by adding `DisplayName`/`Description`/`Icon` to `UTurnBasedAction`
   (required promoting `SlateCore` from private to public in
   `UnrealTurnBasedMechanics.Build.cs`, since the property lives on a
   public header).

## Fixed while writing this catalogue

Two correctness bugs found during the exploration for this catalogue,
fixed ahead of the gaps above since production UI would otherwise build on
top of them:

- **`AConnectIt_GameState::IsLocalPlayerTurn()` returned `false` on every
  remote client, always** — it read `UTurnBasedParticipantManagerComponent::GetControllerAtIndex()`,
  which is backed by `ServerControllers`, documented on that class as
  server-only and never replicated. Reimplemented as a passthrough to
  `UConnectIt_GameUtilityLibrary::IsLocalPlayerTurn`, the already-correct
  slot-index-based implementation — same name/signature, no BP breakage,
  and now there is exactly one implementation instead of two disagreeing
  ones.
- **`AConnectIt_GameState::GetFactionScore`/`GetAllScores` `check()`-crashed
  on a null board-state component** instead of degrading gracefully like
  their sibling accessors (`IsTileOccupied`, `IsTileValidForPlacement`,
  `GetBoardSnapshot`) — both are `BlueprintPure`, so a scoreboard widget
  constructed before the board manager registers on that client would hard
  crash. Now guarded with `if (BSC)` like the rest.

## Not covered by this catalogue

- The not-yet-started `GameEventTaskSubsystem::ContainerQueue` backlog (§G) —
  carried over unchanged from `RuntimeStateAccess.md`.
- `AConnectIt_GameMode::HandleGameOver`'s hardcoded `EMatchEndReason::ScoreThresholdReached`
  (§C) — a GameMode logic gap, not a missing accessor.
- Faction visual identity itself (§H) — deferred on purpose, see Gap 1.
  Wiring a future palette's piece/tile-highlight materials onto
  `AConnectIt_GridPiece` is a further follow-up beyond even that — the
  actual piece-spawn pipeline is itself an open item (see `Duplication.md`).
