# AI as composable tools: candidate lists, MinMax as a node, coordinator strategies, and a match director

> **Partly superseded (2026-10-16)** by [ai-director-pipeline](ai-director-pipeline.md): the coordinator is no longer a
> strategy, mistake chance moves from MinMax to a selector, the AI profile is removed (the director owns loadout,
> strategies and selector), and win conditions become data assets. Candidate lists, MinMax as a node and the
> controller's submit-with-fallback stand.

## Context
From a back-and-forth with the owner on `ConnectIt/code/systems/ai-turn` and their notes. Goals: MinMax usable as a
tool from any strategy; strategies as small, swappable, side-effect-free "what to do given this context" units; a way
to combine them; and a way for a match to change the AI's loadout and strategy as it goes (per turn, or at either
side's win checkpoints), which also drives gameplay "tells".

Settled in the discussion:
- **Strategies only decide** (no side effects): safe to query speculatively, reusable, testable.
- **A coordinator is a strategy** holding child strategies; the controller keeps one strategy slot.
- **A strategy finishes with a ranked, capped list of candidates**; the controller submits them in order until one is
  accepted (the AI's version of a human retrying after a rejection); all rejected = loud error, end the turn.
- **Coordinator:** collects children's candidates within its time budget, then ranks them with an overridable judge;
  default = child priority order. Alternatives (weighted random, re-score by simulation) documented in a recipe.
- **Loadouts stay on the PlayerState** (server-enforced, replicated). Whether the player sees them is the UI's call,
  no flag.
- **An AI director on the match preset** changes the AI's loadout / strategy on events; it lives with the preset
  because it reacts to that preset's checkpoints. Move tells = the director sets the next turn's loadout at the end
  of the AI's turn, so it is known (and replicated) during the player's turn.

Step 0 after approval: move this file to `ConnectIt-Vault/ConnectIt/design/composable-ai.md`, delete the staging
copy, and write the decision note.

Built in four stages, each built and tested before the next.

## Stage 1 -- Candidate lists and what the AI may do now
- `FConnectItAIDecision` (`Public/AI/ConnectIt_AIStrategy.h`) holds `TArray<FConnectItAICandidate> Candidates`
  (each: operation payload + summary), best first. `UConnectIt_AIStrategy::MaxCandidates` (default 5) caps it;
  `FinishDecision` trims. Helpers for Blueprint: make a decision from one operation / from a list.
- `AConnectIt_AIController::SubmitDecision` (`Private/Framework/Controller/ConnectIt_AIController.cpp`): try each
  candidate through the existing action lookup + `ProcessBoardRequest`; first accepted wins; log every rejection with
  its reason; all rejected = `Error` log naming the strategy, then end the turn instead of stalling.
- Decision context gains `AvailableRequestTypes`: request types the AI's loadout can produce **and** may use right
  now (`ATurnBasedPlayerState::CanUseAction`: per-turn cap, cooldown, numbered uses). Strategies filter by it up
  front; the server gate still has the final word.
- MinMax strategy: returns its top `MaxCandidates` root moves (it already ranks every root move); mistake chance
  reorders the head of the list as today.

## Stage 2 -- MinMax as a Blueprint tool
- `FConnectItMinMaxSearchSettings` (BlueprintType): depth, time budget, evaluation + ordering weights, own and
  opponent request types to search with.
- Async Blueprint node `Find Best Moves (MinMax)` (`UBlueprintAsyncActionBase`): board, rules, side, settings ->
  `On Complete(ranked candidates)` / `On Cancelled`; runs the existing background search; cancellable.
- `UConnectIt_AIStrategy_MinMax` becomes a thin wrapper: its properties are one settings struct (asset values reset
  -- noted in the log) and `BeginDecision` is one search call.
- A strategy may declare the request types it can produce (`GetSupportedRequestTypes`, BlueprintNativeEvent);
  `UConnectIt_AIProfile::IsDataValid` warns when the loadout and the strategy don't overlap, or the loadout grants
  moves the strategy never plays.

## Stage 3 -- Coordinator strategy
- `UConnectIt_AIStrategy_Coordinator` (Blueprintable): instanced `Children` (in priority order), its own
  `TimeBudgetSeconds`, and a per-child budget handed down.
- `BeginDecision` starts every child (C++ runs them; children report back to the coordinator, not the controller),
  collects candidate lists as they arrive; when the budget is up -- or every child has answered -- cancels the rest,
  calls `RankCandidates` (BlueprintNativeEvent; default = child priority order, de-duplicated), finishes with the
  capped list. Nothing arrived by the budget = keep waiting for the first answer.
- Cancelling the coordinator cancels every child.
- Recipe `code/recipes/combine-ai-strategies.md`: build a coordinator; overriding `RankCandidates` for weighted
  random and for re-scoring with `Simulate Operation`.

## Stage 4 -- Match director
- `UConnectIt_AIDirector` (abstract, Blueprintable, EditInlineNew), instanced on `UConnectIt_MatchPresetDataAsset::AIDirector`
  (optional). Server-only. Events (BlueprintNativeEvent): `OnAITurnStarted`, `OnAITurnEnded` (the place to set the
  next turn's loadout, for tells), `OnWinCheckpointReached(Faction, GroupIndex, GroupName)` (either side).
- Its only actions: `SetAILoadout(Loadout)` and `SetAIStrategy(Strategy)` on the controller it belongs to.
  `SetAILoadout` re-seeds the AI PlayerState (`InitialiseActionState` -- numbered uses restart from the new loadout;
  to confirm with the owner when built); `SetAIStrategy` duplicates the template and takes effect from the next
  decision.
- The controller duplicates the preset's director at match start and calls it: turn start / end from its existing
  turn handling; checkpoints from the server-side board change (`FConnectItBoardEvent_WinCheckpointReached` in each
  committed change event).
- Recipe `code/recipes/direct-the-ai-during-a-match.md`: alternating loadout per turn; new behaviour at a checkpoint;
  a move tell (set next turn's loadout in `OnAITurnEnded`, let the UI read it).

## Docs and vault
Update `code/systems/ai-turn.md` and `code/recipes/set-up-a-custom-ai-opponent.md` per stage; decision note; log per
stage. Task rows left to the owner.

## Verification (per stage)
- Build (editor closed); `Automation RunTests ConnectIt` all pass. New tests: candidate cap/trim and ordering
  (stage 1); search node settings round-trip and wrapper parity with the old MinMax results (stage 2); coordinator
  ranking with synchronous stub children, including budget expiry (stage 3); director event dispatch with a stub
  director (stage 4, as far as possible without a world).
- Owner, PIE: vs AI still plays (each stage); a deliberately bad first candidate is rejected and the next one plays
  (stage 1); a Blueprint strategy calling the MinMax node (stage 2); a coordinator of MinMax + a random strategy
  (stage 3); an alternating-loadout director and a checkpoint change (stage 4).

## Status (2026-10-14): built, not yet played

- All four stages built and tested in turn: `ConnectIt` automation tests 27 -> 28 -> 29 -> 30, all passing (new:
  `ConnectIt.AI.Candidates`, `ConnectIt.AI.MinMax.SearchTool`, `ConnectIt.AI.Coordinator`, `ConnectIt.AI.Director`).
- Additions beyond the plan: strategies and directors resolve a world through their owner (`GetWorld`), needed for the
  coordinator's timer and for Blueprint world-context nodes; `IsOperationAvailable` and `MakeSingleMoveDecision` helpers;
  `MakeDecisionFromSearch` for the MinMax node's result.
- `FConnectItBoardOperation::GetAllRequestTypes` (found by reflection) is how "what can this loadout produce" is answered,
  since an action only says yes or no per request type.
- Breaking for assets: the Blueprint strategy `CI_AIStrategy_PlaceRandom` builds a decision with `Payload` / `Summary`, which
  are now on candidates (rewire to Make Single Move Decision); MinMax profiles' depth / time / weights moved into the strategy's
  `Search` struct and reset to defaults.
- Not covered by tests (need a running match): submitting candidates in order, the director's turn and checkpoint wiring,
  deferred loadout changes, the coordinator's real timer.
- Recipes: [combine-ai-strategies](../code/recipes/combine-ai-strategies.md),
  [direct-the-ai-during-a-match](../code/recipes/direct-the-ai-during-a-match.md).
- Decision: [composable AI](../_decisions/2026-10-14-composable-ai.md).
