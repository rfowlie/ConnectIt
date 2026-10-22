---
type: plan
status: complete
approved: true
approved-on: 2026-10-16
completed-on: 2026-10-19
playtested: false
design:
question:
decision: "[2026-10-16-ai-director-pipeline](../_decisions/2026-10-16-ai-director-pipeline.md)"
---

# Plan: AI director pipeline + win conditions as data assets

> **Status: built** 2026-10-16..19 in stages 0-7, each built and tested (40/40 at the end); not yet played in PIE. Log:
> [_logs/2026-10-16-1700](../_logs/2026-10-16-1700.md); decision:
> [_decisions/2026-10-16-ai-director-pipeline](../_decisions/2026-10-16-ai-director-pipeline.md). Deviations from the plan below
> are listed in the log.

## Context

The composable AI from 2026-10-14 works and tests pass (30/30), but the follow-up design talks settled a cleaner split of jobs:

- The **director** owns the AI's setup over a match: loadout, strategies and selector.
- The **coordinator** stops being a strategy. It becomes one fixed pipeline: run the strategies, pull their answers at the deadline, hand them to a selector.
- **Strategies** only decide, and keep a best-so-far answer cached.
- **Selectors** pick among the answers. Mistake chance moves here.
- The **AI profile** goes away. A "mode" is a match preset that names its director.
- **Win conditions** become data assets wrapping the existing thread-safe structs.

The owner chose "Ready: everything" (one plan), and these selectors for the first version: First Available (default), Top-N + mistake chance, Greedy scorer, Weighted by confidence.

Assumption stated in the review and not contested: **the controller still submits.** It tries candidates in order, falls back to the next, and if all are rejected it logs an error and ends the turn.

Each stage below builds and passes the tests before the next starts. The editor must be closed for builds. Nothing is committed by me.

## Stage 0: move this plan

- Copy this file to `ConnectIt-Vault/ConnectIt/design/ai-director-pipeline.md`, then delete the staging file.
- `design/composable-ai.md` gets a line at the top saying which parts this supersedes, with a link.

## Stage 1: MinMax publishes after each depth

- **Plugin** (`Plugins/UnrealGameIntelligence/.../Search/MinMax/GI_MinMaxAlphaBeta.h`):
  - `TAlphaBeta::Run` and `LaunchAlphaBetaAsync` take an optional `TFunction<void(const TArray<TScoredMove<FMove>>&, int32 Depth)> OnDepthCompleted`.
  - It is called right after `Result.RootScores = DepthScores` on the search thread.
  - This stays game-agnostic; the default is empty, so existing callers are unchanged.
- **Game** (`Public/AI/ConnectIt_MinMaxSearch.h/.cpp`):
  - `Launch` gets a thread-safe progress holder: `TSharedPtr<FConnectItMinMaxSearchProgress, ESPMode::ThreadSafe>`, holding a ranking and a depth behind an `FCriticalSection`.
  - The callback writes converted `FConnectItMinMaxRankedMove`s into it; the game thread reads a copy whenever it likes. No UObjects cross threads.
  - `ToResult` and the progress holder share one conversion.
- **Tests:**
  - plugin-level: the callback fires once per completed depth, with the same ranking `Run` returns at the end;
  - game-level: the progress holder holds the depth-N ranking.

## Stage 2: candidate fields and strategy caching

- **`FConnectItAICandidate`** gains optional fields:
  - `Confidence`: a float from 0 to 1; −1 means not given.
  - `StrategyScore`: raw, only comparable within one strategy.
  - `Tags`: an `FGameplayTagContainer`.
  - `SourceLabel`: an `FName`.
  - `UConnectIt_AIStrategyLibrary` (or the existing helper spot) gets `HasConfidence`, for Blueprint.
- **`UConnectIt_AIStrategy`** (`Public/AI/ConnectIt_AIStrategy.h`) adds caching:
  - `PublishBestSoFar(Decision)`: protected, BlueprintCallable. Replaces the cached decision.
  - `FinishDecision` also caches, and marks the strategy finished.
  - `virtual FConnectItAIDecision GetBestSoFar()` (C++, game thread): the cached decision by default.
  - `IsDecisionFinished()`.
  - Keep `BeginDecision` / `CancelDecision` / `IsDecisionActive` / `GetSupportedRequestTypes`, plus `MaxCandidates` and trimming.
  - Remove `MinThinkSeconds` from the strategy; it moves to the director (Stage 4).
  - `SourceLabel` defaults to the strategy's display name, and the coordinator stamps it when it is empty.
- **`UConnectIt_AIStrategy_MinMax`:**
  - It loses `TopMovesConsidered` and `MistakeChance` (they move to the Top-N selector) and keeps `Search`.
  - It overrides `GetBestSoFar` to read the progress holder.
  - Its candidates are in rank order, with `StrategyScore` set to the raw score.
  - `Confidence`: 1 for a forced win; otherwise (score − worst) / (best − worst) across the ranking; 1 when only one move exists.
  - `Tags` stay empty: nothing to tag yet.
- **`MakeDecisionFromSearch`** fills the same fields, so the Blueprint node and the strategy agree.
- **Tests:**
  - publish, then cancel: `GetBestSoFar` returns the published decision;
  - a MinMax with a short budget exposes a partial ranking before it finishes;
  - confidence normalisation, including the single-move and all-equal cases.

## Stage 3: selectors

- **New `UConnectIt_AISelector`** (`Public/AI/ConnectIt_AISelector.h/.cpp`): abstract, `EditInlineNew`, `DefaultToInstanced`, `Blueprintable`.
  - `Select(Context, Decisions)` (C++, non-virtual) runs three steps:
    1. `Endorse` merges duplicate payloads across strategies, in priority order. The result is `FConnectItAIEndorsedCandidate {Candidate, Endorsements, FirstStrategyIndex}`.
    2. The BNE `RankCandidates(Context, EndorsedCandidates)` returns the ordered candidates.
    3. The result is trimmed to `MaxCandidates`.
  - Input per strategy: `FConnectItAIStrategyAnswer {StrategyIndex, SourceLabel, bFinished, Decision}`. These are decisions only, never strategy objects.
  - A `FRandomStream` (seeded at the start of each selection; a `RandomSeed` set from C++ fixes it, for tests).
- **The four selectors** (each is a small subclass, in one file pair or one per class, matching the repo):
  - **First Available** (default): endorsed order unchanged.
  - **Top-N**:
    - Settings: `TopMovesConsidered` and `MistakeChance`.
    - With probability `MistakeChance`, a uniform pick from ranks 2..N goes first; the rest keep their order.
    - This is the same behaviour that leaves MinMax.
  - **Greedy scorer:**
    - Runs `SimulateOperation` on each candidate (with `Context.Rules` and `Context.OwnSlot`) and sorts by points scored.
    - Ties keep priority order; an operation that doesn't apply sorts last.
  - **Weighted by confidence:**
    - Settings: `Temperature` and `MissingConfidence` (the value used when a candidate gives none).
    - Samples one candidate with weight exp(confidence / temperature); it goes first, the rest keep their order.
    - A temperature near 0 approaches always-best.
- **Tests:** endorsement counting and order, plus each selector's ordering with fixed seeds. Greedy uses a board where one candidate scores.

## Stage 4: coordinator as a pipeline, and the director owns the setup

- **`UConnectIt_AICoordinator`** (replaces `UConnectIt_AIStrategy_Coordinator`): a plain UObject owned by the controller, not a strategy, not designer-configured.
  - **`Begin(Context, Strategies, Selector, TimeBudgetSeconds, StrategyBudgetShare)`:**
    - starts every strategy at once;
    - each strategy's context gets `TimeBudgetSeconds × share`;
    - array order is priority.
  - **Polling, not delegates:** a looping game-thread timer, every 0.05 s, checks `IsDecisionFinished` on every strategy. It concludes early when all are finished.
  - **At the deadline:**
    - pull `GetBestSoFar` from every strategy;
    - if none has a move, keep polling until one does, logging one warning that names the slow strategies;
    - cancel the rest, then call `Selector->Select`.
  - **Hands the result back** through a direct call to the controller: `OnCoordinatorConcluded(Decision)`.
  - Decision ids guard against stale answers, as now.
- **`UConnectIt_AIDirector`** gains the setup:
  - Instanced properties: `StartingLoadout`, `Strategies` (instanced array), `Selector` (instanced; default First Available), `ThinkTimeBudgetSeconds` (2), `StrategyBudgetShare` (0.8), `MinThinkSeconds`.
  - Actions:
    - existing: `SetAILoadout`, `GetAILoadout`;
    - new: `SetStrategies(Templates)`, `SetSelector(Template)`, `GetStrategies` / `GetSelector` (the live copies, for tweaking parameters).
    - Strategy and selector changes apply from the next decision, as `ChangeStrategy` does now.
  - Events:
    - existing: `OnAITurnStarted`, `OnAITurnEnded`, `OnWinCheckpointReached`;
    - new `OnMatchStarted`: set up at level start;
    - new `OnOpponentTurnStarted(TurnNumber)`: needed for the tell flow.
  - Checkpoint events carry the win-condition asset (Stage 6).
  - `GetWorld` is kept, for anything else.
  - Not abstract any more. The base class with its defaults is the plain "classic" director: one MinMax strategy plus First Available.
- **`AConnectIt_AIController`** (`Private/Framework/Controller/ConnectIt_AIController.cpp`):
  - Duplicates the director from the preset; with none set, it duplicates a default base director and logs a warning.
  - Creates the coordinator, and seeds its loadout from `Director->StartingLoadout`.
  - `BeginDecision` runs the coordinator with the director's current strategies and selector.
  - `MinThinkSeconds` is read from the director.
  - The `PendingStrategy` / `AdoptStrategy` code becomes pending strategies plus a pending selector. `PendingLoadout` deferral is unchanged.
  - Submitting with fallback is unchanged.
- **Delete:** `ConnectIt_AIStrategy_Coordinator.h/.cpp`, `FConnectItAIChildDecision`, and `RankCandidates` on it.
- **Tests:** the existing Coordinator and Director tests are rewritten against the new shapes:
  - a slow strategy is cut off, but its best-so-far is used;
  - nothing has answered: wait plus a warning;
  - priority order through First Available;
  - director strategy and selector swaps apply next decision;
  - `OnMatchStarted` and `OnOpponentTurnStarted` fire.

## Stage 5: remove the AI profile

- **Delete** `Public/AI/ConnectIt_AIProfile.h` and `Private/AI/ConnectIt_AIProfile.cpp`.
- **Remove every user:**
  - `UConnectIt_MatchPreset::AIProfile`. Its `IsDataValid` checks move onto the director: the loadout grants request types that some strategy supports, and vice versa (today's profile check, moved).
  - `AConnectIt_GameMode`: the profile resolution in `EnsureMatchSetupResolved`.
  - `AConnectIt_GameState`: replicated `OpponentProfile`, `GetOpponentProfile` and `SetOpponentProfile`.
  - `UConnectIt_GameUtilityLibrary::GetOpponentProfile`.
  - `FConnectItMatchSettings::AIProfile` and its log line in `ConnectIt_MatchSetupSubsystem.cpp`.
  - `UConnectIt_LevelCatalog::AIProfiles`. The menu offers presets instead: a preset names its director, so "Classic Easy / Hard" are two presets.
- **Owner-side asset work** (listed in the log and the decision note):
  - `ConnectIt_UI_MainMenu`: remove the AI-profile choice and `Get Opponent Profile` nodes; pick presets.
  - Delete `CI_AIProfile_PlaceRandomPiece` and `CI_AIProfile_Test`, after moving their loadout and strategy onto a director on the relevant presets.
  - The level catalog asset: drop the AI profiles and add any presets the menu should offer.
  - Both level configs' default presets: set the AI Director.

## Stage 6: win conditions as data assets

- **New `UConnectIt_WinConditionAsset`** (`Public/Board/Rules/ConnectIt_WinConditionAsset.h/.cpp`), a `UPrimaryDataAsset`:
  - `DisplayName`;
  - `Conditions` (`TArray<TInstancedStruct<FConnectItWinCondition>>`, the same structs as now);
  - `IsDataValid` (at least one condition).
  - Variants are just more assets.
- **`FConnectItRuleSet`** (`Public/Board/Rules/ConnectIt_RuleSet.h`):
  - Editor-facing `PlayerWinConditions` / `AIWinConditions` become arrays of asset references, in order.
  - Today's `PlayerWinGroups` / `AIWinGroups` (`TArray<FConnectItWinConditionGroup>`) stay as the runtime copies, no longer edited.
  - `ResolveWinGroups()` copies each asset's conditions into a group at match start. The GameMode calls it where it seeds the rules.
  - From then on the search thread only ever sees struct copies, as today.
- **`FConnectItWinConditionGroup`** gains `TObjectPtr<const UConnectIt_WinConditionAsset> Source`. It is never dereferenced off the game thread; it is only carried along. `Name` comes from the asset's `DisplayName`.
- **`FConnectItBoardEvent_WinCheckpointReached`** gains `Asset`. `AdvanceWinGroups` fills it from the group's `Source`. The director's `OnWinCheckpointReached` gets `Asset` and keeps `GroupIndex` / `GroupName`.
- **Validation:** the preset warning "no AI win groups" now checks `AIWinConditions`.
- **Owner-side asset work:** each match preset's inline win groups must be recreated as win-condition assets and referenced. Changing the property type drops the old inline data, so screenshot or note the groups first. The decision note lists the presets.
- **Tests:**
  - resolve copies the conditions in order;
  - the checkpoint event carries the asset;
  - a MinMax search on resolved rules matches the inline-struct result. The existing scoring and win tests are switched to build rules through `ResolveWinGroups` or the runtime arrays directly.

## Stage 7: docs and notes

- **Recipes:**
  - rewrite `set-up-a-custom-ai-opponent.md` (director instead of profile);
  - rewrite `combine-ai-strategies.md` as "choose strategies and a selector" (strategies plus selectors, the four stock selectors, Blueprint selectors via `RankCandidates`, endorsement);
  - update `direct-the-ai-during-a-match.md` (new events and actions, a tell-flow walkthrough);
  - new `add-a-win-condition.md`.
  - Update `code/recipes/__INDEX.md`.
- **Pages:** `code/systems/ai-turn.md`, and any `code/` type pages for AIProfile and Coordinator (removed or replaced).
- **Design notes:**
  - `design/ai-selector-ideas.md`: the selector idea catalogue, grouped (order and chance, immediate outcome, reply look-ahead, context-aware), plus the "measure plus pick policy" generalisation, kept as an idea;
  - `design/match-menu-options.md`: the `FConnectItMatchOptions` struct handed to the director; rule tweaks stay on the rules. Idea only.
  - Designer note "react to state, not visual tags": a section in the director recipe plus a line in `ai-turn.md`.
  - The tell flow: AI moves → visuals → player turn starts → the director sets the loadout in `OnOpponentTurnStarted` → a visual manager reacts via `OnLoadoutChanged`, queued after the AI's move visuals. It goes in the director recipe; if `OnLoadoutChanged` doesn't exist on the PlayerState yet, it's noted as owner-side Blueprint work, not built.
- **Records:** a decision note `_decisions/2026-10-16-ai-director-pipeline.md` (supersedes parts of `2026-10-14-composable-ai.md`), plus its index line. A log `_logs/2026-10-16-HHMM.md`, plus its index line.

## Critical files

- `Public/AI/ConnectIt_AIStrategy.h`, `ConnectIt_AIStrategy_MinMax.h`, `ConnectIt_MinMaxSearch.h`, `ConnectIt_AIDirector.h`, new `ConnectIt_AICoordinator.h`, new `ConnectIt_AISelector.h` (+ cpps).
- `Public/Framework/Controller/ConnectIt_AIController.h/.cpp`.
- `Public/Framework/Data/ConnectIt_MatchPreset.h`, `ConnectIt_LevelCatalog.h`, `GameMode`, `GameState`, `GameUtilityLibrary`, `MatchSetupSubsystem`.
- `Public/Board/Rules/ConnectIt_RuleSet.h/.cpp`, `ConnectIt_WinConditionGroup.h`, new `ConnectIt_WinConditionAsset.h`, the checkpoint board event.
- `Plugins/UnrealGameIntelligence/.../GI_MinMaxAlphaBeta.h`.
- Tests: `Private/Tests/ConnectIt_MinMaxTests.cpp`, `ConnectIt_MinMaxTestTerms.h`, plus the scoring and win tests that build rule sets.

## Verification

- **After each stage:** build with `Build.bat ConnectItEditor Win64 Development` (editor closed), then run `UnrealEditor-Cmd ... -ExecCmds="Automation RunTests ConnectIt; Quit"`. All tests must pass, new ones included.
- **Owner PIE pass at the end** (after the asset work), Output Log filtered to `ConnectIt_AIController`:
  - `AI director '...'` at start;
  - per turn, `decided -- N candidate(s), best: [SourceLabel] ...`;
  - a slow strategy's partial result is used with a warning;
  - the director swapping the selector or strategies mid-match takes effect next decision;
  - a checkpoint fires `OnWinCheckpointReached` with the asset.
- This also covers the still-unplayed composable-AI items.

## Risks / notes

- This reworks code from 2026-10-14 that has not been committed or played. Suggest the owner commits the current state before Stage 1, so there is a clean diff per stage.
- Stage 6 touches the win-groups system the owner has been editing. I'll re-read those files right before Stage 6 and match edits to whatever is there.
- Asset breakage is unavoidable with the profile removal and the property-type change on win groups. Every affected asset is listed in the log for the owner.
