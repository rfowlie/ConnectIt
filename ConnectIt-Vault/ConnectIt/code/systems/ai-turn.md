---
schema: systems
spans:
  - Source/ConnectIt/Public/Framework/Controller/ConnectIt_AIController.h
  - Source/ConnectIt/Private/Framework/Controller/ConnectIt_AIController.cpp
  - Source/ConnectIt/Public/AI/ConnectIt_AIDirector.h
  - Source/ConnectIt/Public/AI/ConnectIt_AICoordinator.h
  - Source/ConnectIt/Public/AI/ConnectIt_AIStrategy.h
  - Source/ConnectIt/Public/AI/ConnectIt_AISelector.h
  - Source/ConnectIt/Public/AI/ConnectIt_AIStrategy_MinMax.h
  - Source/ConnectIt/Public/AI/ConnectIt_MinMaxSearch.h
  - Source/ConnectIt/Public/Framework/Data/ConnectIt_MatchPreset.h
  - Source/ConnectIt/Public/Framework/GameMode/ConnectIt_GameMode.h
  - Source/ConnectIt/Public/Board/ConnectIt_BoardRequestMediator.h
  - Plugins/UnrealTurnBasedMechanics/Source/UnrealTurnBasedMechanics/Public/Framework/Controller/TurnBasedAIController.h
  - Plugins/UnrealGameIntelligence/Source/UnrealGameIntelligence/Public/Search/MinMax/GI_MinMaxAlphaBeta.h
reconciled: 2026-10-20
commit: f03d10d
---

# AI turn: who the opponent is, and how it takes a move

The step-by-step for designers is the recipe [[set-up-a-custom-ai-opponent]]; this page is the flow behind it.

## What happens

Five parts, each answering one question. **The GameMode** spawns an `AConnectIt_AIController` (its `AIControllerClass`) when an
Adventure match starts and registers it as the second participant. **The match preset** names an **AI director**, which says
*who* the opponent is: its **loadout** (what it may do, like a player's), its **strategies** (what proposes moves, in
priority order) and its **selector** (how one is picked), and changes them as the match goes on. The **controller** never
changes per level: it owns timing and submitting. Each decision it copies the board, the rules and both loadouts into a
context and hands it to the **coordinator**, which runs every strategy at once, collects their answers within the think time,
and has the selector order the moves. The controller then submits them **through the same Mediator gate a human uses**, best
first, falling back on a rejection. Then it ends the turn, or decides again if the loadout needs more moves in a turn.

## Diagram

```mermaid
sequenceDiagram
    participant GM as AConnectIt_GameMode
    participant AC as AConnectIt_AIController
    participant DI as UConnectIt_AIDirector (copy)
    participant CO as UConnectIt_AICoordinator
    participant ST as Strategies (director's)
    participant SE as Selector (director's)
    participant MED as UConnectIt_BoardRequestMediator
    participant PS as AI PlayerState / participant

    Note over GM: match start (Adventure)
    GM->>GM: EnsureMatchSetupResolved (preset -> Rules + ResolveWinGroups, AIDirector)
    GM->>AC: SpawnActor(AIControllerClass) + RegisterAIParticipant
    AC->>DI: DuplicateObject(preset's AIDirector) (or a default one)
    AC->>PS: InitialiseActionState(Director.StartingLoadout)
    AC->>DI: OnMatchStarted

    Note over AC: its turn starts (once per turn number)
    AC->>DI: OnAITurnStarted
    AC->>AC: BeginDecision: Director.ApplyPendingChanges; copy Board, Rules, loadouts, AvailableRequestTypes...
    AC->>CO: Begin(Context, Director.Strategies, Director.Selector, think time)
    CO->>ST: StartDecision (all at once; each told budget x share)
    loop every 0.05 s until all finished, or the budget ends and one has a move
        CO->>ST: IsDecisionFinished / GetBestSoFar
    end
    CO->>ST: StopDecision (the unfinished ones)
    CO->>SE: Select(answers) -> Endorse -> RankCandidates -> keep MaxCandidates
    CO->>AC: HandleCoordinatorConcluded(candidates)
    AC->>AC: still current? wait out Director.MinThinkSeconds
    loop candidates, best first, until one is accepted
        AC->>MED: GameMode.ProcessBoardRequest(as the loadout action producing it, own slot)
        MED-->>AC: accepted / rejected
    end
    AC->>PS: CanEndTurn()?
    alt turn-end requirements met
        AC->>PS: ServerSubmitTurnEnd
    else needs more moves (max 16 per turn)
        AC->>AC: BeginDecision again (DecisionIndexThisTurn + 1)
    end
    AC->>DI: OnAITurnEnded
    Note over AC: the opponent's turn starts
    AC->>DI: OnOpponentTurnStarted (a loadout set here applies at once: tells)
```

## Steps

1. **Setup (match start).** [[AConnectIt_GameMode]] resolves the live setup (`EnsureMatchSetupResolved`: the menu's preset, else
   the level config's `DefaultMatchPreset`): copies the preset's rules and **resolves its goal assets into plain win groups**
   (`FConnectItRuleSet::ResolveWinGroups`), keeps the preset's director template, applies the menu's tweaks (target score).
   Then, for `MatchType == Adventure`, `SpawnAndRegisterAI`, and tells the rule set which slot is the AI's (`AIFaction`).
2. **Seeding.** The controller's `BeginPlay` duplicates the director (strategies and selector with it, so decision state is per
   match), creates its coordinator, seeds its PlayerState from the director's `StartingLoadout`, and calls `OnMatchStarted`.
   No director on the preset = the default one (MinMax, First Available) **with no loadout**: every move is rejected.
3. **Turn start.** The participant component notifies `TurnStart`/`TurnActive`; the controller starts one decision per turn
   number. Pause, game over, turn end, timeout or skip cancel it (a pause resumes it if it is still this AI's turn).
4. **Context.** `BeginDecision` first applies the director's pending strategy / selector changes, then copies, on the game
   thread: `Board`, `OwnSlot`, `Rules` (plain thread-safe data), `OwnLoadout`/`OpponentLoadout` (assets: game thread only),
   `TurnNumber`, `DecisionIndexThisTurn`, `AvailableRequestTypes` (produced by its loadout, usable now, and with at least one
   legal instance on the board -- `FConnectItBoardOperation::GetLegality`; a kind of move that can't tell stays in).
   **Nothing available = the AI passes**: it logs `no legal move -- passing` and ends its turn without asking a strategy.
5. **Decide.** The coordinator starts every strategy (`StartDecision` -> `BeginDecision`, C++ or Blueprint), each with
   `TimeBudgetSeconds = think time x StrategyBudgetShare` (MinMax uses that; its own `DefaultTimeBudgetSeconds` only when a
   decision gives none). A strategy answers with `FinishDecision` (a ranked, capped candidate
   list), and may `PublishBestSoFar` while it works; **MinMax publishes each completed search depth** (a thread-safe progress
   holder the game thread reads). The coordinator polls; it concludes when all have finished, or at the think time with each
   strategy's best so far (nothing yet = it keeps waiting, with one warning naming the slow ones).
6. **Select.** The selector merges the answers (`Endorse`: one entry per move, in priority order, counting the strategies that
   proposed it), orders them (`RankCandidates`: First Available, Top N with mistake chance, Greedy Scorer, Weighted by
   Confidence, or a Blueprint one) and keeps `MaxCandidates`.
7. **Think time.** The controller holds the answer until the director's `MinThinkSeconds` have passed since the decision began.
8. **Submit.** `SubmitDecision` re-checks the decision is current, then tries the candidates in order (`SubmitCandidate`): the
   loadout action whose class `ProducesRequestType` the operation's request type, sent through `GameMode->ProcessBoardRequest` as
   its own slot. The first accepted one wins; if every candidate is rejected it logs an error and ends the turn.
9. **The gate.** [[UConnectIt_BoardRequestMediator]] holds the AI to the same rules as a human: the action must be in its loadout,
   produce that request type, be usable now (uses, per-turn cap, cooldown), and the operation must `CanApply` to the board. The
   requester's faction is supplied by the server, never by the payload.
10. **End the turn, or go again.** `PlayerState->CanEndTurn()` true -> `ServerSubmitTurnEnd`. Otherwise the controller decides
    again from the new board, up to 16 times per turn.
11. **The director** hears `OnAITurnStarted` / `OnAITurnEnded`, `OnOpponentTurnStarted`, and `OnWinCheckpointReached` (from the
    server-side committed board change, with the goal asset). It changes the loadout (deferred to the end of the AI's own turn)
    and the strategies / selector (from the next decision).

## Gotchas

- **A strategy that never finishes or publishes** holds up the decision only if nothing else has a move: once the think time is
  up, any strategy's move is enough.
- **A rejected candidate falls through to the next**; only when all are rejected does the AI log an error and end its turn.
  Usual causes: the loadout has no action producing that operation type; the action's uses/caps are spent; the operation
  can't apply (occupied tile).
- **Gameplay reacts to state, not to visual event tags.** The director's events come from committed state (board changes, turn
  notifications). Board event tags play through the visual queue, paced by animations; keying gameplay on them makes it depend
  on animation timing.
- **Strategy and selector state is per controller** (inside its duplicate of the director), reset each match.
- **Threads.** `Rules` and `Board` in the context are plain data and safe to hand to a background task; `OwnLoadout`,
  `OpponentLoadout` and the strategy / selector UObjects are not. Blueprint strategies and all selectors run on the game thread.
  The goal assets ride along in the rules as references only; the search reads the copied conditions.
- **Two factions assumed.** The opponent's loadout is looked up as slot `1 - OwnSlot`.
- **The built-in MinMax only searches Place Piece.** A loadout with swaps or shifts doesn't make MinMax use them; a bespoke
  strategy is how an AI uses those.
- The controller's turn logic is **not overridable from Blueprint**. Customising an AI means the director, its strategies and its
  selector.
- `CheckAndApplyForcedMove` (a blackboard forced move) is a stub that always returns false.

## Cross-impact

- New board operation the AI should be able to play: a strategy must build it; the AI loadout needs an action that produces it
  ([[add-a-board-request-type]]).
- Changing the decision context (`FConnectItAIDecisionContext`) or the candidate (`FConnectItAICandidate`) changes what every
  Blueprint strategy and selector sees.
- Changing turn-end requirement handling (`ATurnBasedPlayerState::CanEndTurn`) changes how many decisions per turn the AI makes.
- The AI reads the match's goals through `Rules`; a staged goal is seen as won only when the last one completes
  ([[add-a-win-condition]]).

## See also

- Recipes: [[set-up-a-custom-ai-opponent]], [[combine-ai-strategies]], [[direct-the-ai-during-a-match]], [[add-a-win-condition]]
- Design: [ai-director-pipeline](../../design/ai-director-pipeline.md), [ai-selector-ideas](../../design/ai-selector-ideas.md),
  [blueprint-ai-strategy-surface](../../design/blueprint-ai-strategy-surface.md),
  [stage-and-match-preset](../../design/stage-and-match-preset.md)
- Decisions: [AI director pipeline](../../_decisions/2026-10-16-ai-director-pipeline.md),
  [AI controller runs its own turn](../../_decisions/2026-10-02-ai-controller-runs-its-own-turn.md)
