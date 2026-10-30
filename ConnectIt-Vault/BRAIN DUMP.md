
## Free Plugin Integration

- Lots of great free plugins that could be helpful or that we should decide to incorporate into the ChimearGameFramework (unreal game mechanics)
- https://www.fab.com/listings/34f435fb-b607-4a3b-95ba-3d16c028b169
- https://www.fab.com/listings/510af476-43e7-41ff-a7c5-e46112864f9e
	- Get Claude to help you unpack this
- https://www.fab.com/listings/8a7105d6-6f98-4951-8ce2-9ce600c8bd78
- https://www.fab.com/listings/06668d29-347d-4a71-b0bd-9b4204b10d01
- https://www.fab.com/listings/620352e4-6f27-4675-a809-99c16784fa17
- https://www.fab.com/listings/8dac53ee-251c-4057-9d85-eca34d898ca5
- We need to create some sort of catalog somewhere that explains what plugins are in the framework, what they do, can be used for so that we do not have a bunch of things for solving the same problems. We should try to use a single plugin or set of code for accomplishing specific tasks


## Updating Outdated BP

- find and replace deprecated pathways
- CI_GridTileBase listens to it's own mouse hover instead of utilizing the OnTileHovered virtual function
- Visualizer needs to be overhauled as we just used a random actor to connect things while we figured 

## Visualizer Framework

- the new setup is to have one visualizer per level that then becomes responsible for handling all misc. visualizations for all possible board reactions and operations. 
- This feels a bit monolithic and daunting.
- It might be easier down the line to add flexibility by creating single visualizers. The concept would be that you have a base visualizer (the C++ default for example). Then a list of single visualizers that override one board operation each. This way we can more easily swap out behaviors. 
	- Should be a map with Operation tag and Visualizer Actor (single)

## Hover Framework
- Right now actions tell tiles and pieces if they are being hovered over and generally expect them to handle what that visually looks like
- Do we want to utilize what we did in the other project where there is a cursor actor that moves from tile to tile instead of tiles adjusting their visuals?
	- Easily accomplished using actor pooling
- Can we devise a way to incorporate both of these for maximum flexibility?
	- As well this should remove complexity as each tile does not need to know how to visually handle each possible situation if it does not want. Instead it can rely on actions to change the cursor look
- Example hovering over a lava tile, we grab the generic hover actor and move it to that tile, but then the tile is notified being hovered over and causes some sort of lava burst effect at the same time
- Things to consider
	- Tile has hover effects
	- Cursors for different tiles potentially
	- Cursors for different player actions
		- Swap for example might want a different cursor for self tile selection and then for opponent tile selection
	- How can we reconcile an action wanting a certain cursor and the tile actor wanting a certain cursor? Do actions override tiles?


## Hover Continued

• Tricky to consolidate who has control in what context. Generally an action will want to control cursor. For swap there will be marker cursors and a general hover cursor. Potentially a specific one the action wants. But then what if the level or even the tile want to control their look for hover? It will be easy to have them do some visual update when hovered as that is in their control. But what if we want a specific cursor for a tile during an action? In the same vein how does a tile understand what visual update to do? Will we need specific tags for each action? Having actions use the same cursors for their parts is easiest overall. The cursor classes can live in the action and don’t have to worry about overrides.  
• What are the tags that would be needed to represent parts of actions?  
• Place.hovered  
• Place.selected  
• Swap.hovered  
• Swap.playerSelection  
• Swap.opponentSelection  
• Should tiles just never be responsible for cursors or interact with cursors?  
• Any cursor selection lives in the action as cursors are related to input? Cursor selection logic can live in third party classes but then used by action? In general actions will tell tiles that they are (hovered, selected, valid, invalid, etc.). If there is a situation where we want the tile to control the cursor selection (incorporate it into the visual update, perhaps it’s easier this way sometimes), we could have a no cursor option for the action cursor selection and then when the action calls tile visual update and the tile handles it  
• In general tiles probably don’t want to be responsible for cursor selection or behaviour. That should generally be managed in the cursor selection logic within actions.  
• Cursor selection logic could potentially be setup per level, there should be a default cursor selection and then each level could override it if desired.  
• Cursor selection logic should work more on a single action basis. Swap action would define a cursor selection object


## Actor Pool Subsystem

Actor pool subsystem should incorporate some sort of asynchronous actor spawn if possible and as an option when we aren’t worried about the speed of creation. Otherwise we should have a gate that only allows n actors to be spawned per tick as this is one of the clearest ways that a game gets poor performance  