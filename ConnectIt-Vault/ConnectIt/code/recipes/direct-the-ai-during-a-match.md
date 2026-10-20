---
schema: recipes
task: Change what the AI opponent can do, or how it thinks, as a match goes on -- every turn, when either side reaches a goal checkpoint, or on anything else in the level -- including "tells" the player can read.
touches:
  - a Blueprint subclass of UConnectIt_AIDirector
  - the match preset (AI Director)
  - the action loadouts / strategies / selectors the director switches between
  - (for tells) a visual manager Blueprint listening to the AI PlayerState's On Loadout Changed
reconciled: 2026-10-19
commit: fd369a4
---

# Recipe: direct the AI during a match

## Goal

An AI that changes during a match: an alternating loadout every turn, new moves or a smarter strategy after a checkpoint, a
harsher selector when the player is winning, or a "tell" that shows the player what the AI will be able to do next turn. This
is the job of the **AI director**: the object on the **match preset** that owns the AI's setup -- its **loadout**, its
**strategies** and its **selector** -- and changes them on match events. It never picks a move itself (strategies propose,
the selector picks).

One director per adventure level (its script for the AI), or one per way of playing against the AI (a "Classic" director
whose difficulty is its settings).

## Prerequisites

- An Adventure match preset with an AI director ([[set-up-a-custom-ai-opponent]]).
- The extra loadouts (action loadout assets), strategies and selectors you want to switch to.
- For checkpoint reactions: the preset's **Rules > Player Win Conditions / AI Win Conditions** with more than one goal
  (completing one that isn't the last is a checkpoint; see [[add-a-win-condition]]).

## Steps

1. Content Browser: **Blueprint Class** with parent `ConnectIt_AIDirector` (e.g. `BPAID_SwapEveryOtherTurn`). Its Class
   Defaults are the AI's starting setup: `Starting Loadout`, `Strategies`, `Selector`, think times.
2. Add variables for what you'll switch to: `Action Loadout Data Asset` references, **Instanced** strategy / selector variables
   (`ConnectIt_AIStrategy` / `ConnectIt_AISelector`, with Instanced checked) to configure them inline, and `ConnectIt Win
   Condition Asset` references for the goals you react to.
3. Override the events you need:
   - **On Match Started** -- the AI has joined, before its first turn: set-up that depends on the level.
   - **On AI Turn Started (Turn Number)** -- before the AI decides anything this turn.
   - **On AI Turn Ended (Turn Number)** -- its turn is over.
   - **On Opponent Turn Started (Turn Number)** -- the player's turn has begun. **A loadout set here applies at once and
     replicates**: this is where tells come from.
   - **On Win Checkpoint Reached (Faction, Is AI, Goal, Group Index, Group Name)** -- either side completed a goal that isn't
     its last. Compare **Goal** with your goal asset variables (stable when goals are reordered).
   - Anything else: the director has world access (Get Game Mode, the board state component...). See "React to state" below.
4. Use the actions:
   - **Set AI Loadout** -- uses restart from the new loadout's values; **during the AI's own turn the change waits until the turn
     ends**, so it can't grant itself extra moves mid-turn. **Get AI Loadout** reads the current one.
   - **Set AI Strategies** (an array) / **Set AI Selector** -- copies of what you pass are used from the AI's **next decision**.
   - The think-time variables (`Think Time Budget Seconds`, `Min Think Seconds`...) can be set directly; read each decision.
   - `Strategies` / `Selector` are the live ones: you can read them, and tweak a strategy's settings (e.g. MinMax depth) --
     read when its next decision starts.
5. Open the **match preset** > **AI Director**: pick your Blueprint class and fill in its variables.

### React to state, not to visual event tags

Gameplay decisions (what the AI may do next) should follow **committed state**: these events, the board state changing
(`On Board State Changed`), turn notifications. The board's **event tags** (Placed, Scored, Win Checkpoint Reached...) are
played one at a time through the visual queue, paced by animations, so they can arrive seconds after the state changed -- a
director keyed on them is at the mercy of animation timing. Event tags are for the visual side (like the tell manager below).

### The tell flow

1. The AI makes its move; its visuals start playing through the event queue.
2. The player's turn starts: **On Opponent Turn Started** fires on the director (server).
3. The director decides next turn's kind of move and calls **Set AI Loadout** with a loadout that only allows it. It applies at
   once (it isn't the AI's turn) and replicates.
4. On every machine, the AI's PlayerState fires **On Loadout Changed**. A visual manager Blueprint listening to it shows the
   tell -- **queued behind the AI's move visuals** (put it on the game event queue rather than playing it immediately), so the
   tell doesn't appear before the AI's move has finished animating.
5. The AI's next turn plays with that loadout, so the tell is honest: only those moves are in its `Available Request Types`.

(The visual manager is Blueprint work that doesn't exist yet; `On Loadout Changed` is on `ATurnBasedPlayerState` already.)

### Examples

- **Alternating loadout** (place one turn, swap the next): in On AI Turn Ended, `Get AI Loadout`; if it is the place loadout,
  Set AI Loadout(swap loadout), else Set AI Loadout(place loadout). Starting Loadout = the place loadout.
- **New behaviour at a checkpoint**: in On Win Checkpoint Reached, if `Goal` is your "Phase 2" goal asset, Set AI Loadout(the
  bigger loadout) and Set AI Strategies([the stronger strategy variable]). Pair it with a big visual change in the level (the
  board event `Win Checkpoint Reached` plays on every machine).
- **Harder when losing**: in On AI Turn Started, compare scores (`Get Faction Score (Board State)`); Set AI Selector(a First
  Available variable) when behind, Set AI Selector(a Top N with mistakes) when ahead.
- **Hidden or shown**: whether the player sees the AI's loadout at all is the UI's choice; the loadout is always on the AI's
  PlayerState.

## Verify

- Output Log, filter `ConnectIt_AIController`: `AI director '...'` at match start; `loadout changes to '...' when this turn
  ends` / `seeded action state from loadout '...'` when it switches. Filter `ConnectIt_AIDirector`: `N strategies from the next
  decision`, `selector changes to ... from the next decision`.
- The AI's next moves follow the new loadout; a move the old loadout allowed and the new one doesn't is no longer offered (it
  isn't in `Available Request Types`).

## Pitfalls

- **The director runs on the server only.** Visual tells must read replicated state (the PlayerState's loadout, board events),
  not the director.
- **Set AI Loadout during the AI's turn is deferred** to the end of that turn; set next turn's loadout in On AI Turn Ended or On
  Opponent Turn Started.
- **Swapping loadouts restarts numbered-action uses** from the new loadout's starting values.
- **Strategies that can't play the new loadout's moves** (e.g. MinMax with a swap-only loadout) have nothing to offer: switch
  the strategies too.
- **Goals referred to by index** break when they are reordered; compare the Goal asset.
- **Set AI Strategies replaces the whole list.** To add one, pass the old ones plus the new.

## See also

- Flow: [[ai-turn]]
- Design: [ai-director-pipeline](../../design/ai-director-pipeline.md), [match-menu-options](../../design/match-menu-options.md)
- Recipes: [[set-up-a-custom-ai-opponent]], [[combine-ai-strategies]], [[add-a-win-condition]]
