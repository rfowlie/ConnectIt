
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



