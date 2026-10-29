
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


## Notes

- Tile and Piece registries return GridTileBase so you have to cast to get the ConnectIt_GridTileBase. Should we make connectIt classes for the registrys or should we utilize interfaces for calling desired functions like OnTilePowerTriggered()?
- Visualizer is too rigid. It's trying to capture all possibilities, but in reality it's limiting because it only calls the C++ only On functions for pieces and tiles. This is not ideal, we want visualizers to be artist and designer entry points into creating visual intrigue for the game.
	- Visualizers should handle the important game event syncing by binding to all tags set in a tag container.
	- Or perhaps we bind to tags on the visualizer based on interfaces? If it implements it then we bind in C++ to the correct game event. This way in BP we can ensure we are always getting the correct struct?
	- But it might be messy if we allow for numerous visualizers per level. Might make sense to still only incorporate one per map, could set in same place as registerys (on data asset). In this case we would make all the functions blueprint implementable, one for each board operation that uses the correct struct (just like interface idea)
		- Again because this is heavily artist and designer facing
	- Visualizers will want access to creating and destroying other visual effects potentially...
	- Visualizer MakeReaction() - do not love the random count indicating when to stop or go. Should be like the game event system where we are tracking from the overall event that happened. Otherwise it looks like we can take in numerous events at once and erase by accident?
		- I suppose because Game Event is doing the heavy lifting in terms of gating events this is probably fine
	- See example HandlePlacePiece(), Dispatch() still checks for each payload but then we pass it to the blueprint event so that BP can handle it
		- The visualizer might often call the pieces On functions for events so that that grid actor can do it's own personal visuals, but we still want to ensure we have a way to add additional or alternate visuals for board events

