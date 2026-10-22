---
schema: code
kind: UCLASS
role: primary
source:
  - Source/ConnectIt/Public/Framework/GameMode/ConnectIt_GameMode.h
  - Source/ConnectIt/Private/Framework/GameMode/ConnectIt_GameMode.cpp
reconciled: 2026-10-22
commit: 1e0d5cf
---

# AConnectIt_GameMode

`ATurnBasedGameMode` subclass. The **server-only** hub of a match: it resolves and owns the live match setup (the
match's `FConnectItRuleSet`, the three director classes, loadouts), owns the
[[UConnectIt_BoardRequestMediator|board request mediator]], is the single entry point for board-change requests, sends
every director the same events, carries out the level's changes, and enforces **one board change at a time**.

## When you touch this

- Match setup: which preset plays (menu, else the map's World Settings), the rules, directors, AI spawn (Adventure),
  board initialisation, ready-check.
- A new kind of level change (board operation, rules, goals, layout) or director event.
- The board-change lock (requests refused / level changes queued while a change plays out).
- Game-over / forfeit handling.

## Entry points

- **Config (`EditDefaultsOnly`):** `MatchType` (`Adventure` | `Online`), `AIControllerClass`, `AIDisplayName`,
  `NumFactions`, `BoardChangeTimeoutSeconds` (default 15).
- **Requests:** `ProcessBoardRequest(Request, RequestingFaction) -> bool` -- refused unless the board is settled, then
  `BoardRequestMediator->ProcessRequest`. The player controller's server RPC and the AI controller both call it.
- **One board change at a time:** `IsBoardChangeResolving()` (a change being carried out or its visuals still
  playing), `IsBoardSettled()` (that, and no level change waiting), `GetNumBoardChanges()`, `OnBoardChangeSettled`
  (native). Mirrored to the GameState for clients.
- **Live setup:** `GetRules()`, `ModifyRules(Change)` (the one way to change the rules after setup -- republishes them),
  `GetAIDirectorClass()`, `GetLevelDirector()` / `GetPlayerDirector(Faction)` / `GetAIDirector()`.
- **Level changes** (what the Level director calls), each at an `EConnectItChangeTiming`: `ApplyLevelOperation`,
  `SetScoringRule`, `SetConditions(Side, bWinGoals, Conditions)`, `SetLayout(Layout)`. Loadouts:
  `SetLoadout(Role, Loadout, Timing)` / `SetLoadoutForPlayer`.
- **Overrides:** `PostLogin` (-> `SeedActionStateForPlayer`), `HandleMatchHasStarted` (mediator, AI spawn, board,
  directors, bindings), `HandleMatchHasEnded`.
- **Bound handlers:** `HandleGameOver` (`ConnectIt.Event.PlayerWin` completing), `HandleBoardChangeComplete`
  (`ConnectIt.Event.BoardChangeComplete` completing -- lifts the lock), `HandleTurnPhaseChanged` (turn start / end ->
  turn-start level changes, director events), `HandleBoardStateChangedForDirectors` (checkpoint / lose / phase / win
  events -> directors; Level triggers), `HandleInvalidNumberOfPlayers`, `HandleActiveControllerChanged` (parked
  loadouts).

## Collaborators

- **Setup** (`EnsureMatchSetupResolved`, once): the preset from the main menu (`UConnectIt_MatchSetupSubsystem`), else
  the map's `AConnectIt_WorldSettings::FindDefaultMatchPreset`; copies its rules and director classes; resolves both
  sides' goals from the **Level director's class defaults** (`ResolveConditionGroups`); applies the menu's tweaks;
  publishes to the GameState (`PublishMatchSetup`).
- **Directors:** makes the Level director and one Player director per human (`FindOrCreatePlayerDirector`); the AI
  director belongs to the AI controller. `ForEachDirector` sends events in dispatch order (Level, Players by slot, AI).
- **Board:** `InitialiseBoard` -> [[UConnectIt_BoardStateComponent]]`::InitialiseBoardState` with registries from
  [[UConnectIt_BoardRegistrySubsystem]]. Binds `OnBoardStateChanged` (directors) and `OnBoardChangeCommitted` (lock).
- **Mediator:** made in `HandleMatchHasStarted`, given `&Rules`. Requests go through `ProcessRequest`; level changes
  through `ProcessLevelOperation`.
- **Level-change queue** (`FConnectItLevelChangeQueue`): changes asked for while the board is busy wait in order;
  `DrainLevelChanges` applies them one board change at a time as each change plays out; turn-start changes join the same
  line. Goals changes (`ApplyGoalsChange`: rules, then a Change Goals operation) and layout changes
  (`ApplyLayoutChange`: the layout's Change Layout, made against the board at apply time) are board changes too.
- **Seeding:** `PostLogin` seeds each human's PlayerState action state from their Player director's `StartingLoadout`.
- Flows: [[place-piece-request]], [[ai-turn]], [[board-state-single-source-of-truth]].

## Gotchas

- **Structurally server-only.** `GetAuthGameMode()` is null on clients; clients read the GameState mirrors (rules,
  board-busy, match result).
- **One board change at a time.** A request before the board settles is refused (the requester's action re-enables).
  The lock lifts when the change's completion marker completes -- the server's visuals; with no visual tasks that is
  inside the commit, so the release only schedules the next drain (next tick). A visual task that never completes
  holds the event queue; the timeout lifts the lock and logs what's stuck, but can't unstick the queue.
- **A preset needs a Level Director Class**: both sides' goals come from it (none = nobody can win; warned at setup).
- **Humans' action state is seeded here** (`PostLogin`); nothing else seeds a remote human, and the mediator gate
  rejects every request from an unseeded player. The AI controller seeds itself.
- Don't `NewObject` the mediator in the constructor (Blueprint-child property overrides aren't applied yet).
- `HandleGameOver` ignores its tag param for data -- the winner is read from the board.
- A level whose triggers keep feeding each other is stopped after 64 changes in a row (error logged).

## Cross-impact

- A new kind of level change: `FConnectItLevelChange` (a field), `ApplyLevelChange`, the Level director's call, and
  usually a level-only board operation + event ([[add-a-board-request-type]]).
- A new director event: `UConnectIt_Director` (the event), the hub's dispatch here, and the recipes that list events
  ([[script-a-level]]).
- The lock's semantics: `AConnectIt_GameState` (mirror), `AConnectIt_PlayerController` (client check),
  `AConnectIt_AIController` (waits for a settled board), [[place-piece-request]].

## Changes

- 2026-10-22 — page rewritten against a month of changes: owns the rule set (`UConnectIt_BoardRules` retired) and the
  live match setup (presets, World Settings, no project settings); the three directors and their event hub; level
  changes with timing and the level-change queue; one board change at a time (lock, timeout, settled delegate, GameState
  mirror); goals on the Level director (`SetConditions`); phased levels (`SetLayout`, phase events). See the
  2026-10-20..22 decisions.
- 2026-09-21 — `SeedActionStateForPlayer` added (server seeding of human PlayerState action state from
  `PostLogin`), part of the loadout / turn-end redesign; `HandleMatchHasStarted` no longer needs to grant
  swap budgets (legacy `SwapUsesRemaining` removed).
- 2026-09-14 — `PlayerStateClass` fixed to `AConnectIt_PlayerState` (was silently dead
  code — every `PlayerState` was a plugin `ATurnBasedPlayerState`, see
  [[ConnectIt/_discussions/2026-09-14-swap-implementation-qa|_discussions/2026-09-14-swap-implementation-qa]]);
  `HandleMatchHasStarted` now grants each player their `SwapUsesRemaining` budget.
- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored (`eddd631`).
- 2026-09-09 — `InitialiseBoard` stopped doing its own `FConstPlayerControllerIterator`
  scan for the registries; now pulls them from `UConnectIt_GameUtilityLibrary` →
  [[UConnectIt_BoardRegistrySubsystem|UConnectIt_BoardRegistrySubsystem]], and passes a
  real `PieceRegistry` into `InitialiseBoardState` instead of `nullptr`. Removed a
  silent-failure path (the scan could run before any controller connected). See
  [[ConnectIt/_decisions/2026-09-09-board-registries-to-world-subsystem|decisions/2026-09-09-board-registries-to-world-subsystem]].
- 2026-09-08 — board authority moved here: `AConnectIt_BoardManager` (world actor)
  retired; `BoardRequestMediator` + `BoardRules` constructed in `HandleMatchHasStarted`
  as server-only UObject members. See
  [[ConnectIt/_decisions/2026-09-08-board-architecture-overhaul|decisions/2026-09-08-board-architecture-overhaul]].

## See also

- Decisions: [one board change at a time](../_decisions/2026-10-21-one-board-change-at-a-time.md),
  [director hub and level changes](../_decisions/2026-10-21-director-hub-and-level-changes.md),
  [conditions on the Level director](../_decisions/2026-10-22-conditions-on-the-level-director.md),
  [phases as events, layouts as data](../_decisions/2026-10-22-phases-as-events-layouts-as-data.md),
  [no ConnectIt project settings](../_decisions/2026-10-21-no-connectit-project-settings.md),
  [[ConnectIt/_decisions/2026-09-08-board-architecture-overhaul|board-architecture-overhaul]]
- Recipes: [[script-a-level]], [[script-a-phased-level]]
- [[UConnectIt_BoardRegistrySubsystem|UConnectIt_BoardRegistrySubsystem]]
