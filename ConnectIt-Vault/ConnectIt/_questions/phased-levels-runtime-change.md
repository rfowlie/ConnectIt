---
created: 2026-10-21
question: "How should a level change its board layout, rules and goals at runtime when a phase ends?"
status: answered
closed-by: "[2026-10-22-phases-as-events-layouts-as-data](../_decisions/2026-10-22-phases-as-events-layouts-as-data.md)"
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

**2026-10-21, round 5 (owner).** A phase is a Level director event (On Phase Changed, sent to every director through
the hub), not a bundle: directors set their own conditions in response, and can change conditions at any time (needs a
"set conditions" on Player / AI directors; today they are fixed at match start). Score and goal progress follow
conditions, not phases. A layout can store a camera position the camera lerps to on the layout event. Layouts that score
on arrival are a design choice (Claude: an optional warning in the layout tool). Open (Claude): a new goal list must
restart goal progress at 0; carried-over score can instantly meet a new score goal -- reset score explicitly, or count
score goals from when they were set.

**2026-10-21, round 6 (owner).** Only the Level director changes conditions mid-match (conditions are rules; matches the
existing ownership rule) -- side directors still author their starting conditions. Carried-over score is a natural
consequence of phases-as-events, handled by designer guidance rather than code (optional runtime warning); a pitfalls
list is in the design note, to become the recipe once built.

**2026-10-21, round 7.** Owner questioned keeping starting conditions on the Player / AI directors; Claude agrees they
should move to the Level director too (one level's goals in one place; side directors become loadout + AI setup).
Plan written: [phased-levels-plan](../design/phased-levels-plan.md) (seven stages, not approved; three points to
confirm at its end).

**2026-10-21, round 8 (owner).** Phases are gameplay tags (open: shared generic tags vs per-level tags). A layout is an
array of cell structs with the tile class and tile data inline, all editable in the asset (Claude: a plain struct, not
an inline UObject -- tile actors can't be instanced in an asset; per-cell look via a variant tag). Plan updated.
Then: shared generic phase tags (`ConnectIt.Phase.1`, `.2`, ..., `.Final`) reused by every level. Only the move of
starting conditions to the Level director remains to confirm before approval.

**2026-10-21: plan approved (owner)**, including starting conditions moving to the Level director. Stays open until
built; closes with the decision notes in the plan's Stage 7.

**2026-10-21, round 9: plan reviewed against the code (Claude), after one board change at a time.**

- **Decided (owner):**
  - **A layout change is one Change Layout operation:** remove then add in one board change, one win check. Two
    separate changes would each be win-checked, and a shrunken in-between board could meet a lose condition (No Legal
    Move).
  - **Changing a side's conditions is a board change:** a `GoalsChanged` operation resets goal progress through
    `SetBoardState` and the normal win check follows. Goals already met win at once.
  - **Begin Phase is a queued board change:** the phase is written onto the board (replicated, so UI can read it) with a
    `PhaseChanged` event; directors hear On Phase Changed from the committed change. The Level director is server-only,
    so the plan's `GetCurrentPhase` couldn't have reached clients.
- **Fixed in the plan:**
  - the level config needs a `DefaultTileClass`;
  - `BeginPhase` is on the Level director only;
  - the tile registry already registers spawned tiles, so only removal is new;
  - pieces are replicated, so they go through the server's piece path;
  - the test directors' goals move too;
  - the layout tool's split (C++ editor helpers, the owner's widget);
  - the checkpoint order: phase, then conditions, then layout.

## Resolution

**Answered 2026-10-22, built** ([phased-levels-plan](../design/phased-levels-plan.md), all seven stages):

- **A phase is an event.** The Level director's Begin Phase (a shared `ConnectIt.Phase.*` tag) is a board change; every
  director hears On Phase Changed and changes what it owns.
- **Layouts are data.** `UConnectIt_BoardLayout` assets are made with a layout tool in a sandbox level.
- **Set Layout is one board change.** A Change Layout is worked out against the board when it is applied, with one win
  check. Tile actors follow on every machine.
- **Only the Level director changes conditions**, and it holds both sides' goals. Set Win / Lose Conditions is a board
  change: goal progress restarts, score carries over.

Decisions: [phases as events, layouts as data](../_decisions/2026-10-22-phases-as-events-layouts-as-data.md),
[conditions on the Level director](../_decisions/2026-10-22-conditions-on-the-level-director.md). Recipe:
[[script-a-phased-level]].

Still open, outside this question:
- range rules for player actions that add / remove tiles;
- an optional "layout already scores" warning in the tool.
