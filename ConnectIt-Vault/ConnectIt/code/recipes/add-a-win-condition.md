---
schema: recipes
task: Set up a match's conditions -- each side's goals (in order) and ways to lose, plus the level's triggers -- as condition assets on the directors, react when one is met, and (C++) add a new kind of condition or make a kind of move answer "any legal move?".
touches:
  - Source/ConnectIt/Public/Board/Rules/Conditions/ConnectIt_MatchConditionAsset.h (a condition asset per condition, Content Browser)
  - Source/ConnectIt/Public/AI/ConnectIt_LevelDirector.h (the Level director Blueprint: Player / AI Win and Lose Conditions, Triggers; Set Win / Lose Conditions mid-match)
  - Source/ConnectIt/Public/AI/ConnectIt_Director.h (reacting: the directors' events)
  - Source/ConnectIt/Public/Board/Rules/Conditions/ConnectIt_MatchConditions.h (new condition type, C++: a struct deriving FConnectItMatchCondition)
  - Source/ConnectIt/Public/Board/Operations/ConnectIt_BoardOperation.h (legal-move support, C++: FConnectItBoardOperation::HasAnyLegalInstance on an operation type)
reconciled: 2026-10-23
commit: 54c1284
---

# Recipe: set up match conditions

(File name kept from "add a win condition" so links hold. The types were renamed on 2026-10-20 from "win condition" to
"match condition"; old assets load through redirects.)

## Goal

A match whose sides win and lose the way you designed: "score 20", "clear the board having scored", or a sequence ("score 10,
then clear the board") where each step is a checkpoint; "the player is stuck" as a loss. Each is a **condition asset**, listed
on the **Level director** -- both sides' goals live there, so a level's whole script is in one place and the Player / AI
directors stay reusable loadout and AI setup ([conditions on the Level director](../../_decisions/2026-10-22-conditions-on-the-level-director.md)):

- **Player / AI Win Conditions**: each side's goals in the order they must be completed. Completing one that isn't the
  side's last is a **checkpoint**; completing the last is the **win**.
- **Player / AI Lose Conditions**: if **any one** holds after a move, that side **loses** and the other side wins -- unless a
  goal was completed in the same move (wins are checked first). Only in a match with an AI.
- **Triggers**: conditions the level reacts to, without winning or losing ([[script-a-level]]).
- **Mid-match**: **Set Win Conditions / Set Lose Conditions (Side, Conditions, Timing)** replace a side's list (a board change;
  that side's goal progress restarts, score carries over -- see [[script-a-phased-level]]).

## Prerequisites

- A match preset naming a **Level director** (required -- it holds the goals; a generic one per standard way of playing),
  a Player director, and an AI director for a match against the AI -- see [[script-a-level]] and
  [[set-up-a-custom-ai-opponent]].
- The condition types that exist:
  - **Score Threshold** (`Win Score Threshold`): reach this score.
  - **Piece Count** (`Comparison` + `Count`): e.g. Equal 0 = no pieces of yours left on the board.
  - **Not Enough Pieces To Score**: fewer of your pieces on the board than the scoring rule needs for a score (Lines: its
    Connect Length). For levels where a side can't add pieces; mostly meant as a lose condition.
  - **No Legal Move** (`Move`: a kind of move, e.g. Swap Pieces): you have no legal move of that kind on the board. Board
    only, not your uses left. Place Piece and Swap Pieces can answer it today; other kinds can't yet (it never holds, and
    validation warns). "Can't place and can't swap" = two of these in one asset.

## Steps

1. Content Browser: **Data Asset** of class `ConnectIt_MatchConditionAsset` (e.g. `CIWC_Score20`, `CIWC_ClearBoard`).
2. **Display Name**: what UI shows ("Score 20"). **Conditions**: add one or more and pick each type; **all of them must hold at
   the same time** (Piece Count Equal 0 + Score Threshold 4 = clear the board, having scored 4).
3. Open the **Level director** Blueprint's Class Defaults (**Level|Conditions**): **Player Win Conditions** = the player's
   goals, in order; **Player Lose Conditions** = ways the player loses (e.g. an exhaustion level: *Not Enough Pieces To
   Score*, *No Legal Move: Swap Pieces*). **AI Win / Lose Conditions**: the AI's own -- set its goals whenever the match has an
   AI (it never borrows the player's; none = it can't win). In a match with no AI (online), every faction plays the player's
   goals, and lose conditions are ignored.
4. A variant ("score 30 instead") is a **new asset**: assets are shared, so editing one changes every director using it.
5. Validate the preset (**Asset Actions > Validate**): it validates its directors -- empty entries and assets with no
   conditions are reported -- and each condition's own warnings given the preset's rules (e.g. Not Enough Pieces To Score
   under a scoring rule with no minimum, No Legal Move with a kind of move that can't tell).
6. React (optional):
   - **Any director** (Level, Player, AI): **On Win Checkpoint Reached** (Faction, Is Mine, **Goal**, Group Index, Group
     Name), **On Lose Condition Met** (Faction, Is Mine, Condition), **On Match Won** -- server side, from the committed
     change, before its visuals ([[script-a-level]], [[direct-the-ai-during-a-match]]).
   - **Visuals on every machine**: the board events **Win Checkpoint Reached** / **Lose Condition Met** (`Asset` says which;
     *Get Active ... Event* nodes).
   - Compare the **Goal** asset rather than the index: indexes change when goals are reordered.

### A new kind of condition (C++)

7. A `USTRUCT(BlueprintType, meta = (DisplayName = "..."))` deriving `FConnectItMatchCondition`, added to the family file
   `Board/Rules/Conditions/ConnectIt_MatchConditions.h` with the others (Score Threshold, Piece Count...). Override `IsMetFor(Context, Faction)` -- `Context` has the `Board` and the match's
   `Rules` (e.g. `Rules.GetScoringRule()->GetMinPiecesToScore()`) -- `GetProgress(Context, Faction)` (0..1, feeds progress
   bars), only if score-based `GetTargetScore` / `SetTargetScore`, and optionally `GetValidationWarnings(Rules, Out)` (Rules
   is null when the asset is validated on its own).
8. Keep it **plain data**: it runs on the server, on clients and in the AI's background search. Read only the context it is
   given; no UObjects, no world access.
9. It then appears in the asset's Conditions type picker.

### A kind of move answering "any legal move?" (C++)

10. In `ConnectIt_BoardOperations.h/.cpp`, override `HasAnyLegalInstance(Board, ForFaction)` on the operation struct: return
    `Legal` if any instance could be applied for that faction (use the same rule as its `CanApply`), else `NoLegalMove`. It
    is asked of a default instance, so ignore the struct's own fields. Place Piece and Swap Pieces are the examples.
11. That makes **No Legal Move** work for it, the Blueprint node **Has Any Legal Move (Board State)** answer it (for greying
    out actions), and the AI drop it from `Available Request Types` (and pass if nothing is left) when there is none.

## Verify

- Output Log, filter `ConnectIt_`: `faction N completed win group I ('<asset name>')` at each checkpoint; `faction N met lose
  condition '<asset>'` on a loss; no `Level director has no Player / AI Win Conditions` warnings at match start. The match result's reason is
  **Opponent Lost** after a loss.
- The UI's target / progress (board `TargetScore`, `WinProgress`) follows the current goal.

## Pitfalls

- **A goal with no conditions never completes**, and blocks every goal after it.
- **Goals are read from the director's class** when the match starts: changing a director's lists during a match does nothing.
- **Lose conditions are ignored in a match with no AI** (online is undecided; the GameMode warns).
- **The main menu's target score changes the LAST goal of each side** (the final goal); checkpoint goals keep their values. It
  changes the match's copy, never the asset.
- **Editing a goal asset during play** does nothing to the running match: its conditions were copied when the match started.
- **The AI plays toward its own goals** ([[ai-turn]]): MinMax sees a staged goal as won only when the last one completes.
- **Not Enough Pieces To Score in a level where pieces are placed** holds early (2 pieces on the board is normal there).
- Conditions are a game-rules tool and aren't limited by what MinMax can plan around.

## See also

- Design: [win-condition-stages](../../design/win-condition-stages.md),
  [ai-director-pipeline](../../design/ai-director-pipeline.md) (why goals are assets),
  [conditions-and-legal-moves](../../design/conditions-and-legal-moves.md), [three-directors](../../design/three-directors.md)
- Recipes: [[set-up-a-custom-ai-opponent]], [[direct-the-ai-during-a-match]], [[script-a-level]]
