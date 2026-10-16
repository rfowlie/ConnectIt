---
schema: recipes
task: Change what the AI opponent can do, or how it thinks, as a match goes on -- every turn, or when either side reaches a win checkpoint -- including "tells" the player can read.
touches:
  - a Blueprint subclass of UConnectIt_AIDirector
  - the match preset (AI Director)
  - the action loadouts / strategies the director switches between
reconciled: 2026-10-14
commit: 5262fc9
---

# Recipe: direct the AI during a match

## Goal

An AI that changes during a match: an alternating loadout every turn, new moves or a smarter strategy after a checkpoint, or a
"tell" that shows the player what the AI will be able to do next turn. This is the job of the **AI director**: a Blueprint
object on the **match preset** that reacts to match events and does exactly two things -- give the AI a different
**loadout**, or a different **strategy**. It never picks a move (strategies do).

It lives on the preset because what it reacts to -- the preset's win checkpoints -- is defined there.

## Prerequisites

- An Adventure match preset with an AI profile ([[set-up-a-custom-ai-opponent]]).
- The extra loadouts (action loadout assets) and/or strategies you want to switch to.
- For checkpoint reactions: the preset's **Rules > Player Win Groups / AI Win Groups** with more than one group (completing a
  group that isn't the last is a checkpoint; see [win-condition-stages](../../design/win-condition-stages.md)).

## Steps

1. Content Browser: **Blueprint Class** with parent `ConnectIt_AIDirector` (e.g. `BPAID_SwapEveryOtherTurn`).
2. Add the variables you'll switch between, e.g. two `Action Loadout Data Asset` references, or an **Instanced** strategy
   variable (`ConnectIt_AIStrategy`, with Instanced checked) to configure a strategy inline.
3. Override the events you need:
   - **On AI Turn Started (Turn Number)** -- before the AI decides anything this turn.
   - **On AI Turn Ended (Turn Number)** -- its turn is over. **A loadout set here is the one it plays next turn**, and it is
     already visible (it replicates) during the player's turn: this is where tells come from.
   - **On Win Checkpoint Reached (Faction, Is AI, Group Index, Group Name)** -- either side completed a win group that isn't
     its last. Prefer matching on **Group Name** (stable if groups are reordered).
4. Use the two actions: **Set AI Loadout** (uses restart from the new loadout's values; **during the AI's own turn the change
   waits until the turn ends**, so it can't grant itself extra moves mid-turn) and **Set AI Strategy** (a copy of the strategy
   you pass is used from the AI's next decision). **Get AI Loadout** reads the current one.
5. Open the **match preset** > **AI Director**: pick your Blueprint class and fill in its variables.

### Examples

- **Alternating loadout** (place one turn, swap the next): in On AI Turn Ended, `Get AI Loadout`; if it is the place loadout,
  Set AI Loadout(swap loadout), else Set AI Loadout(place loadout). Start the profile on the place loadout.
- **New behaviour at a checkpoint**: in On Win Checkpoint Reached, if `Group Name` is "Phase 2", Set AI Loadout(the bigger
  loadout) and Set AI Strategy(the stronger strategy variable). Pair it with a big visual change in the level (the board event
  `Win Checkpoint Reached` fires on every machine).
- **Move tell**: decide next turn's kind of move in On AI Turn Ended and Set AI Loadout to one that only allows it. A widget
  reading the AI's PlayerState loadout (or listening for its loadout changing) shows the tell during the player's turn.
- **Hidden or shown**: whether the player sees the AI's loadout at all is the UI's choice; the loadout is always on the AI's
  PlayerState.

## Verify

- Output Log, filter `ConnectIt_AIController`: `AI director '...'` at match start; `loadout changes to '...' when this turn
  ends` / `seeded action state from loadout '...'` when it switches; `strategy changes to ... from the next decision`.
- The AI's next moves follow the new loadout; a move the old loadout allowed and the new one doesn't is no longer offered (it
  isn't in `Available Request Types`).

## Pitfalls

- **The director runs on the server only.** Visual tells must read replicated state (the PlayerState's loadout, board events),
  not the director.
- **Set AI Loadout during the AI's turn is deferred** to the end of that turn; set next turn's loadout in On AI Turn Ended.
- **Swapping loadouts restarts numbered-action uses** from the new loadout's starting values.
- **A strategy that can't play the new loadout's moves** (e.g. MinMax with a swap-only loadout) has nothing to offer: switch the
  strategy too.
- **Win groups referred to by index** break when groups are reordered; use the name.

## See also

- Flow: [[ai-turn]]
- Design: [composable-ai](../../design/composable-ai.md)
- Recipes: [[set-up-a-custom-ai-opponent]], [[combine-ai-strategies]]
