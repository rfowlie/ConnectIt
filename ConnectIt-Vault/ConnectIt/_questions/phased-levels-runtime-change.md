---
created: 2026-10-21
question: "How should a level change its board layout, rules and goals at runtime when a phase ends?"
status: open
closed-by:
tags:
  - question
---

# How should a level change its board layout, rules and goals at runtime when a phase ends?

## Why it matters

The owner wants phased levels: an event (checkpoint, trigger) moves the level to its next phase and anything can change
-- layout, pieces, rules, goals, loadouts -- with a loss in any phase restarting the level. Today the board layout and the
win / lose conditions are fixed once the match starts, so phased levels can't be built yet.

## What would answer it

Owner calls on: how a layout comes into existence (masked grid, layout-as-data, or streamed sub-levels); what a phase
holds; how score and goal progress carry between phases. Then a plan.

## Current thinking

**2026-10-21 (owner + Claude).** Tutorial is separate levels (decided); phased levels wanted for later. Mapped what is
changeable at runtime today and what isn't (layout, conditions, score / progress). Proposed: a phase is data owned by
the Level director, applied with Go To Phase after the chain that triggered it, as one board change whose first event
carries the whole new layout. Three layout options compared; Claude leans to layout-as-data. Full write-up:
[phased-levels](../design/phased-levels.md).

**2026-10-21, round 2 (owner).** Layout as data, authored with a grid layout tool in a sandbox level; opt-in per level
(placed tiles stay the default; a phased level's Level director requests the change). A layout change = Remove Tiles
then Add Tiles, two board operations also usable as player actions. The board state is the source of truth; tile actors
reconcile to it, so re-registering is not an issue. Claude: a tile's gameplay data is already data
(`FConnectItTileInitialData`), so a layout asset is cheap; what it loses vs sub-levels is per-actor visuals (a layout
entry can name a tile class or variant); sub-levels would break the single source of truth during a change. Open: tiles
present in both layouts (keep or reset), whether pieces on added tiles score, range rules for player add/remove.

**2026-10-21, round 3 (owner).** Tiles in both layouts keep their current piece and multiplier; only appearing and
disappearing tiles change. Set Layout = Remove Tiles + Add Tiles, nothing else. Still open: whether pieces on added tiles
score; range rules for player add/remove.

**2026-10-21, round 4 (owner).** Pieces on added tiles are ordinary arrivals: Add Tiles runs the normal chain (powers,
then scoring), so they set off Enter powers and score if they complete lines; the operation decides whether arriving
pieces survive scoring, as for every operation. Still open: range rules for player add/remove (can wait for that action's
own design).

## Resolution

