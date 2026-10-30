---
schema: recipes
task: Build an Adventure hub map (or a sub-map) -- node actors with map node components, a Start, links, hover scaling and the node panel hook -- that reveals itself from the player's progress.
touches:
  - Source/ConnectIt/Public/Framework/GameMode/ConnectIt_GameMode_AdventureHub.h (the hub maps' game mode)
  - Source/ConnectIt/Public/Adventure/ConnectIt_AdventureHubComponent.h (discovery, the reveal, hover / select, links, focus)
  - Source/ConnectIt/Public/Adventure/ConnectIt_MapNodeComponent.h (NodeData, Start / Exit, state)
  - Source/ConnectIt/Public/Adventure/ConnectIt_HubLinkActor.h (a link's spline; OnLinkBuilt)
  - Source/ConnectIt/Public/Adventure/ConnectIt_AdventureSubsystem.h (StartNode, ReturnToHub, sub-maps)
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/Animation/TransformLerpComponent.h (hover scale)
  - Source/ConnectIt/Public/Adventure/ConnectIt_MapNodeDataAsset.h (Unlocks, Gate, Rewards, Level / SubMap)
reconciled: 2026-10-29
commit: 81ac444
status: 'current'
---

# Recipe: build a hub map

## Goal

A map the player walks through Adventure in. Only the start shows on a fresh save. Winning a node reveals what it
unlocks. Nodes scale up under the cursor, and clicking one focuses it and opens the node panel. Design:
[adventure-hub](../../design/adventure-hub.md); decision: [hub map nodes are components](../../_decisions/2026-10-29-hub-map-nodes-are-components.md).

## Prerequisites

- A **map node data asset** for each node, with `Unlocks` (the graph), `Gate` / `Rewards`, and a `Level`, or a
  `SubMap` for a node that opens another hub map.
- The Adventure subsystem loads the save when the game starts, so nothing needs doing for that.

## Steps

1. **Game mode:** set the map's game mode to `ConnectIt_GameMode_AdventureHub`, or a Blueprint of it. The Blueprint's
   **Adventure Hub** component holds the settings:
   - **Link Actor Class:** a Blueprint of `ConnectIt_HubLinkActor`; empty = a bare spline;
   - **Framing Margin:** the space around the revealed nodes, read through `GetRevealedBounds`.
2. **Nodes:** on each actor that stands for a node:
   - add a **Map Node** component and set its **Node Data**;
   - flag exactly one node **Start**; on a sub-map, flag one node **Exit** (winning it finishes the sub-map);
   - give the actor something that blocks the cursor trace (a mesh with collision), or hover never fires.
   - The hub adds the grid hover component itself, so don't add one.
3. **Hover scale:** add a **Transform Lerp** component to the node actor.
   - Defaults: 0.15 s, 15% bigger, from the actor's root.
   - Call `SetTarget` to scale only the mesh.
4. **Look per state:** bind the map node's **On State Changed** (Available / Gated / Completed; Hidden is handled for
   you) and its focus flag for the "you were here" marker.
5. **Links:** in the link actor Blueprint, implement **On Link Built**. Its spline already runs node to node; lay spline
   meshes or a ribbon along it. Links are spawned at runtime, so don't place any.
6. **Node panel:** bind the hub's **On Node Selected** to open your panel.
   - Its Start button calls the Adventure subsystem's `StartNode`; its Break Seal button calls `BreakSeal`.
   - On the results screen, call `ReturnToHub`.
   - Optionally, override **Focus On Node** to drive your camera. The default moves the first player pawn over the node,
     keeping its height.
7. **Validate:** use Validate Data on the map. It errors on no Start (or two), two Exits, one data asset on two nodes,
   or a sub-map with no Exit. It warns on unlocks that point off this map and on nodes the Start can't reach.

## Verify

- PIE with a fresh save (or `ConnectIt.Adventure.RevealAll 1` for layout work): only the Start shows.
- Win it, then return: its unlocks appear, with links.
- Hover scales a node, and clicking one fires the panel event.
- For a sub-map: start its node, win the Exit, then return. You land in the parent map with the sub-map node completed.
- Tests: `ConnectIt.Adventure.HubDiscovery`, `.RevealWalk`, `.HubValidation`, `.HubInteraction`, `.SubMapComplete`.

## Gotchas

- Hidden nodes have no collision, so they can't be hovered; the hub ignores them anyway.
- The sub-map return path is remembered for the session only, so quitting inside a sub-map loses it.
- The hub exists only on the server or in standalone. That is fine, because Adventure is single-player.
