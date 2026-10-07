# Rules as thread-safe structs, and a shared board simulation

Design + plan note (2026-10-06). Approved in plan mode. Phase A is implemented from this note; Phase B is design only.

## Context
Owner review (10-06): `IConnectIt_WinCondition` has become a wrapper around `MakeSearchWinCheck`; `IConnectIt_ScoringRule`
is line/score-centric and, being a UObject interface, can't be called by the async MinMax; `FConnectItMinMaxRules::ApplyMove`
only knows PlacePiece. Root cause: the game's rules exist twice -- UObject interfaces (BlueprintNativeEvents, game thread)
for the real game, and thread-safe copies/snapshots for the AI. Fix: **one set of rules as plain, thread-safe data** that
both the Mediator and the search call. Decisions: rules become polymorphic **C++ structs** (no Blueprint-authored rule
logic; designers still pick/configure them in the Details panel); win rules expose **generic per-faction progress** plus an
**optional target score**; build **Phase A now**, write **Phase B** (move types + shared pipeline, amending the 09-20
`board-request-objects` design to be thread-safe) as a design note.

## Phase A -- implement

### Rule structs (`Source/ConnectIt/Public/Board/Rules/`, replacing the interfaces + UObject classes in place)
All are USTRUCTs with virtual functions, held in `TInstancedStruct`; pure functions of their inputs (no UObject refs).
- `FConnectItScoringRule` -- `virtual float ApplyScoring(Board&, Position, Faction, OutScoringPositions) const`.
  - `FConnectItScoringRule_Lines` (`ConnectLength`) -- today's line algorithm (the existing static functions move onto
    it; `GetScoringDirections` stays public). Nothing line-specific on the base (no `GetMinimumConnectLength`).
- `FConnectItWinCondition` -- `GetWinningFaction(const Board&) const` (INDEX_NONE = nobody);
  `GetProgress(const Board&, Faction) const` (0-1, any rule); optional `GetTargetScore(float& Out) const` /
  `SetTargetScore(float)` (false unless score-based).
  - `FConnectItWinCondition_ScoreThreshold` (`WinScoreThreshold`) -- progress = score / threshold.
- `FConnectItTilePlaceableRule` -- `IsTilePlaceable(const Board&, int32 TileIndex) const` (by index: no position lookups
  in the search's hot path; callers with a position resolve the index once).
  - `FConnectItTilePlaceableRule_Unoccupied`.
- `FConnectItRuleSet` (new `ConnectIt_RuleSet.h/.cpp`) -- the three `TInstancedStruct` members (`ExcludeBaseStruct`),
  constructor seeds today's defaults (Lines 4 / ScoreThreshold 100 / Unoccupied). Helpers: `ApplyScoring`,
  `IsTilePlaceable` (index + position overloads), `GetWinningFaction`, `GetTargetScore`/`SetTargetScore`,
  `GetScoringRuleAs<T>()`, and `StampWinState(Board&)` (sets `bGameOver` / `WinningFactionSlot` / `TargetScore` /
  `WinProgress`) -- the one place the result is written into board state.
- Removed: `IConnectIt_ScoringRule`, `IConnectIt_WinCondition`, `IConnectIt_TilePlaceableRule`, their three UObject
  classes, `FConnectItWinCheck` (+ score-threshold check), `UConnectIt_BoardRules`.

### Consumers
- `UConnectIt_LevelConfigDataAsset`: `FConnectItRuleSet Rules` replaces the three Instanced UObject properties.
- `AConnectIt_GameMode`: holds its own per-match `FConnectItRuleSet` (a value copy of the level config's -- replaces
  `BoardRules` and the `DuplicateObject` calls); applies the menu's target score via `Rules.SetTargetScore`; exposes
  `GetRules()`. `UConnectIt_BoardRequestMediator::Initialise` takes the rule set; its handlers' rule calls switch over
  (handlers otherwise unchanged); `CheckWinCondition` calls become `Rules.StampWinState(NewState)`.
- `FConnectItBoardState`: `TArray<float> WinProgress` (replicated like `TargetScore`, for a generic progress bar).
- AI: `FConnectItAIDecisionContext` carries `FConnectItRuleSet Rules` (value copy) instead of `ConnectLength` /
  `WinScoreThreshold` / `WinCheck`. `FConnectItMinMaxRules` copies it and resolves the three rule pointers once;
  `GenerateMoves` uses the real placement rule, `ApplyMove` the real scoring rule, `IsTerminalState` /
  `EvaluateTerminalState` the real win condition. Line geometry is built only if the scoring rule is
  `FConnectItScoringRule_Lines` (otherwise the Line Potential term contributes 0, with one warning). `MakeRoot` clears
  `WinProgress` on the search's copy (no extra allocation per node).

### Tests (`Private/Tests/ConnectIt_MinMaxTests.cpp`, `ConnectIt_MinMaxTestTerms.h`)
- Helper builds a rule set with a given threshold; the test-only corner win check becomes a test-only
  `FConnectItWinCondition` struct (Hidden). Add `RuleSetScoresLikeTheGame`: placing the 4th piece through
  `FConnectItRuleSet::ApplyScoring` and through the search's `ApplyMove` gives the same board.

## Phase B -- design note only (no code)
Amend the 09-20 `design/board-request-objects.md` design: move/request types, primitives, reactions and the pipeline are
**thread-safe plain C++** (not UObjects), so `Mediator::ProcessRequest` and the search's `ApplyMove` run the *same*
pipeline (apply → score touched positions → reactions → win state); the search's `FMove` becomes "request type + compact
payload"; per-move-type generation for the AI is Phase C (the existing wishlist task). Record the hot-path constraint
(no per-node heap churn; change-event output optional in the search).

## Editor fallout (owner)
- `CI_LevelConfig_Test`: rules reset to defaults -- re-enter any non-default values (win threshold, connect length).
- `Content/_ConnectItNetworked/Sandbox/CI_WinConTest` (Blueprint subclass of the removed win condition) will fail to
  load -- delete it. Other Blueprints only mention the rules in a tooltip; no fixes expected.

## Vault
Step 0: plan moved here from the plan-mode staging file (done); decision `2026-10-06-rules-are-thread-safe-structs.md` (supersedes the mechanism in
`2026-09-24-minmax-calls-real-rules-as-static-thread-safe-functions` and `2026-10-04-minmax-win-detection-from-win-condition`);
Phase B appended to `design/board-request-objects.md` with a pointer; update the affected `code/` pages' notes
(`UConnectIt_BoardRules` page marked removed); log + index.

## Verification
- Build (editor closed); `Automation RunTests ConnectIt.AI` -- all pass; throughput within ~10% of 387k nodes/s.
- PIE: place/swap/shift still score and win as before (human vs human, 2 clients); vs AI still wins/blocks; a menu
  target-score override still applies; UI target score still shows.

## Status (2026-10-06) -- Phase A implemented, built, unit-tested; Phase B written up

- Build clean; 9/9 `ConnectIt.AI` tests pass (new: `RuleSetScoresLikeTheGame` -- the Mediator's way and the search's
  `ApplyMove` produce the same board); throughput unchanged (~386k nodes/s).
- Decision: [2026-10-06-rules-are-thread-safe-structs](../_decisions/2026-10-06-rules-are-thread-safe-structs.md).
  Phase B: appended to [board-request-objects](board-request-objects.md) as an amendment.
- Not yet run in PIE. Editor fallout for the owner is listed above (level config rules back at defaults;
  `CI_WinConTest` to delete).
