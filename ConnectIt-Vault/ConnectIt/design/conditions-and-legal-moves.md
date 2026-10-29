---
type: plan
status: complete
approved: true
approved_on: 2026-10-20
completed_on: 2026-10-20
started:             2026-10-20          # YYYY-MM-DD, when the first stage was built (blank until then)
stages: 6              # how many stages the plan has
current_stage: complete   # the first stage not yet built ("complete" / "not started")
playtested: false
design: "[three-directors](three-directors.md)"
question:
decision: "[2026-10-20-three-directors-and-match-conditions](../_decisions/2026-10-20-three-directors-and-match-conditions.md)"
---

# Plan: MinMax default budget, condition rename + context, Not Enough Pieces, legal moves (part 1 of 2)

> **Status: built** 2026-10-20 (stages 0-5; 42/42 tests); not yet played in PIE. Part 2 (three directors, lose
> conditions) is designed in [three-directors](three-directors.md). Log: [_logs/2026-10-20-1500](../_logs/2026-10-20-1500.md).

## Context

The owner set up the first match presets after the AI director pipeline and came back with notes. The 2026-10-20 discussion
settled a set of changes, then a larger redesign: **three directors** (Level, Player, AI) with the same responsibility in
different contexts.
- Conditions are authored in the directors (each owns its context's: player win/lose, AI win/lose, level triggers).
- Conditions are copied into the rule set at match start, so the server, the AI's search and clients keep evaluating them as now.
- Level conditions are triggers that fire once (re-armable).
- The owner chose to **split**:
  - build now the parts the director redesign doesn't affect (this plan);
  - design the three directors next (lose conditions, the preset shrinking to director classes, level triggers and board
    changes).

Settled for this plan:
- **MinMax:** keeps its request-type settings; its own time budget becomes a real default, `DefaultTimeBudgetSeconds`. The
  director's think time wins whenever set.
- **Full rename** of the condition types to neutral "match condition" names, with redirects.
- **Conditions get a context** (board + rule set). New **Not Enough Pieces To Score**: the scoring rule says its minimum.
- **Legal moves:**
  - each operation type answers "any legal instance on this board?" (Place and Swap now; the others say "unknown");
  - new **No Legal Move: X** condition (one condition with a Move picker, board-only);
  - a Blueprint helper for the UI;
  - `Available Request Types` drops moves with no legal instance;
  - the AI **passes** (ends its turn) when nothing is legal.
- Conditions are a game-rules tool, not bound by what MinMax can model.
- Until the directors land, the new conditions can only be used in the existing **win** lists. Their main use, lose lists,
  arrives with part 2.

Each stage builds and passes the tests before the next. The editor must be closed for builds. The owner commits between stages.
Nothing is committed by me.

## Stage 0: move this plan

- Copy this file to `ConnectIt-Vault/ConnectIt/design/conditions-and-legal-moves.md`, then delete the staging file.

## Stage 1: MinMax default time budget

- `Public/AI/ConnectIt_MinMaxSearch.h`: `FConnectItMinMaxSearchSettings::TimeBudgetSeconds` → `DefaultTimeBudgetSeconds`
  ("used when the decision gives no budget, e.g. the Blueprint node called without one").
- `Private/AI/ConnectIt_MinMaxSearch.cpp` `Prepare`: budget = `Context.TimeBudgetSeconds > 0 ? Context.TimeBudgetSeconds :
  Settings.DefaultTimeBudgetSeconds`. This replaces today's min-of-both.
- `Config/DefaultEngine.ini` `[CoreRedirects]`: `+PropertyRedirects` for `ConnectItMinMaxSearchSettings.TimeBudgetSeconds` →
  `DefaultTimeBudgetSeconds`, so entered values carry over.
- Tests: callers renamed; `ConnectIt.AI.MinMax.SearchTool` checks that the context's budget wins when set and the default is
  used otherwise.

## Stage 2: rename to "match condition" (no behaviour change)

| Old | New |
|---|---|
| `FConnectItWinCondition` (`ConnectIt_WinCondition.h`) | `FConnectItMatchCondition` (`ConnectIt_MatchCondition.h`) |
| `FConnectItWinCondition_ScoreThreshold` | `FConnectItMatchCondition_ScoreThreshold` (`ConnectIt_ScoreThresholdCondition.h`) |
| `FConnectItWinCondition_PieceCount` | `FConnectItMatchCondition_PieceCount` (`ConnectIt_PieceCountCondition.h`) |
| `FConnectItWinConditionGroup` | `FConnectItMatchConditionGroup` (`ConnectIt_MatchConditionGroup.h`) |
| `UConnectIt_WinConditionAsset` | `UConnectIt_MatchConditionAsset` (`ConnectIt_MatchConditionAsset.h`) |
| test `FConnectItWinCondition_TestCorner` | `FConnectItMatchCondition_TestCorner` |

- **Redirects:** `+StructRedirects` / `+ClassRedirects` in `DefaultEngine.ini` for each. Existing assets, instanced-struct
  types and Blueprint pins keep working.
- **Unchanged:** the rule-set property names (`PlayerWinConditions`, `PlayerWinGroups`...); "win" is right there.
- **Callers:** every user is updated (rule set, board events, director, GameMode, preset, tests, includes).

## Stage 3: conditions get a context; Not Enough Pieces To Score

- **Condition API** (`ConnectIt_MatchCondition.h`):
  - New `FConnectItConditionContext { const FConnectItBoardState& Board; const FConnectItRuleSet& Rules; }`. It's plain C++
    (the rule set is forward-declared) and safe on the search thread.
  - The base gets `virtual bool IsMetFor(const FConnectItConditionContext&, int32 Faction) const` (default false) and
    `GetProgress(Context, Faction)`. These replace the conditions' `HasFactionWon(Board, Faction)` / `GetWinningFaction(Board)`;
    `GetWinningFaction` leaves the conditions.
  - Score Threshold, Piece Count, the group and the test condition are ported. The rule set's own match-level
    `HasFactionWon` / `GetWinningFaction` stay and build the context (`AdvanceWinGroups`, `GetProgress`, target-score paths).
- **Scoring minimum:** `FConnectItScoringRule::GetMinPiecesToScore()` (default 0 = unknown). Lines returns `ConnectLength`.
- **New `FConnectItMatchCondition_NotEnoughPiecesToScore`** (DisplayName "Not Enough Pieces To Score"):
  - Met when the scoring rule's minimum is > 0 and the faction's pieces on the board (Piece Count's `CountPieces`, reused) are
    fewer than it.
  - Its comment says it's for levels where a side can't add pieces.
- **Validation:** the condition asset gets a hook so a condition can report its own issues, e.g. `virtual void
  GetValidationWarnings(TArray<FText>&) const` on the base, called from `UConnectIt_MatchConditionAsset::IsDataValid`.
- **Tests:**
  - existing condition tests ported;
  - new `ConnectIt.Board.Conditions.NotEnoughPiecesToScore`: Lines(4) with 3 / 4 / 0 pieces; Lines(5); a scoring rule with no
    minimum never holds.

## Stage 4: legal moves; No Legal Move; AI pass

- **Per operation type** (`Public/Board/Operations/ConnectIt_BoardOperation.h`, `Private/Board/Operations/ConnectIt_BoardOperations.cpp`):
  - New `UENUM EConnectItMoveLegality { Unknown, Legal, NoLegalMove }`.
  - `virtual EConnectItMoveLegality HasAnyLegalInstance(const FConnectItBoardState&, int32 Faction) const` (default Unknown).
    Overrides:
    - **Place**: any tile where `IsTilePlaceableAt` holds.
    - **Swap**: the faction has a piece, and some other occupied tile isn't the faction's (the `CanApply` rule).
  - Static `FConnectItBoardOperation::GetLegality(FGameplayTag RequestType, Board, Faction)`, through a cached request-type →
    default-instance table. It's built beside `GetAllRequestTypes` by the same reflection, warmed on the game thread at match
    setup (and in tests), and only read afterwards.
- **New `FConnectItMatchCondition_NoLegalMove`** (DisplayName "No Legal Move"):
  - `Move` (request-type tag). Met when `GetLegality` is NoLegalMove; Unknown never holds.
  - A validation warning when its move type answers Unknown ("not supported yet").
  - "Can't place and can't swap" = two conditions in one asset.
- **Blueprint helper:** `UConnectIt_BoardStateLibrary::HasAnyLegalMove(Board State, Move, Faction)` → `EConnectItMoveLegality`,
  for the UI's "grey out actions with no viable use".
- **AI controller** `BeginDecision` (`Private/Framework/Controller/ConnectIt_AIController.cpp`):
  - `AvailableRequestTypes` also drops types whose legality is NoLegalMove (Unknown stays).
  - If none remain, it logs `no legal move -- passing` and calls `ServerSubmitTurnEnd`. The server doesn't require
    `CanEndTurn`, so this also covers the second-and-later decisions of a turn.
- **Tests:**
  - new `ConnectIt.Board.Conditions.NoLegalMove`:
    - Place: full board vs one free tile;
    - Swap: no opponent piece vs one;
    - an unsupported type answers Unknown and never holds;
  - the Blueprint helper agrees.
  - The controller pass is covered in PIE.

## Stage 5: vault

- **New `design/three-directors.md`**: the design settled on 2026-10-20, to be planned next.
  - **The directors:** Level, Player and AI, on a shared base with the same events, their own condition lists, and a broadcast
    when a condition is met.
    - The Player director: one instance per human player, owned by the GameMode.
    - The Level director: one instance, owned by the GameMode.
    - The AI director: owned by the AI controller.
  - **Conditions:** authored in the directors, copied into the rule set at match start, which stays the evaluated, replicated,
    thread-safe copy. Queried on the server through the directors, on clients through the rules and board.
  - **Each director owns its own context's conditions:**
    - the Player director: the player's win and lose conditions;
    - the AI director: the AI's;
    - the Level director: level triggers, which fire once and can be re-armed.
  - **Ownership rule:** only a side's director changes that side's loadout; the Level director changes the board, which needs
    a server entry point to carry out an operation outside a player's move.
  - **The preset** shrinks to display name, scoring rule and the three director classes, replacing the earlier
    "preset names an AI director class" idea.
  - **Lose-condition rules agreed earlier:**
    - win goals are checked first and beat a loss;
    - a `Lose Condition Met` event, then `Game Won` for the other side;
    - an `Opponent Lost` match-end reason;
    - the search treats a loss as game over;
    - online is undecided.
  - **History:** why the inline director in the preset was clunky (silent overwrite risk).
- **`design/swap-puzzles.md`:**
  - the AI must move first, or AI pieces must start on the board;
  - long and crossing lines clear more than 4, so "divisible by the line length" isn't enough;
  - the AI clearing its own pieces leaves no swap target;
  - stuck states, via Not Enough Pieces To Score and No Legal Move: Swap, are meant as player lose conditions once directors
    land.
- **New `design/content-folders.md`:** per-level folders plus `Shared/`, prefixes, Fix Up Redirectors, and the
  `DefaultGame.ini` level-config paths I update after the owner has moved them.
- **Wishlist:** a reusable board data asset (a line in `design/stage-and-match-preset.md`).
- **Recipes:**
  - `add-a-win-condition.md`: the renamed types, the two new conditions, legality support.
  - `set-up-a-custom-ai-opponent.md`: Default Time Budget Seconds.
  - `code/systems/ai-turn.md`: the AI pass, the available-move filter, the budget.
- **`_questions/`:** `lose-conditions-online.md`, `out-of-uses-condition.md`, `level-trigger-board-changes.md` (each with
  current thinking).
- **Records:**
  - decision `_decisions/2026-10-20-three-directors-and-match-conditions.md`, plus its index line. It covers the director
    split, "authored in directors, evaluated in rules", the rename, and "conditions aren't bound by MinMax";
  - `2026-10-16-ai-director-pipeline` marked partly superseded;
  - log `_logs/2026-10-20-HHMM.md`, plus its index line.

## Owner-side asset work (listed in the log)

- Re-check MinMax `Default Time Budget Seconds`: the redirect keeps the value, but it now means "only when no think time is
  given".
- Resave condition assets once after the rename.
- Optional UI: grey out actions using `Has Any Legal Move`.
- The folder moves: tell me when the level configs have moved, so I can update `DefaultGame.ini`.

## Critical files

- `Source/ConnectIt/Public/Board/Rules/`: the condition, group and asset headers (renamed), `ConnectIt_RuleSet.h/.cpp`,
  `ConnectIt_ScoringRule.h`, `ConnectIt_LineScoringRule.h`.
- `Public/Board/Operations/ConnectIt_BoardOperation.h`, `Private/Board/Operations/ConnectIt_BoardOperations.cpp`.
- `Public/Board/Events/ConnectIt_BoardEvents.h` (renamed asset type), `Public/AI/ConnectIt_AIDirector.h`,
  `ConnectIt_GameMode.h/.cpp`, `ConnectIt_MatchPreset.cpp`.
- `Public/AI/ConnectIt_MinMaxSearch.h/.cpp`, `Private/Framework/Controller/ConnectIt_AIController.cpp`,
  `Public/Framework/Library/ConnectIt_BoardStateLibrary.h/.cpp`.
- `Config/DefaultEngine.ini` (`[CoreRedirects]`).
- Tests: `Private/Tests/ConnectIt_MinMaxTests.cpp`, `ConnectIt_MinMaxTestTerms.h`.

## Verification

- **Each stage:** `Build.bat ConnectItEditor Win64 Development` (editor closed), then `UnrealEditor-Cmd ...
  -ExecCmds="Automation RunTests ConnectIt; Quit"`. All tests pass, new ones included.
- **Owner PIE after stage 4:**
  - the presets still load (renamed types via redirects);
  - the swap level's AI passes with `no legal move -- passing` when it has nothing to do;
  - a MinMax opponent still thinks for the director's think time.

## Risks / notes

- The rename relies on redirects for existing assets and Blueprint pins. Verify by opening both presets and the condition assets
  after Stage 2.
- The condition API change touches every condition. There are only three, plus the test one.
- Legality for the other six operation types answers Unknown until each is implemented, so No Legal Move with those never holds
  (validation says so).
