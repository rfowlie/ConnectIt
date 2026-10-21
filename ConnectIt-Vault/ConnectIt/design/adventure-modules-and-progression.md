# Adventure modules, levels and progression

Design note (2026-10-20), design/ is schema-less. **Nothing here is decided or built**: it is the owner's organized thinking on how
Adventure mode is structured beyond single levels. It sits beside [adventure-mode](adventure-mode.md) (what Adventure is today and
a pool of prospective levels) and [adventure-levels-swap-heist-and-shift-puzzle](adventure-levels-swap-heist-and-shift-puzzle.md) /
[swap-puzzles](swap-puzzles.md) (two concrete levels).

## Level types

- **Rigid levels** (e.g. "exhaust"): a specific play style and goals, with limited freedom.
- **Checkpoint levels:** the player must hit several conditions or checkpoints in turn, then win (see
  [win-condition-stages](win-condition-stages.md) for how that is built).
- **Concept-introduction levels:** teach a new action, tile power or game style.

## Level template (to create)

A template that records, for each level:

- its purpose
- the concepts it holds
- the actions that can be used

The goal is to see what each level is for and what it needs at a glance.

## First module: Intro / Tutorial (place piece)

1. **1x1 map:** place a piece.
2. **4x1 map, 3 tiles pre-filled:** the player sees what scoring looks like.
3. **Several scoring lines:** place pieces to complete numerous lines.
4. **4x1 map, two pieces together:** the opponent blocks whichever side you play. Then a tile opens up on the other side so you
   can score. This introduces opponent blocking.
5. **Race to one line:** the opponent tries to block you, and you lose if it scores a line first. The player starts with a few
   pieces already placed as an advantage.
6. **Full match:** place-piece only, first to a threshold score, against a weak opponent.
7. **Reward:** clearing it unlocks the "mainframe", which opens other regions and modules. The first module is place piece and can
   be continued, with levels getting harder.

Also to cover: introduce basic multipliers (where in the module is still to be decided).

### Tutorial levels, revised (owner, 2026-10-21)

**Decided: one level per board shape**, for organization -- each a map, a match preset and a small Level director subclass
on a shared tutorial base (lock every tile but the target, show the prompt, move on at the checkpoint). Phased levels (one
level whose board changes) are still wanted for later levels -- see [phased-levels](phased-levels.md).

1. **1x1:** place a piece on the only tile.
2. **4x1, three tiles filled:** fill the last spot to score (introduces scoring).
3. **4x4, two lines almost complete:** fill the one crossing tile for a double-line score.
4. **5x1, two tiles filled:** the player makes three; then an opponent arrives and blocks one end; the player completes
   the other end to score and win (introduces opponents).
5. **4x4, opponent pieces placed:** the opponent makes pre-set moves; the player must block, and blocking correctly also
   scores the player's winning line.

In levels 1-3 (and at each scripted step) only the expected tile may be placed on; the UI says, writes or highlights it.

What this needs, against the current code (design only):

- **A tile lock** -- a per-tile "not placeable" flag checked by the one placement rule
  (`FConnectItBoardOperation_PlacePiece::IsTilePlaceable`, which today checks only active + empty), set by a board
  operation the Level director calls. Not built. Covers placement, hover highlights and the AI in one place.
- **An opponent that joins later** -- works today: an AI with an empty loadout passes its turn; the Level director asks
  the AI director to grant Place Piece.
- **An opponent with pre-set moves** -- works today in Blueprint: an AI strategy subclass whose `BeginDecision` plays the
  next position from a list.
- **Step goals** -- win conditions with checkpoints on the Player director; `On Win Checkpoint Reached` starts the next
  step.
- **Level-to-level flow** (load the next level on a win) -- needed for adventure anyway; `ConnectIt_MatchSetupSubsystem`
  already opens levels.

## Progression and gating

- The first module is mandatory up to a certain point before other levels can be tried.
- Place piece is permanently unlocked and usable everywhere, unless a level prohibits it (e.g. swap-teaching levels).
- **Open:** gate levels (block them until prerequisites are met), or let them be played even if nearly impossible without the
  skills?

## Rewards and skill economy

- **Open:** how to reward players during adventure mode.
- Other skill modules won't necessarily grant the skill permanently on completing their teaching levels. They may grant:
  - **Item uses:** consumed permanently when used.
  - **Permanent numbered uses:** a level has n uses available for that level, growing over time through the module.
- **Shards** (or a theme-appropriate equivalent) are the currency:
  - Levels grant shards for various skills.
  - After a set number of shards, they convert into numbered uses per level (not item uses).
  - Some levels grant a permanent use for the level's duration or phase and block other skills.
- **Side idea:** until all shards are collected, uses are only acquired during levels (pseudo-random) and carry over to other
  levels. That creates a customized economy: players decide when best to spend them.
- **Side idea:** tile powers as a placeable skill, possibly specific to certain levels only (not a permanent or acquirable skill).

## UI and tracking

- A navigable UI showing levels and their states (unlocked, completed, blocked...) and their conditions.
- A system to track which levels are prerequisites for others, and which skills each level requires. This is only needed if
  levels are gated; if every unlocked level is playable, it matters less.

## Next steps

- Draft the level list and the template, starting with the tutorial module.
- Decide gating (block vs. allow), then the reward/shard model.
