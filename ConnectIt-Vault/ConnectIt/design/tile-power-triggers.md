# Tile power triggers

Design chart (2026-10-21), from the owner's idea that some tile powers fire **before** scoring. Nothing is built. The
wider discussion is [tile-powers-in-the-board-pipeline](../_questions/tile-powers-in-the-board-pipeline.md); the original
power sketches are [tile-powers](tile-powers.md).

A power is set off by **one trigger** (what happened to its tile) at **one timing** (before or after this wave's
scoring) -- one cell of the chart. Any power can also have a **gate** (a state check such as "multiplier ≥ 3") and a
**cooldown** (turns until it can fire again; 0 = every time it is triggered, at most once per action).

## The chart

| Trigger | Fires when | Pre scoring: changes what scores this wave | Post scoring: acts on the board after lines clear |
| --- | --- | --- | --- |
| **Tile Enter** | A piece arrives: place, force place, swap in, shift in, moved in by a power, **captured** | **Repel**: the arriving piece is pushed one tile before lines are checked, so it scores where it lands, not where it was put | **Push**: after its line clears, a random neighbouring piece is pushed one tile away |
| **Tile Exit** | A piece leaves: remove, swap out, shift off, moved off by a power, **captured** (not cleared by scoring) | **Vacuum**: the nearest piece behind it is pulled into the empty tile and can complete a line this wave | **Scar**: the tile's multiplier rises by 1 once the piece has gone |
| **Scoring** | A line scores through the tile | -- (scoring powers are post only) | **Poison**, **Overgrowth**, **Siphon**; the Push from the owner's chain example |
| **Tile Change** | The tile itself changes: multiplier, active/inactive, power given or lost | Rare: only sees changes made by the action (Toggle, Destroy Multiplier) or other pre-scoring powers. **Fuse**: if its multiplier changes, it turns inactive, breaking lines through it | **Crumble** (level-wide): multiplier reaches 5, the tile deactivates and its piece is removed. **Overflow**: multiplier past 4, each neighbour gets +1 |

Rows are in firing order within a step. Pre vs post in one sentence: a **pre** power changes what this wave scores; a
**post** power reacts to the board after this wave's lines have cleared.

**A capture is both an exit and an enter** (the old owner's piece leaves, the new owner's arrives), the same way a swap
or shift reads on each tile it touches. No separate trigger. Since a tile holds one power with one trigger, a capture
fires it once whichever of the two it is. Quirk: an Exit power whose effect needs an empty tile (Vacuum) finds the
tile still occupied after a capture and does nothing; if that matters for a power, it can be given a gate later.

## One wave, with both timings

1. **Changes in**: the action's changes (wave 0), or what the previous wave's post powers changed.
2. **Settle (pre)**: pre-scoring powers fire on what changed -- Enter, then Exit, then Change, each in tile order. What
   they change can set off more pre-scoring powers; this repeats until nothing more fires.
3. **Score**: find every line made by any piece that arrived since the last scoring, then apply them all at once
   (points, +1 multipliers, clear pieces).
4. **React (post)**: post-scoring powers fire -- Enter, then Exit, then Scoring, then Change, each in tile order. Change
   goes last so a multiplier-threshold power (Crumble) sees what the scoring powers did.
5. What step 4 changed is the next wave's step 1. The chain stops when step 4 changes nothing (or at the wave cap).

Each power on a tile fires at most once per action, across settle and react, which is what makes the settle step and
the chain finish. The win is checked once, after the last wave. Every step's changes become events in order, so the
visuals play the chain as it happened.

## Decided (owner, 2026-10-21)

- Pre-scoring powers settle until nothing more fires, then the wave scores.
- No pre timing for Scoring powers.
- One trigger and one timing per power.
- Firing order within a step: Enter, Exit, Scoring, Change.
- A capture counts as both Tile Exit and Tile Enter (owner's suggestion; Claude agrees, with the Vacuum quirk above).
