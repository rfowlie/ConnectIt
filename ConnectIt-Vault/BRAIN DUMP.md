
I think the shape we should aim for first is create one new action (let's go with the SWAP action that is already being discussed) and give each player 3 uses of the skill at the start of the match. The bonus skill can only be used once per turn and acts as the turn (no placing a piece after). Players cannot gain more uses of the skill after using it 3 times. We will want to make sure that the UI indicates number of uses.

What happened here?
**`ActionTag` ≠ the wire-level `RequestType`, and `PlacePieceAction` already conflates them** (`Request.RequestType = ActionTag;`, verified at `ConnectIt_PlacePieceAction.cpp:127`) — that only works today because someone hand-set PlacePiece's `ActionTag` in the editor to the _native_ tag string (`"ConnectIt.Game.State.PlacePiece"`), silently diverging from the clean `"ConnectIt.Action.PlacePiece"` the ini declares.

Action Stats
- is it possible to genericize state for an action? 
	- Permanent (always available) + turn uses, cooldowns
	- Non-permanent + turn uses + uses available + cooldown
- Or do we just create project specific interfaces to get the expected info and setup?

The real concern here is setting up the player state to track action state and have it not exposed unnecessarily. 

Encountering a state issue when it comes to Actions. The ActionsComponent is tracking the state, but we want the player state to know about the current state of each action so all players can have access to that information. Just having it on the actions component of the local player controller is not ideal.
At the same time we run into the issue of tampering. If a players actions and their associated state are stored on the actions component there is opportunity to tamper. If on the player state, it becomes a read only and imitate setup situation locally.
We have a bit of a circle. The Actions UI (which the player uses to switch between place piece and swap piece) reads what actions (place, swap) are available for this player through the player state (we want it hear so that all clients can have access to this information cleanly?). But then how does the player state know what the state of each action is? The ActionsComponent and each action it holds contain that. So we need to send some server request to update it? This is a bad setup. 
Something to consider is that Actions in theory are transient in nature in that are just logic to perform an action. How that action behaves is dictated by some state (which we can consider a payload). Actions themselves are actually high level concepts, not atomic. They should be directed at accomplishing a super specific task (action). How this happens is totally up to interpretation (internal state machine, etc.). 

What if the actions component doesn't maintain a live set of actions that track their own set and cooldowns etc. What if the actions component simply receives requests to pop, push already preconstructed and state set actions? Would this just make it a glorified state machine (action machine?).

Realistically no matter what turn based game we make, there will always exist an actions menu (think final fantasy, fire emblem) which guards and gates what actions can be performed, and when. We don't actually need the Actions component to manage this. An actions menu UI reading from immutable player action state data will set itself up properly so that inappropriate actions cannot be selected or performed. 
So where does this leave us with the current actions setup? It removes much of the responsibility but perhaps it is okay that we leave it as a state machine like wrapper class that is specific to actions for clarity and ease of use. 

- Currently there are far too many places to mess up gameplay tags for actions
	- Action Loadout
	- Action BP
	- OnBoundInputTriggered
	- UI for each action
- We should be able to read the actions tag instead of having to set it everywhere manually

I've noticed a slight issue with the current logic regarding SWAP piece. If swapping a piece results in a scoring line for either player. The new board state that gets created and replicated has the end position. We lose the intermediary state of the pieces being swapped. This is huge disruption for the visualization side of the game. As it clouds what needs to happen? Or does it? As everything that happens gets recorded into the game event. So as long as that is setup in the correct order for visuals we should be able to interpret each intermediary step around what happened. Updating visuals chronoligcally.


Extract Grid Definition from TileRegistry
Add Grid Definition to LevelConfigDataAsset
Follow same pattern on ConnectIt_BoardRegistrySubsystem (duplicate object from DataAsset, make globally available)
*pass Grid Definition to Registries?*

Rename ConnectIt_BoardRegistrySubsystem -> ConnectItSubsystem