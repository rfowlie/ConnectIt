---
Date: 2026-10-21
status: Partly superseded
superseded by: "[2026-10-21-one-board-change-at-a-time](2026-10-21-one-board-change-at-a-time.md) (Immediately = once the current board change has played out, its visuals included; level changes play one at a time); [2026-10-22-conditions-on-the-level-director](2026-10-22-conditions-on-the-level-director.md) (win / lose conditions move from the side directors to the Level director, changeable mid-match)"
tags:
  - ai
  - architecture
  - designer-tools
  - win-conditions
summary: 'The three directors as built: preset names the classes; GameMode hub sends every director the same events (Level -> Players by slot -> AI); its checkpoint / match-won delegates removed; win / lose conditions on the side directors; level triggers on the Level director; level board / scoring-rule changes with timing'
---
## Decision

How the three directors ([2026-10-20-three-directors-and-match-conditions](2026-10-20-three-directors-and-match-conditions.md))
were built, settling the open points of that design (owner, 2026-10-20, confirmed with the plan
[three-directors-plan](../design/three-directors-plan.md)):

- **The preset names three director classes** (Level, Player -- required, AI); each director's setup is its Blueprint's Class
  Defaults. The AI controller makes its own AI director; the GameMode makes the Level director and one Player director per
  human.
- **The GameMode is the hub**: it sends every director the same events in a fixed order -- **Level, then Player directors by
  faction slot, then AI** -- with "is it mine". Turn events come from the participant manager's turn phases; checkpoint /
  lose / won from the committed board change (state, not the visual queue). Directors find each other through the GameMode;
  only a side's director changes that side's loadout, the Level director changes the board and the rules.
- **The GameMode's `On Win Checkpoint Reached` / `On Match Won` delegates were removed** (nothing used them; the Level director
  replaces level-Blueprint reactions).
- **Win and lose conditions live on the Player / AI directors** and are copied from their classes into the rule set at match
  start (fixed for the match). Lose conditions: any one = loss; wins checked first; AI matches only.
- **Level triggers live on the Level director**, not in the rule set: checked after each committed board change, fire once,
  re-armable, state on the director, not seen by the AI's search.
- **Level changes**: the Level director applies board operations (through the Mediator's move step without the loadout gate)
  and scoring-rule changes, at a timing (`EConnectItChangeTiming`: Immediately = right after the move being carried out, or
  Next Turn Start); chains capped at 64.
- **Migration by checklist**, not code (two presets): the owner rebuilt the inline AI director, player loadout and win lists as
  director Blueprints.

## Why

A fixed order and "is it mine" make reactions predictable without directors needing to know about each other; the ownership
rule avoids two directors fighting over one loadout. Triggers carry state (fired / armed) the stateless rule set can't hold,
and the search can't use them anyway. Deferring level changes until the current move has finished keeps scoring, action
uses and events consistent -- a reaction never changes the board or the rules mid-move.

## What Would Change It

Directors that need to coordinate within one event beyond the fixed order; a level whose triggers need to fire mid-move;
versus play needing lose conditions or a Level director that acts per player.
