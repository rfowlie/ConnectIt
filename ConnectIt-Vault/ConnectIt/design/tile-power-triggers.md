# Tile power triggers

Design note (2026-10-21), from the owner's idea that some tile powers fire **before** scoring. Nothing is built. The
wider discussion is [tile-powers-in-the-board-pipeline](../_questions/tile-powers-in-the-board-pipeline.md); the original
power sketches are [tile-powers](tile-powers.md).

> **Revised (round 8):** an earlier version had a pre / post timing per power (a two-column chart). The owner dropped
> post-scoring reactions: they are almost indistinguishable from scoring reactions and make the chain much harder to
> reason about. There are now **two phases, pre scoring and scoring**, and a power's phase follows from its trigger.

A power has **one trigger** (what happened to its tile). It can also have a **gate** (a state check such as
"multiplier ≥ 3") and a **cooldown** (turns until it can fire again; 0 = every time it is triggered, at most once per
action).

## Triggers

| Trigger | Phase | Fires when | Examples |
| --- | --- | --- | --- |
| **Tile Enter** | Pre scoring | A piece arrives: place, force place, swap in, moved in by a power | **Repel**: the arriving piece is pushed one tile before lines are checked, so it scores where it lands, not where it was put |
| **Tile Exit** | Pre scoring | A piece leaves: remove, swap out, moved off by a power (not cleared by scoring) | **Vacuum**: the nearest piece behind it is pulled into the empty tile and can complete a line |
| **Piece Change** | Pre scoring | The piece on the tile changes but stays: a capture (new owner) | **Loyal** (sketch): a captured piece flips back once |
| **Tile Change** | Pre scoring | The tile itself changes: multiplier, active/inactive, power given or lost | **Crumble** (level-wide): multiplier reaches 5, the tile deactivates and its piece is removed. **Overflow**: multiplier past 4, each neighbour gets +1. **Fuse**: multiplier changed, the tile turns inactive |
| **Scoring** | Scoring | A line scores through the tile (whether or not its piece is cleared) | **Push** (the owner's chain example), **Poison**, **Overgrowth**, **Siphon** |

Pre-scoring powers fire in the order Enter, Exit, Piece Change, Tile Change, each in tile order. Scoring powers fire in
tile order.

**A shift is neither an exit nor an enter** (review, 2026-10-21): Board Shift moves whole tile data, so a tile's power,
cooldown and piece travel together and no piece changes tile. Shifted positions are only scored.

**A capture is a Piece Change, not an exit and an enter** (owner, 2026-10-22; replaces the round-7 call): the piece
doesn't leave, it only changes faction. Treating it as exit + enter made the visuals tangle (the piece "destroyed", a
chained tile closing and destroying its piece, the piece "placed", the chain reopening) for something that isn't
happening. Piece Change acts for the new owner. It is scored like an arrival (a captured piece can complete a line), as
today.

## How one action resolves

1. **Settle (pre scoring):** Enter / Exit / Change powers fire on what changed, and on what *they* change, until nothing
   more fires.
2. **Score:** find every line made by any piece that arrived since the last scoring pass, then apply them all at once
   (points, +1 multipliers, clear pieces). **No lines: the chain is over.**
3. **Scoring reactions:** Scoring powers on the tiles those lines ran through fire.
4. **Back to 1** with what steps 2 and 3 changed: the multipliers scoring raised (so Crumble fires in that settle) and
   whatever the scoring powers moved or changed (so a pushed piece landing on a Repel tile is repelled before the next
   scoring pass).

So the chain ends at the first scoring pass that finds no lines. Each power on a tile fires at most once per action,
which is what makes every settle step and the chain as a whole finish (with a wave cap as a safety net). The win is
checked once, after the chain. Every step's changes become events in order, so the visuals play the chain as it
happened.

**Reserved, not used:** a post-chain step after the last scoring pass, if post-scoring reactions are ever wanted.

**After the chain:** anything a director does in response -- a checkpoint reaction, a level trigger, a win -- is a new
board request with its own chain, through the Level director's existing path (`ApplyLevelOperation` →
`ProcessLevelOperation`, run once the move's chain has played out -- [one board change at a
time](../_decisions/2026-10-21-one-board-change-at-a-time.md)).

## Decided (owner, 2026-10-21)

- Two phases: pre scoring and scoring. No post-scoring reactions for now.
- Pre-scoring powers settle until nothing more fires, then the wave scores.
- What scoring and the scoring powers change goes back through settle before the next scoring pass; the chain ends at
  the first scoring pass that finds no lines.
- One trigger per power.
- ~~A capture counts as both Tile Exit and Tile Enter.~~ Replaced 2026-10-22: a capture is a **Piece Change** (new
  trigger, pre scoring, acts for the new owner).
- Director, checkpoint and win reactions happen after the chain, as new board requests.
- A power acts for the faction whose change set it off: the scoring faction for Scoring, the arriving piece's owner
  for Enter, the leaving piece's owner for Exit, and the faction of the change that caused it for Change (-1 when
  nobody).
- A shift is neither an Enter nor an Exit (the tile and its piece move together).
- First powers, one per trigger: Push (Scoring), Repel (Enter), Vacuum (Exit), Crumble (Change, level-wide).
- A power's effect is a board operation, the same kind a player action sends, so powers and actions can share them.
