---
schema: recipes
task: Set up a match's conditions -- its goals, one or several in order, per side -- as condition assets, react when a side reaches one, and (C++) add a new kind of condition or make a kind of move answer "any legal move?".
touches:
  - a UConnectIt_MatchConditionAsset per goal (Content Browser)
  - the match preset (Rules > Player Win Conditions / AI Win Conditions)
  - (reacting) the GameMode's On Win Checkpoint Reached / the AI director's event
  - (new condition type, C++) a struct deriving FConnectItMatchCondition, Source/ConnectIt/Public/Board/Rules/
  - (legal-move support, C++) FConnectItBoardOperation::HasAnyLegalInstance on an operation type
reconciled: 2026-10-20
commit: f03d10d
---

# Recipe: set up match conditions

(File name kept from "add a win condition" so links hold. The types were renamed on 2026-10-20 from "win condition" to
"match condition"; old assets load through redirects.)

## Goal

A match whose sides win the way you designed: "score 20", "clear the board having scored", or a sequence ("score 10, then clear
the board") where each step is a checkpoint the level can react to. Each **goal** is a **win condition asset**; a match
preset lists each side's goals in the order they must be completed. Completing a goal that isn't the side's last is a
**checkpoint**; completing the last is the **win**.

**Losing** (e.g. "the player is stuck") is designed but not built yet: lose conditions arrive with the
[three directors](../../design/three-directors.md), using these same condition assets.

## Prerequisites

- A match preset ([stage-and-match-preset](../../design/stage-and-match-preset.md)).
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
3. Open the match preset: **Rules > Player Win Conditions**: the player's goals, in order. **Rules > AI Win Conditions**: the
   AI's, in order -- set them whenever the match has an AI (it never borrows the player's; none = it can't win). In a match
   with no AI (online), every faction plays the player's goals.
4. A variant ("score 30 instead") is a **new asset**: assets are shared, so editing one changes every preset using it.
5. Validate the preset (**Asset Actions > Validate**): empty entries and goals with no conditions are reported, and each
   condition's own warnings (e.g. Not Enough Pieces To Score under a scoring rule with no minimum, No Legal Move with a kind
   of move that can't tell).
6. React to checkpoints (optional):
   - **Level script / GameMode Blueprint**: bind **On Win Checkpoint Reached** on the GameMode (server only: Get Game Mode is null
     on clients). It gives the role, faction, **Goal** (the asset), group index and name, after the move's scoring visuals.
   - **The AI**: the director's **On Win Checkpoint Reached** ([[direct-the-ai-during-a-match]]).
   - **Visuals on every machine**: the board event **Win Checkpoint Reached** (its `Asset` field says which goal).
   - Compare the **Goal** asset rather than the index: indexes change when goals are reordered.

### A new kind of condition (C++)

7. A `USTRUCT(BlueprintType, meta = (DisplayName = "..."))` deriving `FConnectItMatchCondition`, next to
   `ConnectIt_ScoreThresholdCondition.h`. Override `IsMetFor(Context, Faction)` -- `Context` has the `Board` and the match's
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

- Output Log, filter `ConnectIt_GameMode`: `faction N completed win group I ('<asset name>')` at each checkpoint; no `has no
  Player Win Conditions` / `win condition I ... has no conditions` warnings at match start.
- The UI's target / progress (board `TargetScore`, `WinProgress`) follows the current goal.

## Pitfalls

- **A goal with no conditions never completes**, and blocks every goal after it.
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
- Recipes: [[set-up-a-custom-ai-opponent]], [[direct-the-ai-during-a-match]]
