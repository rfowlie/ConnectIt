# Three directors: Level, Player, AI

> **Status: built** 2026-10-20..21 (plan stages 1-6, 47/47 tests); not yet played in PIE. The open points below were
> settled in the plan ([three-directors-plan](three-directors-plan.md)) and the decision
> [2026-10-21-director-hub-and-level-changes](../_decisions/2026-10-21-director-hub-and-level-changes.md).

Settled with the owner on 2026-10-20, after setting up the first presets; **not built yet** -- it is part 2 of the plan whose
part 1 is [conditions-and-legal-moves](conditions-and-legal-moves.md). **Plan: [three-directors-plan](three-directors-plan.md)**
(2026-10-20; adds Blueprint rule changes owned by the Level director, and settles the open points below).
Decision: [2026-10-20-three-directors-and-match-conditions](../_decisions/2026-10-20-three-directors-and-match-conditions.md).

## Why

- **The AI isn't the only thing that reacts to a match.** A level wants to script the board (one-off changes, triggers),
  and a player's side wants its own reactions (loadout changes at a checkpoint) -- today those live in level Blueprints and
  the GameMode's delegates, or nowhere.
- **The director in the preset was clunky.** Editing a director inline in the match preset cluttered it, and since every
  adventure level gets its own director subclass, values set in the preset could be **silently overwritten** by the
  Blueprint at match start. One place for a director's configuration -- its Blueprint's Class Defaults -- removes that risk.
- **Conditions belong with whoever reacts to them.** A level's goals and losses are part of that level's script.

## The shape

- **A shared base director**, three kinds with the same responsibility in different contexts:
  - **Level director** -- the level's own script: **level conditions** (triggers) and **board changes**. One instance,
    owned by the GameMode. Since 2026-10-22 it also names the level's **level-wide tile powers** (`Level Wide Powers`:
    tags; the tile catalog defines them).
  - **Player director** -- a human side: its starting loadout, its **win and lose conditions**, loadout changes during play.
    One instance per human player, owned by the GameMode (in versus both players get an instance of the same class).
  - **AI director** -- what `UConnectIt_AIDirector` is today (loadout, strategies, selector, think time) plus the AI's
    **win and lose conditions**. Owned by the AI controller.
- **The base gives every director**: world access; events for match start, any side's turn start / end; its own condition
  lists; and a **broadcast when one of its conditions is met** (and the checkpoint events it has today). Other classes can
  query a director's conditions on the server.
- **The match preset shrinks** to display name, scoring rule and the **three director classes**. Classic / versus presets
  reuse generic Player and AI directors; an adventure level has its own Level director and bespoke side directors where
  needed. (This replaces the briefly-planned "preset names an AI director class".)
- **Ownership rule** (no tug-of-war): only a side's director changes that side's loadout; the Level director changes the
  board.

## Conditions: authored in directors, evaluated in the rules

- Directors are where conditions are **authored** (lists of condition assets) and where **reactions** live.
- At match start their lists are **copied into the rule set**, which stays the evaluated copy: the server after each move,
  the AI's background search, and clients (the rules replicate) keep working exactly as now. Directors are server-only and
  UObjects, so they can't be that copy themselves.
- Querying: on the server through the directors; on clients through the replicated rules and board.

## Lose conditions (rules agreed on 2026-10-20)

- Per side (Player director / AI director), in matches with an AI. **Online: undecided** (see
  [lose-conditions-online](../_questions/lose-conditions-online.md)).
- **Any one** lose condition holding for a side after a move = that side loses (unlike win goals, which are an ordered
  sequence).
- **Win goals are checked first**: a move that completes a win and meets a lose condition is a win.
- A **`Lose Condition Met`** board event (faction + the asset), then the usual **Game Won** for the other side; a new
  match-end reason **Opponent Lost**.
- The AI's search treats a loss as game over (it already asks the rule set who has won).
- Typical use: an exhaustion level lists *Not Enough Pieces To Score* and/or *No Legal Move: Swap Pieces* (built in part 1)
  as the player's lose conditions.

## Level conditions (triggers)

- Owned by the Level director; **fire once, the first time they are met; the director can re-arm one**.
- Not part of win / loss, and **not seen by the AI's search** (it can't simulate Blueprint reactions).
- Their reactions change the board, which needs a **server entry point to carry out a board operation outside a player's
  move** (no loadout, no requesting player) through the normal rule set, board commit and events. The operations exist
  (Force Place, Toggle Active, Destroy Multiplier...); the entry point doesn't. See
  [level-trigger-board-changes](../_questions/level-trigger-board-changes.md).

## Still to settle when planning

- Turn-event ordering between directors when several react to the same event.
- Whether the GameMode's level-Blueprint delegates (`OnWinCheckpointReached`, `OnMatchWon`) stay once the Level director
  exists.
- Migration: the presets' current inline AI director and win lists move into director Blueprints.
