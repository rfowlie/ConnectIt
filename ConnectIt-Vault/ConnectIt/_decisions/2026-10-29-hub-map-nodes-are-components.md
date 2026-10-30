---
Date: 2026-10-29
status: Active
superseded by:
tags:
  - architecture
  - adventure
summary: 'Adventure hub maps: a map node is a component on any actor, discovered by a component on the hub game mode (pull at BeginPlay, late nodes push); Start / Exit flags on the node; a pure reveal walk from the save sets states and links; hover, scale and link actors are driven by the hub; sub-maps return through their Exit'
---
## Decision

**A hub map is any map using `AConnectIt_GameMode_AdventureHub`; its nodes are `UConnectIt_MapNodeComponent`s on
whatever actors an artist dresses, and the game mode's `UConnectIt_AdventureHubComponent` runs everything.**

- **Nodes are components, not an actor class.** A node holds its `NodeData` (map node data asset), a **Start** flag and
  an **Exit** flag (sub-maps). It starts Hidden, and Hidden hides its owner and the owner's collision.
- **The hub is a component on the hub game mode,** not a world subsystem, so it only exists in hub maps.
  - It keeps node data → node component and actor → node data.
  - **Discovery is pulled** once, at its `BeginPlay`. A node that begins play later **pushes** itself (add-or-update),
    and a node's `EndPlay` removes it.
  - Server-only: a game mode exists only on the server or in standalone. Adventure is single-player, so this is fine.
- **The reveal walk is a pure static function** (`RunRevealWalk`): breadth-first from the Start, through completed
  nodes' `Unlocks`, staying in this map. States come from the progress rules (`GetNodeState`). It returns each revealed
  node's state and the links.
  - It re-runs on the Adventure subsystem's `OnProgressChanged`.
  - A late node takes its state from the stored result.
  - The only things saved are completed ids and broken seals.
- **The hub drives interaction, so nodes don't react on their own:**
  - each node gets a grid hover component (category `ConnectIt.Adventure.MapNode`);
  - the hub plays the node's `UTransformLerpComponent` on hover and unhover;
  - click selects: `FocusOnNode` (Blueprint-native), then `OnNodeSelected` for the node panel;
  - each revealed link becomes a transient `AConnectIt_HubLinkActor` (a spline) spawned between the two node actors.
- **Sub-maps:**
  - starting a sub-map node opens its hub map, and the Adventure subsystem remembers the parent map (for the session,
    not saved);
  - winning that map's **Exit** node and returning completes the sub-map node once (`CompleteNode`, rewards once) and
    opens the parent map.
- **Map validation** on the hub game mode:
  - errors: no Start or more than one; more than one Exit; one data asset on two nodes; a sub-map with no Exit;
  - warnings: an unlock with no node in this map; a node unreachable from the Start.

## Why

- **Components:** artists dress any actor as a node, and nothing forces an actor hierarchy.
- **A game mode component, not a subsystem:** owner's call (2026-10-28). It is clear who owns the hub, its settings
  live on the game mode Blueprint, it never exists outside hub maps, and editor worlds can't register nodes by
  accident.
- **Pull, then push:** discovery doesn't depend on load order, and streamed or spawned nodes still join.
- **A pure walk:** it can be tested without a world, loops can't hang it, and the map's revealed state is never saved,
  so it can't drift from the progress.

## Gotchas kept

1. Discovery runs once per map load. A node that pushes itself twice is added once.
2. The save must be loaded before the hub begins play. The Adventure subsystem loads it in `Initialize`. Without it,
   only the Start is shown and an error is logged.
3. A save id that no node has (a deleted node) is ignored.
4. The sub-map return path lives only in the session. Quitting inside a sub-map loses it; see the plan's follow-ups.

## Links

- Plan: [adventure-hub-map-plan](../design/adventure-hub-map-plan.md); design: [adventure-hub](../design/adventure-hub.md)
  ("The reveal walk").
- Recipe: [build a hub map](../code/recipes/build-a-hub-map.md).
- Related: [adventure saves actions by tag](2026-10-29-adventure-saves-actions-by-tag.md),
  [hover through a component](2026-10-28-hover-through-a-component.md).
