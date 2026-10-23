---
schema: recipes
task: Change what the AI opponent can do, or how it thinks, as a match goes on -- every turn, when either side reaches a goal checkpoint, or on anything else in the level -- including "tells" the player can read.
touches:
  - a Blueprint subclass of UConnectIt_AIDirector (named by the match preset's AI Director Class)
  - the action loadouts / strategies / selectors the director switches between
  - (for tells) a visual manager Blueprint listening to the AI PlayerState's On Loadout Changed
reconciled: 2026-10-21
commit: bbf8daf
---

# Recipe: direct the AI during a match

## Goal

An AI that changes during a match: an alternating loadout every turn, new moves or a smarter strategy after a checkpoint, a
harsher selector when the player is winning, or a "tell" that shows the player what the AI will be able to do next turn. This
is the job of the **AI director**: a Blueprint whose Class Defaults are the AI's setup -- its **loadout**, **strategies**,
**selector**, think times and goals -- and whose events change that setup as the match goes on. It never picks a move itself
(strategies propose, the selector picks).

It is one of three directors (Level, Player, AI -- see [three-directors](../../design/three-directors.md)): all get the same
events, and each changes only its own context. The level's own script is the **Level director** ([[script-a-level]]).

## Prerequisites

- An Adventure match preset whose **AI Director Class** names your director Blueprint ([[set-up-a-custom-ai-opponent]]).
- The extra loadouts (action loadout assets), strategies and selectors you want to switch to.
- For checkpoint reactions: the Level director's **AI** (or **Player**) **Win Conditions** with more than one goal (completing
  one that isn't the last is a checkpoint; see [[add-a-win-condition]]).

## Steps

1. Open your AI director Blueprint (parent `ConnectIt_AIDirector`). Its Class Defaults are the starting setup: `Starting
   Loadout`, `Strategies`, `Selector`, think times. (Its goals are on the Level director.)
2. Add variables for what you'll switch to: `Action Loadout Data Asset` references, **Instanced** strategy / selector variables
   (`ConnectIt_AIStrategy` / `ConnectIt_AISelector`, with Instanced checked) to configure them inline, and `ConnectIt Match
   Condition Asset` references for the goals you react to.
3. Override the events you need. Each says whose it is (**Faction**) and whether that is the AI's own side (**Is Mine**):
   - **On Match Started** -- the board is ready and every side registered: set-up that depends on the level.
   - **On Turn Started (Faction, Is Mine, Turn Number)** -- Is Mine: the AI's turn is starting, before it decides anything (a
     loadout set here applies at once, before it acts). Not mine: the player's turn has begun -- **a loadout set here applies
     at once and replicates**: this is where tells come from.
   - **On Turn Ended (Faction, Is Mine, Turn Number)** -- a turn is over. In the AI's own, a loadout set here applies when its
     turn ends (next turn).
   - **On Win Checkpoint Reached (Faction, Is Mine, Goal, Group Index, Group Name)** -- a side completed a goal that isn't its
     last. Compare **Goal** with your goal asset variables (stable when goals are reordered).
   - **On Lose Condition Met (Faction, Is Mine, Condition)** / **On Match Won (Faction, Is Mine)** -- the match is ending.
   - Anything else: the director has world access, and **Get Level Director / Get Player Director / Get AI Director** reach
     the other directors (ask them; don't change their side yourself).
4. Use the actions:
   - **Set AI Loadout** -- uses restart from the new loadout's values; **during the AI's own turn the change waits until the
     turn ends**, so it can't grant itself extra moves mid-turn. **Get AI Loadout** reads the current one.
   - **Set AI Strategies** (an array) / **Set AI Selector** -- copies of what you pass are used from the AI's **next decision**.
   - The think-time variables (`Think Time Budget Seconds`, `Min Think Seconds`...) can be set directly; read each decision.
   - `Strategies` / `Selector` are the live ones: you can read them, and tweak a strategy's settings (e.g. MinMax depth) --
     read when its next decision starts.
5. Name it in the **match preset** > **Directors > AI Director Class**. Two difficulties of the same behaviour: a child
   Blueprint that only changes the numbers, and a preset naming each.

### React to state, not to visual event tags

Gameplay decisions (what the AI may do next) should follow **committed state**: these events (they come from the turn order
and from the board's committed changes), the board state changing (`On Board State Changed`). The board's **event tags**
(Placed, Scored, Win Checkpoint Reached...) are played one at a time through the visual queue, paced by animations, so they
can arrive seconds after the state changed -- a director keyed on them is at the mercy of animation timing. Event tags are for
the visual side (like the tell manager below).

### The tell flow

1. The AI makes its move; its visuals start playing through the event queue.
2. The player's turn starts: **On Turn Started** (Is Mine = false) fires on every director (server).
3. The AI director decides next turn's kind of move and calls **Set AI Loadout** with a loadout that only allows it. It applies
   at once (it isn't the AI's turn) and replicates.
4. On every machine, the AI's PlayerState fires **On Loadout Changed**. A visual manager Blueprint listening to it shows the
   tell -- **queued behind the AI's move visuals** (put it on the game event queue rather than playing it immediately), so the
   tell doesn't appear before the AI's move has finished animating.
5. The AI's next turn plays with that loadout, so the tell is honest: only those moves are in its `Available Request Types`.

(The visual manager is Blueprint work that doesn't exist yet; `On Loadout Changed` is on `ATurnBasedPlayerState` already.)

### Examples

- **Alternating loadout** (place one turn, swap the next): in On Turn Ended (Is Mine), `Get AI Loadout`; if it is the place
  loadout, Set AI Loadout(swap loadout), else Set AI Loadout(place loadout). Starting Loadout = the place loadout.
- **New behaviour at a checkpoint**: in On Win Checkpoint Reached, if `Goal` is your "Phase 2" goal asset, Set AI Loadout(the
  bigger loadout) and Set AI Strategies([the stronger strategy variable]). Pair it with a big visual change in the level (the
  board event `Win Checkpoint Reached` plays on every machine; or the Level director reacts to the same event).
- **Harder when losing**: in On Turn Started (Is Mine), compare scores (`Get Faction Score (Board State)`); Set AI
  Selector(a First Available variable) when behind, Set AI Selector(a Top N with mistakes) when ahead.
- **Hidden or shown**: whether the player sees the AI's loadout at all is the UI's choice; the loadout is always on the AI's
  PlayerState.

## Verify

- Output Log, filter `ConnectIt_`: `AI director '...'` at match start (the class); `loadout changes to '...' when this turn
  ends` / `seeded action state from loadout '...'` when it switches; `ConnectIt_AIDirector: N strategies from the next
  decision`, `selector changes to ... from the next decision`.
- The AI's next moves follow the new loadout; a move the old loadout allowed and the new one doesn't is no longer offered (it
  isn't in `Available Request Types`).

## Pitfalls

- **Directors run on the server only.** Visual tells must read replicated state (the PlayerState's loadout, board events), not
  the director.
- **Set AI Loadout during the AI's turn is deferred** to the end of that turn; set next turn's loadout in On Turn Ended (Is
  Mine) or in the player's On Turn Started.
- **Swapping loadouts restarts numbered-action uses** from the new loadout's starting values.
- **Strategies that can't play the new loadout's moves** (e.g. MinMax with a swap-only loadout) have nothing to offer: switch
  the strategies too.
- **Goals referred to by index** break when they are reordered; compare the Goal asset.
- **Set AI Strategies replaces the whole list.** To add one, pass the old ones plus the new.
- **Directors hear events in a fixed order**: Level, then Player directors, then the AI. A change the Level director makes in
  reaction to an event is visible to the AI director's reaction to the same event.

## See also

- Flow: [[ai-turn]]
- Design: [three-directors](../../design/three-directors.md), [ai-director-pipeline](../../design/ai-director-pipeline.md),
  [match-menu-options](../../design/match-menu-options.md)
- Recipes: [[set-up-a-custom-ai-opponent]], [[combine-ai-strategies]], [[add-a-win-condition]], [[script-a-level]]
