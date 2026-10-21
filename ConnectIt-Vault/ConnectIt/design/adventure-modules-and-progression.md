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
