---
type: plan
status: not-started
approved: true
approved-on: 2026-10-28
completed-on:
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
- **Components register themselves with a world subsystem**, which keeps **actor → data asset** and **data asset →
  actor** maps.
- **Register in `OnRegister`, walk on `OnWorldBeginPlay`.** On a map load, world subsystems exist before the level's
  components register, and every placed component registers before any `BeginPlay` -- so the walk sees every node.
- **The start node is a Start flag on the component** (an **Exit** flag likewise for sub-maps).
- **The walk** enables nodes from the **active player file's Adventure progress** (game instance): breadth-first from
  the start, through completed nodes' `Unlocks`; states completed / available / gated; nothing about the map is saved
  beyond completed ids and broken seals.
- **Validation:** a `ConnectIt_GameMode_AdventureHub` map with no Start flag → error.

### Gotchas (must be handled -- each has a test or a check below)

1. **`OnRegister` also runs in editor worlds** (placing or moving an actor). Register only in **game worlds**
   (`GetWorld()->IsGameWorld()`), and the subsystem only exists in game worlds.
2. **`OnRegister` can run more than once** for one component (construction script reruns, re-registration).
   Registering is **add-or-update**; **`OnUnregister` removes** the entry.
3. **Late arrivals** (a node spawned at runtime, a streamed sub-level) register **after** the walk: the subsystem
   applies that node's state as it registers when the walk has already run.
4. **The player file's progress must be loaded** by the game instance **before** the hub map's `OnWorldBeginPlay`,
   or the walk sees an empty save and reveals only the start.
5. A **deleted node id** in a save only matters in testing (ignored by the walk); a save override / renew tool comes
   later.

Each stage builds and passes the tests before the next; the editor must be closed for builds; the owner commits between
stages.

## Stage 1: the node component and registration

- **`UConnectIt_MapNodeComponent`** (`Public/Adventure/`): `NodeData` (`UConnectIt_MapNodeDataAsset`), **`bIsStart`**,
  **`bIsExit`** (editable on instances); **`State`** (Hidden / Available / Gated / Completed, plus a "focus" flag) with
  `OnStateChanged` for Blueprint; on state Hidden it hides its owner (and its collision), otherwise shows it.
- **`UConnectIt_AdventureHubSubsystem`** (world subsystem, `ShouldCreateSubsystem`: game worlds only):
  `RegisterNode` / `UnregisterNode`; `NodeToActor` and `ActorToNode` maps (by data asset); the start and exit
  components found while registering; `GetActorForNode`, `GetNodeForActor`.
- The component registers in **`OnRegister`** (game worlds only -- gotcha 1), unregisters in **`OnUnregister`**;
  registering twice updates, not duplicates (gotcha 2); a second component with the same data asset → logged error, the
  first kept.
- Tests: registering twice keeps one entry; unregister removes from both maps; duplicate data asset rejected
  (unit-level with a test world where needed).

## Stage 2: the reveal walk

- **A pure function** (no world) that the subsystem calls: inputs = the start node, each node's `Unlocks`, the
  progress (completed ids, broken seals, shards, seals), and the requirement check from the adventure subsystem;
  output = each node's state and the revealed links. Breadth-first with a visited set (loops can't hang it); completed
  → its unlocks are revealed; not completed → frontier (revealed, unlocks hidden); revealed nodes are Available when
  their gate is met or their seal is broken, else Gated (with what's missing).
- **On `OnWorldBeginPlay`** the subsystem reads progress from the game instance's player file (gotcha 4: if none is
  loaded, a logged error and the start node only), runs the walk, applies states to the components, and remembers the
  result.
- **Late arrivals** (gotcha 3): a node registering after the walk gets its state from the remembered result (Hidden if
  the walk didn't reach it).
- **Live updates:** after Break Seal (adventure subsystem), the node's state updates in place.
- **Debug:** a console variable / command to **reveal every node** (the owner's worst-case performance check, and for
  layout work).
- Tests: the walk on small graphs -- start only; after completions; a gated node revealed but not playable; a broken
  seal makes it available; a loop; a deleted id in progress ignored (gotcha 5); an unlock with no registered node
  skipped.

## Stage 3: the hub game mode and validation

- **`AConnectIt_GameMode_AdventureHub`** -- the hub maps' game mode (no match; owns the hub listener of Stage 4 and
  the return-from-level focus).
- **Map validation** (the World Settings check already branches on game mode): when the map's game mode is the hub game
  mode --
  - **no** component flagged Start → **error**; **more than one** → **error**;
  - one data asset on two components → **error**;
  - an `Unlocks` entry with no component in this map → **warning** (can't be revealed here);
  - a map used as a sub-map with no Exit flag → **error** (checked from the data assets that name it);
  - match-data checks stay off (as built in the World Settings validation plan).
- Takes over the adventure hub plan's Stage 5 checks that are about one hub map (reachability, one actor per data
  asset); the cross-map gate / shard reachability checks stay there.

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

Decision note (map nodes as registered components, Start / Exit flags, the reveal walk, the gotchas); a `code/`
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
