---
Date: 2026-10-22
status: Active
superseded by:
tags:
  - architecture
  - win-conditions
  - level-design
---
## Decision

**Both sides' goals live on the Level director:** Player / AI Win Conditions and Player / AI Lose Conditions, in its Class
Defaults. The GameMode resolves them from the Level director's class at match start. The Player and AI directors no longer
have goal lists; they are loadout and AI setup.

- **Only the Level director changes conditions mid-match**, with **Set Win Conditions / Set Lose Conditions (Side,
  Conditions, Timing)**.
- **A goals change is a board change:**
  1. the rules are re-resolved and replicate;
  2. a Change Goals operation restarts that side's goal progress (`GroupsCompleted`) and plays `Goals Changed`;
  3. the normal win check follows.
  Score carries over, so goals already met complete at once.
- **A match preset needs a Level Director Class** (none = nobody has goals: an error). A standard way of playing (classic,
  versus) gets a generic Level director carrying its goals.

## Why

- **Conditions are rules**, and the ownership rule already gives the board and the rules to the Level director. With goals
  split between the side directors (starting goals) and the Level director (changes), one level's goals would live in
  three places.
- **Reuse:** with all of them on the Level director, a level's whole script is in one place, and side directors are
  reusable across levels.
- **Board change:** goal progress lives on the board, which only `SetBoardState` may write, so changing goals has to be a
  board change for clients to see it.

## What Would Change It

- A mode where a side genuinely owns its goals independent of the level (e.g. a player picking their own objective in
  versus) -- they would be chosen through the level, or move back to that side.
- Lose conditions online (still undecided: [lose-conditions-online](../_questions/lose-conditions-online.md)).

Partly supersedes [2026-10-21-director-hub-and-level-changes](2026-10-21-director-hub-and-level-changes.md) (win / lose
conditions on the side directors). Plan: [phased-levels-plan](../design/phased-levels-plan.md), Stage 1.
