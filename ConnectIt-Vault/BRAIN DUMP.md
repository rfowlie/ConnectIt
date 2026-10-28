
## World Setting Validation

- Some levels do not require match values (catalog)
- Validation flags these levels it's a bit annoying
- Having match data associated with the level is super convienent for match levels
- Is there an in-between where perhaps based on the set game mode the validation doesn't flag non match levels as missing data


## Map Node Actors

- Instead of creating some base actor for map nodes, can we just use an interface
	- GetMapNodeDataAsset() - get the data associated with the node
- We will want specific functions for mouse over and selection events while in the adventure hub map
- Do we want to use the same setup as grid tiles and pieces? (world subsystem tracks mouse over, mouse click events then the player controller listens to those)
	- If we do it this way then we will want some GridNodeBase actor
- Would be fun to have the actors change scale when hovering over
	- A transform lerp component that takes in a start and end transform with lerp time. Then on tick lerps between start and end
	- https://www.fab.com/listings/ef75203e-5571-4ecf-ab16-09fd1e3df0f6
- We will want some visual lines connecting nodes
	- particles or actors?
	- https://www.youtube.com/watch?v=pzIORuULNfo


## Other Notes

-  Data Assets should always postfix DataAsset in C++ for clarity
- Tile Catalog
	- Tile power struct has redundant gameplay tag parameter.
		- Seems to be needed for later, to what extent I'm not super sure
	- Feels awkward using the gameplay tag for the map key, but then having to specify the effect in the tile power. 
		- Could cause some mismatch problems.
		- We should just enforce in C++ (when a power is made a tag will be made for it in C++ so it will be available and auto enforced)
	- Why are we setting all these values in the first place?
- It would be great if editing a previewed grid tile effects the grid builder tile placement. It's annoying having to flip back and forth to move then check. 
	- Might be easier if we make the grid builder tile visually robust depending on the power tag it possesses
- Made new tile in BP and it is not getting picked up for highlighting or placement through player action but AI can place there fine


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
- 

