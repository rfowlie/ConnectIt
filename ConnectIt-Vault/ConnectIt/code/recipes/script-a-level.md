---
schema: recipes
task: Script an adventure level -- set both sides' goals, react to the match (turns, checkpoints, triggers you define), change the board, the scoring rule or a side's goals on the level's behalf, and work with the Player and AI directors -- through a Level director Blueprint.
touches:
  - Source/ConnectIt/Public/AI/ConnectIt_LevelDirector.h (a Blueprint subclass, named by the match preset's Level Director Class -- required)
  - Source/ConnectIt/Public/Board/Rules/Conditions/ConnectIt_MatchConditionDataAsset.h (condition assets for its goals and triggers)
  - Source/ConnectIt/Public/Board/Operations/ConnectIt_BoardOperations.h (board changes: board operation structs)
  - Source/ConnectIt/Public/Board/Rules/Scoring/ConnectIt_ScoringRule.h (rule changes: scoring rule structs)
  - Source/ConnectIt/Public/Board/Actor/ConnectIt_BuilderTile.h (tiles: builder tiles placed in the level)
  - Source/ConnectIt/Public/Board/Powers/ConnectIt_TileCatalogDataAsset.h (the map's tile catalog, in World Settings; level-wide powers: Level Wide Powers)
reconciled: 2026-10-23
commit: 54c1284
status: 'current'
---

# Recipe: script a level

## Goal

A level with its own script: "when the player reaches 10 points, the multipliers vanish", "every third turn a blocker
appears", "after the first checkpoint, only vertical lines score". That is the **Level director**: one of three directors
(Level, Player, AI -- [three-directors](../../design/three-directors.md)), the one that speaks for the level itself. It hears
the same events as the others, holds **both sides' goals**, watches **triggers** you define, and is the one director that changes
**the board**, **the rules** and **the goals**. A side's loadout belongs to that side's director: ask it. A level that changes
layout and goals as it goes is a phased level: [[script-a-phased-level]].

## Prerequisites

- A match preset for the level ([[set-up-a-custom-ai-opponent]] covers the Player / AI directors it also names).
- Condition assets for whatever the level watches for ([[add-a-win-condition]]).

## Steps

1. Content Browser: **Blueprint Class** with parent `ConnectIt_LevelDirector` (e.g. `CILD_Puzzle_Swap_01`). In the match
   preset: **Directors > Level Director Class** = it (required -- a preset without one is invalid: nobody would have goals).
   **Level|Conditions**: each side's **Win / Lose Conditions** ([[add-a-win-condition]]).
2. **Triggers** (Class Defaults): add entries, each a **Condition** (asset) and a **Side**: **Player** (the human side), **AI**,
   or **Either**. After every committed board change, an armed trigger whose condition holds for its side fires once.
3. Override **On Trigger Fired (Condition, Faction)**: compare **Condition** with your asset variables and react. The trigger
   is now disarmed; **Rearm Trigger (Condition)** arms it again (even from inside this event) so it can fire the next time it
   holds. **Is Trigger Armed** tells you.
4. Override the shared director events as needed: **On Match Started**, **On Turn Started / Ended (Faction, Is Mine, Turn
   Number)** (Is Mine is always false for the level), **On Win Checkpoint Reached**, **On Lose Condition Met**, **On Match
   Won**, **On Phase Changed** (a phased level).
5. Change the board: **Apply Board Operation (Operation, As Faction, Timing)**:
   - **Operation**: **Make Instanced Struct** of a board operation -- **Force Place Piece** (a piece anywhere, even on an
     occupied tile), **Toggle Tile Active**, **Destroy Tile Multiplier**, **Remove Piece**, **Capture Piece**, **Move
     Piece**, **Adjust Multiplier**, **Set Tile Locked**, **Set Chain Partner** ([[lock-a-tile-or-chain-tiles]])... with
     its fields set.
   - **As Faction**: who it is made by (the piece's owner for a placement); **-1** for one that belongs to nobody (toggling a
     tile).
   - It is checked, scores and can win like a move -- but needs no loadout and spends no action. Its tile powers fire as
     after any move ([[tile-power-chain]]). A refused operation (e.g. a tile that doesn't exist, or a locked tile it may
     not act on) is logged with the reason and nothing changes.
6. Change the scoring rule: **Set Scoring Rule (Scoring Rule, Timing)** -- **Make Instanced Struct** of a scoring rule (e.g.
   **Lines** with `Connect Length` / `Scoring Directions` set). Clients see it through the replicated rules (**On Match Rules
   Changed** on the GameState) -- that's where UI showing "only vertical lines score" listens. The AI uses it from its next
   decision (its look-ahead assumes it stays). **Get Match Rules** reads the current rules.
   Change a side's goals: **Set Win Conditions / Set Lose Conditions (Side, Conditions, Timing)**. Phases and layouts: **Begin
   Phase**, **Set Layout** ([[script-a-phased-level]]).
7. **Timing** (all of them): **Immediately** = now, or -- while a board change is resolving, which is when triggers and checkpoint
   reactions run -- once that change has played out, its visuals included. Several play one after another, each with its own
   visuals ([one board change at a time](../../_decisions/2026-10-21-one-board-change-at-a-time.md)). **Next Turn Start** =
   when the next turn begins: they join the same line, before the directors hear the turn started. The player can't act,
   and the AI doesn't decide, until they have all played out.
8. **The level's tiles:** block the board out with **builder tiles** (`ConnectIt_BuilderTile` Blueprints). Each has its
   per-tile settings and, for a powered tile, a **Tile Power Tag**. The map's World Settings **Tile Catalog** (else the project
   default) decides which tile actor shows each tag. Builders become those actors when the match starts. In the editor, the
   layout tool's **Preview Tile Catalog** shows them (never saved). Powers that apply to every tile: the tags in this
   director's **Level Wide Powers**. Making a power: [[add-a-tile-power]].
9. Other sides: **Get Player Director (Faction)** / **Get AI Director** -- e.g. a Player director Blueprint with a custom event
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

- **Triggers don't win or lose**: a goal is one of this director's Player / AI Win / Lose Conditions. Triggers are the level
  reacting.
- **Fires once**: a trigger that should keep firing must be re-armed in its own reaction -- and then fires on every board change
  while its condition holds.
- **Board operations score and can win**: forcing a piece that completes a line scores it for As Faction, and can end the match.
- **The AI's search sees none of this**: it can't simulate triggers or a coming rule change (see
  [rotating-rule-randomness](../../_questions/rotating-rule-randomness.md)).
- **Server only**: the Level director exists only on the server; visuals react to the board events and the replicated rules.
- **React to state, not visual event tags** (see [[direct-the-ai-during-a-match]]).

## See also

- Design: [three-directors](../../design/three-directors.md), [mid-level-rule-changes](../../design/mid-level-rule-changes.md)
- Recipes: [[script-a-phased-level]], [[add-a-tile-power]], [[add-a-win-condition]], [[direct-the-ai-during-a-match]], [[set-up-a-custom-ai-opponent]],
  [[add-a-board-request-type]]
- Flow: [[ai-turn]]
