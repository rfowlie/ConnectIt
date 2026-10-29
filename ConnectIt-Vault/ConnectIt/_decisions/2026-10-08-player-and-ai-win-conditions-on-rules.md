---
Date: 2026-10-08
status: Partly superseded
superseded by: "[2026-10-08-win-condition-groups-and-piece-count](2026-10-08-win-condition-groups-and-piece-count.md) (the single fields became lists; the AI no longer falls back to the player's)"
tags:
  - rules
  - win-conditions
  - adventure
summary: 'The rule set has separate player and AI win conditions (no default for the player''s); the GameMode sets AIFaction after registering the AI. Replaces the Per Faction composite'
---
> Later the same day each field became a **list that must all hold**, and the AI's fallback to the player's condition was
> removed ([decision](2026-10-08-win-condition-groups-and-piece-count.md)). The split into player and AI conditions, `AIFaction`
> set by the GameMode, and the no-default rule stand.

## Decision

The rule set (`FConnectItRuleSet`, the level config's `Rules`) holds **two plain win-condition parameters**, one for the
player and one for the AI, instead of one condition that can be split per faction:

1. **`PlayerWinCondition`** (the old `WinCondition`, renamed with a property redirect in `DefaultEngine.ini` so saved level
   configs keep their value). It is how the player wins, how the AI wins unless told otherwise, and how everyone wins in a
   match with no AI (Online). **It has no default**: a new level config starts empty and must pick one. An empty one is a
   data-validation error on the level config and an error log when a match starts (the match can never be won).
2. **`AIWinCondition`**, optional. Empty = the AI wins the way the player does.
3. **`AIFaction`** (slot index, `INDEX_NONE` = no AI): set by the GameMode right after it registers the AI, because the
   slot is assigned by registration order and is not known earlier. The rule set picks a faction's condition with
   `GetWinConditionFor(Faction)`.
4. The rule set answers the questions the rest of the game asks (`HasFactionWon`, `GetWinningFaction` = lowest slot that has
   won, `GetProgress(Faction)`, `GetTargetScore`, `SetTargetScore`). The AI's search no longer caches a win-condition pointer;
   it asks the rule set. A target-score override from the main menu applies to every score-based condition.

Kept from [the earlier same-day decision](2026-10-08-win-conditions-per-faction-and-no-pieces.md): `HasFactionWon` on the
base condition and the **No Pieces** condition.

## Why

The first attempt (a `Per Faction` condition that wraps other conditions and assigns them to slots) put the "player vs AI"
choice inside a win-condition subclass and keyed it by slot number the designer had to know. Two named fields on the level
config read as what they are ("how the player wins", "how the AI wins") and need no slot numbers. No default for the player
condition because the right one differs per level and a silent Score Threshold hid the choice.

## What Would Change It

A third kind of participant with its own goal (more than a player and an AI): the parameters would become a role-keyed list.
The AI's search evaluating positions by a non-score goal (still score-based, so an AI whose goal is No Pieces wouldn't steer
toward it; the swap puzzles give the AI a score goal).
