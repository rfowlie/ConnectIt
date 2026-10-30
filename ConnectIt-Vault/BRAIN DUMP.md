
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


## Updating Vault

- use that Microsoft article about the development cycle to better structure the ConnectIt Domain
- Already doing some of the things listed, but could look to better arrange things


## Trace Channel

- since we are building a custom framework, should we just make a grid trace channel and use that going forward?
	- Is it just easier to not do this?


## Random Notes

-  AConnectIt_HubLinkActor is fine for now but feels a bit restrictive. Mostly functions and two params could be turned into a single function call SetConnection(To, From) so an interface feels like a solid possibility.
- When closing project there is a crash that happens, not sure what it is