
### Tile Powers

- how do these resolve on the board state since the board state is server side?
- Take Magnet tile power (if a magnet tile is apart of a scoring combination, the nearest pieces in each cardinal direction get pulled towards the tile by n)


### Tile Power Board Change Flow

- Create queue copying all tiles effected by board operation (this should be a struct that holds the tile and what happened to it (piece placed, piece removed, shifted, etc.))
- While loop, pop first in queue, check if tile has power.
	- if post scoring and matches effect type (piece enter/exit) of board operation then add to post scoring array
	- if pre scoring, then pass board to tile power operation (alters board and returns tiles which were touched and why). Add these returned types to end of queue

## Operation Flow

- ConnectIt_BoardRequestMediator::ProcessRequest
	- Some weird stuff happening here need to unpack and have a better understanding
- ConnectIt_BoardRequestMediator::DispatchRequest
	- This gets the current board state and determines asking faction. 
- FConnectItRuleSet::CarryOutOperation
	- Calls Operation::Apply (note that operation was passed in...), then calls FConnectItRuleSet::ResolveBoardChange which then checks scoring
		- This feels like the issue spot. If an operation causes another operation to fire (tile power), then there is no resolving scoring yet
		- As well, carry out opertaion is problematic, we should just be able to call Operation::Apply()
		- Each individual tile power needs to be an operation
		- An operation should be any unique system that effects the board in anyway, not just player actions (player actions are a subset of board operations)
- Ideally we get the current board state, then pass a ref of it to each operation slowly mutating it while passing out the change event that just happened so that we end up with an array of change events, a start and end board state (so that we could potentially recreate all the changes)

## Tile Reaction Rules

- Triggers: Piece Enter, Piece Exit, Tile Changed (this can be broken down into specific aspects of the tile like multiplier, grid position, etc.), Tile Scored On, 
- Timing: Pre Scoring, Post Scoring
	- Scoring itself is a unique phase and tiles should not trigger at the same time
- All pre scoring operations need to resolve before scoring occurs. 
	- If a pre scoring operation triggers another pre scoring operation that needs to resolve prior to scoring
- If scoring triggers a pre scoring operation

Post scoring creates a lot of complications. It is much simpler to track and reason about if we have only 2 timing phases; pre scoring and scoring. We resolve all pre scoring and pre scoring chain reactions. Then we do the scoring. If the scoring causes new reactions then we resolve all those and then once again check scoring. Only when there are no new reactions from scoring do we then conclude board changes from the initial operation. Operations that could result form directors or checkpoints/wins will occur afterwards and be treated like a new board request.


Maybe were thinking of it wrong. Perhaps we have two distinct moments, actions and scoring. When a player performs and action, this can trigger reactions, which themselves can trigger reactions.

## Level Director as Third Player

- to better accomodate certain level changes should we give the level director a artificial turn? This way there is a clean oppertunity for it to enact changes and avoid collisions with player turns? It would likely be a turn after each players turn. A phase that provides a clean entry point for level changes again to avoid collisions.
	- Not used in AI calculations