
## ToDO

- Engine project, need to port it to plugin
- Camera setup seems decent, could convert it or use but is not using enhanced input system
- PC monolithic to support all map modes in one, bad design, need to look through and break apart as each mode could be valuable
- Lots of amazing materials for icons and effects already animated in material
- Path marks are fucked, they made blueprints for each different material rather than just exposing the material slot for generic behaviour
- Noticing that in project, subclasses of the core path tracer BP are just data BPs, we could simply create a data asset instead and slot it into the BP path tracer to use as flexible defaults and generic use
- Core actor BP path tracer is a BP not derived from a C++ class, we would need to fix this. 
	- Need to examine and see if we can break apart different functionality so we can create a variety of base classes for specific situations
	- Everything is BP which is inefficient. Converting to C++ would give huge performance boost
- Grid system here is built on the plane idea and not flexible like ours. The grid aspects work great in there demo but we will have to break it apart to make it work with our system and be generic
	- Again all in BP, using A* algo there is a lot of potential for performance improvement
	- Seems like most of their grid stuff (grid, blocker, etc.) won't be of use to use. Mainly we will want to see how to get the path tracers working with our grid setup.
- Should work in phases, the first is to extract the path tracer as it seems like it can function on it's own. Then look to see if there is anything valuable in the grid section to export for later use with other projects
- 