---
type: plan
status: in-progress
approved: true
approved_on: 2026-10-28
completed_on:
started: 2026-10-29             # YYYY-MM-DD, when the first stage was built (blank until then)
stages: 6              # how many stages the plan has
current_stage: "4: interaction, framing and links (Stage 3 built 2026-10-29)"   # the first stage not yet built ("complete" / "not started")
playtested: false
design: "[adventure-hub](adventure-hub.md)"
question:
decision:
requires:
  - "[adventure-hub-plan](adventure-hub-plan.md)"
  - "[hover-component-plan](hover-component-plan.md)"
---

# Plan: the adventure hub map (node components, the reveal walk, interaction)

Design: [adventure-hub](adventure-hub.md) -- "The reveal walk" and its decisions (2026-10-28). This plan **replaces
Stage 4 ("the hub level") of the [adventure-hub-plan](adventure-hub-plan.md)** and takes the hub-map checks from its
Stage 5. Written 2026-10-28 in a design-only session; **approved by the owner 2026-10-28**. Not built -- building happens in its own session.

> **Prerequisites:** the adventure hub plan's **Stages 2-3** (the progress / save subsystem and returning from a level)
> and the **hover component plan** (map nodes hover through it). The **top-down camera plan** is used for framing and
> focus when it exists; until then Stage 4 uses a simple fallback.

## Context

Settled with the owner:

- **A map node is a component on any actor** (`UConnectIt_MapNodeComponent`), not an actor class.
- **The hub system is a component on the hub game mode** (`UConnectIt_AdventureHubComponent` on
  `AConnectIt_GameMode_AdventureHub`) -- revised 2026-10-28 from a world subsystem, so it only exists in hub maps. It
  keeps **actor → data asset** and **data asset → actor** maps.
- **Discovery is pulled:** when the hub component begins play it finds every map node component in the world, then runs
  the walk -- no dependency on load order. A node spawned later **pushes** itself to the hub component.
- **Server-only accepted:** a game mode exists only on the server / in standalone; Adventure is single-player, so the
  hub always has one. A networked hub would move the system client-side (a GameState component or a local-player
  subsystem).
- **The start node is a Start flag on the component** (an **Exit** flag likewise for sub-maps).
- **The walk** enables nodes from the **active player file's Adventure progress** (game instance): breadth-first from
  the start, through completed nodes' `Unlocks`; states completed / available / gated; nothing about the map is saved
  beyond completed ids and broken seals.
- **Validation:** a `ConnectIt_GameMode_AdventureHub` map with no Start flag → error.

### Gotchas (must be handled -- each has a test or a check below)

1. ~~Editor-world registration~~ -- gone: editor worlds have no game mode, and discovery runs only when the hub
   component begins play.
2. **Discovery runs once** per hub map load; a late node that pushes itself twice is added-or-updated, and a destroyed
   node removes itself (`OnUnregister` / `EndPlay`).
3. **Late arrivals** (a node spawned at runtime, a streamed sub-level) **push** themselves to the hub component (found
   through the game mode) and get their state from the stored walk.
4. **The player file's progress must be loaded** by the game instance **before** the hub component begins play, or the
   walk sees an empty save and reveals only the start.
5. A **deleted node id** in a save only matters in testing (ignored by the walk); a save override / renew tool comes
   later.

Each stage builds and passes the tests before the next; the editor must be closed for builds; the owner commits between
stages.

## Revision (owner, 2026-10-28, applied): a component on the hub game mode, not a world subsystem

Owner's reason: hub maps have their own game mode, so the hub system should only exist there. Trade-offs weighed before
applying -- for: only in hub maps, clear ownership, settings on the game mode Blueprint, no editor-world registration;
cost: server-only (fine for a single-player hub), and discovery must be pulled at `BeginPlay` rather than relying on
`OnRegister` reaching the game mode. The stages below are written for it.

## Stage 1: the hub game mode, its component, and the node component

- **`UConnectIt_MapNodeComponent`** (`Public/Adventure/`): `NodeData` (`UConnectIt_MapNodeDataAsset`), **`bIsStart`**,
  **`bIsExit`** (editable on instances); **`State`** (Hidden / Available / Gated / Completed, plus a "focus" flag) with
  `OnStateChanged` for Blueprint; on state Hidden it hides its owner (and its collision), otherwise shows it.
- **`AConnectIt_GameMode_AdventureHub`** -- the hub maps' game mode (no match; owns the hub listener of Stage 4 and the
  return-from-level focus); creates the hub component.
- **`UConnectIt_AdventureHubComponent`** (on the hub game mode): `NodeToActor` and `ActorToNode` maps (by data asset);
  the start and exit components; `GetActorForNode`, `GetNodeForActor`; a static helper to find it from any world
  context (`GetAuthGameMode` → the component; null outside hub maps).
- **Discovery (pull)** on the hub component's `BeginPlay`: iterate the world's map node components, fill the maps, find
  Start / Exit; a second component with the same data asset → logged error, the first kept; then run the walk
  (Stage 2).
- **Late spawns (push):** a map node component whose `BeginPlay` comes after discovery calls `AddNode` on the hub
  component (add-or-update); `EndPlay` calls `RemoveNode`.
- Tests: discovery finds every node; a duplicate data asset rejected; a late node added once even if it pushes twice;
  removal clears both maps (with a test world where needed).
- **As built (2026-10-29; 78/78 tests):**
  - `UConnectIt_MapNodeComponent`: `NodeData`, `bIsStart`, `bIsExit`; `State` reuses the adventure plan's
    `EConnectItNodeState` (Hidden / Available / Gated / Completed) plus `SetFocused`; `OnStateChanged` (node).
    Hidden hides its owner and its collision; it starts Hidden. Its `BeginPlay` pushes itself to the hub only if the hub
    has already discovered (add-or-update); `EndPlay` removes it.
  - `UConnectIt_AdventureHubComponent`: `NodeToComponent` (data asset -> node component, so -> actor) and
    `ActorToNode`; `DiscoverNodes` (once, on `BeginPlay`; the world's node components); `AddNode` rejects a node with
    no data and a second node naming the same data asset (error, first kept); a second Start / Exit is an error, first
    kept; `GetActorForNode`, `GetNodeForActor`, `GetNodeComponent`, `GetStartNode`, `GetExitNode`; static
    `GetAdventureHub` (the auth game mode's component).
  - `AConnectIt_GameMode_AdventureHub` (`AGameModeBase`, `Framework/GameMode/`) creates the hub component.
  - Test: `ConnectIt.Adventure.HubDiscovery` (a test world: discovery, the duplicate, a late node pushed twice,
    removal, Hidden hides the actor). The walk is Stage 2: discovery doesn't change states yet.

## Stage 2: the reveal walk

- **A pure function** (no world) that the hub component calls: inputs = the start node, each node's `Unlocks`, the
  progress (completed ids, broken seals, shards, seals), and the requirement check from the adventure subsystem;
  output = each node's state and the revealed links. Breadth-first with a visited set (loops can't hang it); completed
  → its unlocks are revealed; not completed → frontier (revealed, unlocks hidden); revealed nodes are Available when
  their gate is met or their seal is broken, else Gated (with what's missing).
- **After discovery** (the hub component's `BeginPlay`) it reads progress from the game instance's player file (gotcha 4: if none is
  loaded, a logged error and the start node only), runs the walk, applies states to the components, and remembers the
  result.
- **Late arrivals** (gotcha 3): a node pushing itself after the walk gets its state from the remembered result (Hidden if
  the walk didn't reach it).
- **Live updates:** after Break Seal (adventure subsystem), the node's state updates in place.
- **Debug:** a console variable / command to **reveal every node** (the owner's worst-case performance check, and for
  layout work).
- Tests: the walk on small graphs -- start only; after completions; a gated node revealed but not playable; a broken
  seal makes it available; a loop; a deleted id in progress ignored (gotcha 5); an unlock with no registered node
  skipped.
- **As built (2026-10-29; 79/79 tests):**
  - `UConnectIt_AdventureHubComponent::RunRevealWalk(Start, MapNodes, Progress, Table, bRevealAll)` (static, no world)
    returns `FConnectItRevealWalkResult`: `States` (revealed nodes; absent = Hidden) and `Links` (`From` / `To`,
    from completed nodes to revealed ones; a loop back to the start is a link too). Unlocks are followed only into
    nodes registered in this map (their soft pointers resolve because the components hold them), so off-map unlocks
    and ids the save has but no node has are skipped.
  - **States use the progress rules** (`GetNodeState`): required actions still count after a seal is broken (the
    adventure plan's Stage 2 rule) -- a broken seal makes a node Available only if its actions are also met.
  - `RefreshReveal` (on `BeginPlay` after discovery; on the Adventure subsystem's new `OnProgressChanged`, broadcast
    after a load, Break Seal and a recorded result) applies the states and keeps `RevealResult`; a late node gets
    its state from it.
  - Debug: console command `ConnectIt.Adventure.RevealAll 1` / `0`, applied at once.
  - Gotcha 4: the Adventure subsystem loads the save in its `Initialize` (game instance start), before any map; no
    subsystem -> a logged error and the start only.
  - Test: `ConnectIt.Adventure.RevealWalk` (fresh; completions; a loop; gated then seal broken; a deleted id; an
    off-map unlock; reveal all).

## Stage 3: validation

- (The hub game mode itself moved to Stage 1, to host the hub component.)
- **Map validation** (the World Settings check already branches on game mode): when the map's game mode is the hub game
  mode --
  - **no** component flagged Start → **error**; **more than one** → **error**;
  - one data asset on two components → **error**;
  - an `Unlocks` entry with no component in this map → **warning** (can't be revealed here);
  - a map used as a sub-map with no Exit flag → **error** (checked from the data assets that name it);
  - match-data checks stay off (as built in the World Settings validation plan).
- Takes over the adventure hub plan's Stage 5 checks that are about one hub map (reachability, one actor per data
  asset); the cross-map gate / shard reachability checks stay there.
- **As built (2026-10-29; 80/80 tests):**
  - `UConnectIt_AdventureHubComponent::CheckHubMapNodes(Nodes, bIsSubMap)` (static, no world) returns errors /
    warnings:
    - errors: no Start, or more than one; more than one Exit; a data asset on two nodes; a sub-map with no Exit;
    - warnings: a node with no data; an `Unlocks` entry with no node in this map; a node not reachable from the
      Start through `Unlocks` (the reachability check taken over from the hub plan's Stage 5).
  - `AConnectIt_WorldSettings::IsDataValid`: a map whose game mode is the hub game mode (or a subclass) gathers its
    node components and runs the checks, with no match data. A **sub-map** is a map that some map node asset's
    `SubMap` names, found through the asset registry's referencers of this map's package (only those assets are
    loaded).
  - Test: `ConnectIt.Adventure.HubValidation`.

## Stage 4: interaction, framing and links

- **Hover / click:** each node actor gets a **grid hover component** with category `Hover.MapNode`
  ([hover-component-plan](hover-component-plan.md)); the hub game mode's listener handles them -- nodes don't react on
  their own.
- **Scale on hover:** **`UTransformLerpComponent`** (`UnrealGameMechanics/Animation/`): start / end transform, time,
  `PlayForward` / `PlayReverse`; the listener plays it on hover / unhover.
- **Select:** focus the node (camera `RequestFocus` when the camera plan exists; until then a simple move of the view)
  and, on arrival, raise **`OnNodeSelected(Node)`** for the node panel (owner's UI). Start and Break Seal only from the
  panel.
- **Framing on load:** bounds = the revealed nodes + margin; focus the last-played node (from progress), else the
  start.
- **Links:** for each revealed link, spawn a **link actor** (a spline from node to node with a spline mesh / ribbon --
  the class set on the hub game mode); hidden links aren't spawned; shown when the destination is revealed.

## Stage 5: sub-maps

- Starting a node whose data asset names a **sub-map** loads that hub map (the adventure subsystem remembers the parent
  map and node).
- Completing the sub-map's **Exit** node returns to the parent map with the sub-map node **completed** (rewards on it
  applied once), and the walk there reveals what it unlocks.
- Tests: the return marks the parent node complete once.

## Stage 6: vault

Decision note (map nodes as components discovered by a component on the hub game mode, Start / Exit flags, the reveal
walk, the gotchas); a `code/`
recipe "build a hub map"; update [adventure-hub](adventure-hub.md) and the adventure hub plan (Stage 4 points here);
log.

## Not in this plan

- The node panel and progress UI (owner's editor work, hub plan Stage 6).
- Animating newly revealed nodes (diff between walks) -- later polish.
- Secret links (wishlist).

## Verification

- Each stage: build (editor closed); `Automation RunTests ConnectIt` passes.
- Owner, PIE: a hub map with five node actors (one Start); a fresh save shows only the start; complete it → its
  unlocks appear (one gated); break the seal → available; hover scales a node; select focuses it and fires the panel
  event; reveal-all debug shows every node; a map with no Start fails validation; placing a node in the editor doesn't
  register anything (no errors in the editor log).

## Settled before approval (owner, 2026-10-28: as written)

1. This plan replaces the hub plan's Stage 4 (and the one-map checks of its Stage 5).
2. The hub listener lives on `AConnectIt_GameMode_AdventureHub` (not a component on the player controller).
3. Link actors are spawned at runtime from node positions (not placed by artists).
