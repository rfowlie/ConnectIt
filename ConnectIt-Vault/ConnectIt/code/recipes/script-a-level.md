---
schema: recipes
task: Script an adventure level -- react to the match (turns, checkpoints, triggers you define), change the board or the scoring rule on the level's behalf, and work with the Player and AI directors -- through a Level director Blueprint.
touches:
  - a Blueprint subclass of UConnectIt_LevelDirector (named by the match preset's Level Director Class)
  - condition assets for its triggers
  - (board changes) board operation structs; (rule changes) scoring rule structs
reconciled: 2026-10-21
commit: bbf8daf
---

# Recipe: script a level

## Goal

A level with its own script: "when the player reaches 10 points, the multipliers vanish", "every third turn a blocker
appears", "after the first checkpoint, only vertical lines score". That is the **Level director**: one of three directors
(Level, Player, AI -- [three-directors](../../design/three-directors.md)), the one that speaks for the level itself. It hears
the same events as the others, watches **triggers** you define, and is the one director that changes **the board** and **the
rules**. A side's loadout belongs to that side's director: ask it.

## Prerequisites

- A match preset for the level ([[set-up-a-custom-ai-opponent]] covers the Player / AI directors it also names).
- Condition assets for whatever the level watches for ([[add-a-win-condition]]).

## Steps

1. Content Browser: **Blueprint Class** with parent `ConnectIt_LevelDirector` (e.g. `CILD_Puzzle_Swap_01`). In the match
   preset: **Directors > Level Director Class** = it.
2. **Triggers** (Class Defaults): add entries, each a **Condition** (asset) and a **Side**: **Player** (the human side), **AI**,
   or **Either**. After every committed board change, an armed trigger whose condition holds for its side fires once.
3. Override **On Trigger Fired (Condition, Faction)**: compare **Condition** with your asset variables and react. The trigger
   is now disarmed; **Rearm Trigger (Condition)** arms it again (even from inside this event) so it can fire the next time it
   holds. **Is Trigger Armed** tells you.
4. Override the shared director events as needed: **On Match Started**, **On Turn Started / Ended (Faction, Is Mine, Turn
   Number)** (Is Mine is always false for the level), **On Win Checkpoint Reached**, **On Lose Condition Met**, **On Match
   Won**.
5. Change the board: **Apply Board Operation (Operation, As Faction, Timing)**:
   - **Operation**: **Make Instanced Struct** of a board operation -- **Force Place Piece** (a piece anywhere, even on an
     occupied tile), **Toggle Tile Active**, **Destroy Tile Multiplier**, **Remove Piece**, **Capture Piece**... with its
     fields set.
   - **As Faction**: who it is made by (the piece's owner for a placement); **-1** for one that belongs to nobody (toggling a
     tile).
   - It is checked, scores and can win like a move -- but needs no loadout and spends no action. A refused operation (e.g. a
     tile that doesn't exist) is logged and nothing changes.
6. Change the scoring rule: **Set Scoring Rule (Scoring Rule, Timing)** -- **Make Instanced Struct** of a scoring rule (e.g.
   **Lines** with `Connect Length` / `Scoring Directions` set). Clients see it through the replicated rules (**On Match Rules
   Changed** on the GameState) -- that's where UI showing "only vertical lines score" listens. The AI uses it from its next
   decision (its look-ahead assumes it stays). **Get Match Rules** reads the current rules.
7. **Timing** (both): **Immediately** = now, or -- while a move is being carried out, which is when triggers and checkpoint
   reactions run -- right after that move finishes, never part-way through its scoring. **Next Turn Start** = when the next
   turn begins, before the directors hear it started.
8. Other sides: **Get Player Director (Faction)** / **Get AI Director** -- e.g. a Player director Blueprint with a custom event
   "Grant Swap" that the level calls at a checkpoint; the Player director then changes its own loadout.

### Examples

- **Multipliers vanish at 10 points**: trigger {`CIWC_Score10`, Player} -> On Trigger Fired: for each multiplier tile, Apply
  Board Operation(Destroy Tile Multiplier at that position, -1, Immediately).
- **A blocker every third turn**: On Turn Started: if Turn Number % 3 == 0, Apply Board Operation(Toggle Tile Active at a chosen
  free tile, -1, Next Turn Start).
- **Vertical only after the first checkpoint**: On Win Checkpoint Reached (Goal = phase 1 goal): Set Scoring Rule(Lines with
  only Vertical ticked, Next Turn Start).

## Verify

- Output Log, filter `ConnectIt_`: `level director '<class>'` at match start; `trigger '<asset>' fired for faction N`;
  `level operation '<type>' ... as faction N` then the board change; `scoring rule is now '<struct>'`.
- Refused or over-chained changes: `a level operation could not be carried out`, `more than 64 level changes in a row`.

## Pitfalls

- **Triggers don't win or lose**: a goal is a Win / Lose Condition on a side's director. Triggers are the level reacting.
- **Fires once**: a trigger that should keep firing must be re-armed in its own reaction -- and then fires on every board change
  while its condition holds.
- **Board operations score and can win**: forcing a piece that completes a line scores it for As Faction, and can end the match.
- **The AI's search sees none of this**: it can't simulate triggers or a coming rule change (see
  [rotating-rule-randomness](../../_questions/rotating-rule-randomness.md)).
- **Server only**: the Level director exists only on the server; visuals react to the board events and the replicated rules.
- **React to state, not visual event tags** (see [[direct-the-ai-during-a-match]]).

## See also

- Design: [three-directors](../../design/three-directors.md), [mid-level-rule-changes](../../design/mid-level-rule-changes.md)
- Recipes: [[add-a-win-condition]], [[direct-the-ai-during-a-match]], [[set-up-a-custom-ai-opponent]], [[add-a-board-request-type]]
- Flow: [[ai-turn]]
