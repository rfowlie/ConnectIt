
### Tile Powers

- how do these resolve on the board state since the board state is server side?
- Take Magnet tile power (if a magnet tile is apart of a scoring combination, the nearest pieces in each cardinal direction get pulled towards the tile by n)

### Adventure Levels

- Lets devise a list of possible levels
- There will rigid ideas such as 'exhaust' which are specific and limit play style and goals
- There will be checkpoint types of levels where players will need to achieve various conditions/checkpoints to then win.
- There will need to be levels that introduce new concepts (actions, tile powers, game styles)
- Should create a template for adventure levels which help us to understand what the levels purpose is, the concepts it holds, actions that can be used, etc.
- Let's try to create the first module of adventure mode, the 'Intro/Tutorial' module
	- Place piece on a 1x1 map
	- Place piece on 4x1 map where 3 of the tiles are already filled in by pieces so the player sees what scoring looks like
	- Place pieces to complete numerous scoring lines
	- Place piece, starts with two pieces together on a 4x1 map, whatever side you play on the opponent blocks other side, but then a tile opens up on the other side so that you can make the score. Idea is to introduce opponent blocking
	- Place piece to make one line, but now opponent tries to block you. You lose if the opponent scores a line first. Player should have some sort of advantage like a few pieces already placed on the board.
	- Play against opponent using only place piece first to threshold score, opponent is weak
	- This then unlocks the mainframe which will grant you access to other regions/modules. The first module is the place piece module and can be continued, the levels get harder and harder.
- How do we reward players during adventure mode
- How do we gate certain levels? Should we prevent them from being played if prerequisites are not met? Or should we allow it to be played, it would just be almost impossible with minimal skills or knowledge.
- Introduce basic multipliers
- Side idea, what if players need to collect shards or something which translate to permanent uses of skills. But until they collect all the shards they can only acquire uses during levels (pseudo random). Uses carry over into other levels which then can create a minmax economy for players to decide when best to use the numbered uses of skills.
- Side idea, placing tile powers as a skill could be interesting (could just be specific to certain adventure levels. Not a permanent skill or acquirable skill)
- First module is place piece and players are forced to complete up to a certain point before being able to try other levels. Place piece is permanently unlocked as a skill and can be used at all times unless a level prohibits it (for example swap teaching levels). Other modules that introduce skills will not necessarily give the skill permanently upon completion of the teaching levels. They might grant item uses (using it loses a use permanently) or permanent numbered uses (when available in a level will have n uses for that level) over time the further you get into the module. These should be in the form of shards or something similar depending on artistic theme.  Other adventure levels might grant shards for various skills upon completion. After a certain number of shards, the shards are transformed into numbered uses per level (not item uses). Certain levels might grant permanent use for the duration or phase of the level and block other skills depending.
- Will need a UI that is navigable that shows levels and their conditions (unlocked/completed/blocked, etc.). Will likely want to create some system to track what levels are prerequisites for others and also what skills are required to play the level (unless we go with all unlocked levels are playable but might be impossible for players given the skills they have)
# Adventure levels: organized notes

## Level types

- **Rigid levels** (e.g. "exhaust"): specific play style and goals, limited freedom.
- **Checkpoint levels:** the player must hit several conditions or checkpoints in turn, then win.
- **Concept-introduction levels:** teach a new action, tile power or game style.

## Level template (to create)

- A template that records, for each level:
    - its purpose
    - the concepts it holds
    - the actions that can be used
- The goal is to see what each level is for and what it needs at a glance.

## First module: Intro / Tutorial (place piece)

1. **1x1 map:** place a piece.
2. **4x1 map, 3 tiles pre-filled:** the player sees what scoring looks like.
3. **Several scoring lines:** place pieces to complete numerous lines.
4. **4x1 map, two pieces together:** the opponent blocks whichever side you play. Then a tile opens up on the other side so you can score. This introduces opponent blocking.
5. **Race to one line:** the opponent tries to block you, and you lose if it scores a line first. The player starts with a few pieces already placed as an advantage.
6. **Full match:** place-piece only, first to a threshold score, against a weak opponent.
7. **Reward:** clearing it unlocks the "mainframe", which opens other regions and modules. The first module is place piece and can be continued, with levels getting harder.

- **Also to cover:** introduce basic multipliers (placement in the module still to be decided).

## Progression and gating

- The first module is mandatory up to a certain point before other levels can be tried.
- Place piece is permanently unlocked and usable everywhere, unless a level prohibits it (e.g. swap-teaching levels).
- **Open question:** gate levels (block them until prerequisites are met), or let them be played even if nearly impossible without the skills?

## Rewards and skill economy

- **Open question:** how to reward players during adventure mode.
- Other skill modules won't necessarily grant the skill permanently on completing their teaching levels. They may grant:
    - **Item uses:** consumed permanently when used.
    - **Permanent numbered uses:** a level has n uses available for that level, growing over time through the module.
- **Shards** (or a theme-appropriate equivalent) are the currency:
    - Levels grant shards for various skills.
    - After a set number of shards, they convert into numbered uses per level (not item uses).
    - Some levels grant a permanent use for the level's duration or phase and block other skills.
- **Side idea:** until all shards are collected, uses are only acquired during levels (pseudo-random) and carry over to other levels. That creates a customized economy: players decide when best to spend them.
- **Side idea:** tile powers as a placeable skill, possibly specific to certain levels only (not a permanent or acquirable skill).

## UI and tracking

- A navigable UI showing levels and their states (unlocked, completed, blocked...) and their conditions.
- A system to track which levels are prerequisites for others, and which skills each level requires. This is only needed if levels are gated; if every unlocked level is playable, it matters less.

## Next steps

- Draft the level list and the template, starting with the tutorial module.
- Decide gating (block vs. allow), then the reward/shard model.