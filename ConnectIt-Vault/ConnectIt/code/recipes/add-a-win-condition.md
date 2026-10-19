---
schema: recipes
task: Give a match its goals -- one or several in order, per side -- as win condition assets, react when a side reaches one, and (C++) add a new kind of condition.
touches:
  - a UConnectIt_WinConditionAsset per goal (Content Browser)
  - the match preset (Rules > Player Win Conditions / AI Win Conditions)
  - (reacting) the GameMode's On Win Checkpoint Reached / the AI director's event
  - (new condition type, C++) a struct deriving FConnectItWinCondition, Source/ConnectIt/Public/Board/Rules/
reconciled: 2026-10-19
commit: fd369a4
---

# Recipe: add a win condition

## Goal

A match whose sides win the way you designed: "score 20", "clear the board having scored", or a sequence ("score 10, then clear
the board") where each step is a checkpoint the level can react to. Each **goal** is a **win condition asset**; a match
preset lists each side's goals in the order they must be completed. Completing a goal that isn't the side's last is a
**checkpoint**; completing the last is the **win**.

## Prerequisites

- A match preset ([stage-and-match-preset](../../design/stage-and-match-preset.md)).
- The condition types that exist: **Score Threshold** (`Win Score Threshold`: reach this score) and **Piece Count**
  (`Comparison` + `Count`: e.g. Equal 0 = no pieces of yours left on the board).

## Steps

1. Content Browser: **Data Asset** of class `ConnectIt_WinConditionAsset` (e.g. `WC_Score20`, `WC_ClearBoard`).
2. **Display Name**: what UI shows ("Score 20"). **Conditions**: add one or more and pick each type; **all of them must hold at
   the same time** (Piece Count Equal 0 + Score Threshold 4 = clear the board, having scored 4).
3. Open the match preset: **Rules > Player Win Conditions**: the player's goals, in order. **Rules > AI Win Conditions**: the
   AI's, in order -- set them whenever the match has an AI (it never borrows the player's; none = it can't win). In a match
   with no AI (online), every faction plays the player's goals.
4. A variant ("score 30 instead") is a **new asset**: assets are shared, so editing one changes every preset using it.
5. Validate the preset (**Asset Actions > Validate**): empty entries and goals with no conditions are reported.
6. React to checkpoints (optional):
   - **Level script / GameMode Blueprint**: bind **On Win Checkpoint Reached** on the GameMode (server only: Get Game Mode is null
     on clients). It gives the role, faction, **Goal** (the asset), group index and name, after the move's scoring visuals.
   - **The AI**: the director's **On Win Checkpoint Reached** ([[direct-the-ai-during-a-match]]).
   - **Visuals on every machine**: the board event **Win Checkpoint Reached** (its `Asset` field says which goal).
   - Compare the **Goal** asset rather than the index: indexes change when goals are reordered.

### A new kind of condition (C++)

7. A `USTRUCT(BlueprintType, meta = (DisplayName = "..."))` deriving `FConnectItWinCondition`, next to
   `ConnectIt_ScoreThresholdWinCondition.h`. Override `HasFactionWon(Board, Faction)` (or `GetWinningFaction` if it names a
   single winner), `GetProgress` (0..1, feeds progress bars), and only if score-based `GetTargetScore` / `SetTargetScore`.
8. Keep it **plain data**: it runs on the server, on clients and in the AI's background search. Read only the board it is
   given; no UObjects, no world access.
9. It then appears in the asset's Conditions type picker.

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

## See also

- Design: [win-condition-stages](../../design/win-condition-stages.md),
  [ai-director-pipeline](../../design/ai-director-pipeline.md) (why goals are assets)
- Recipes: [[set-up-a-custom-ai-opponent]], [[direct-the-ai-during-a-match]]
