---
type: plan
status: not-started
approved: true
approved-on: 2026-10-23
completed-on:
playtested: false
design: "[adventure-hub](adventure-hub.md)"
question:
decision:
---

# Plan: the adventure hub (node map, progress, currencies)

Design: [adventure-hub](adventure-hub.md) (rounds 1-5, 2026-10-23); background:
[adventure-modules-and-progression](adventure-modules-and-progression.md). Written 2026-10-23 in a design-only session;
**approved by the owner 2026-10-23**. Not built -- building happens in its own session.

## Context

Settled with the owner on 2026-10-23:

- **The hub is a 3D level** of **map node actors** placed and dressed by artists, each holding a
  **`MapNodeDataAsset`** (everything about the node, including which nodes it unlocks). Actors start **hidden**; on
  load the game walks the unlock graph from the starting node and reveals each unlocked node, then focuses the camera
  on the last-played node. The camera stays inside the area of revealed nodes. Top-down camera, reusing the owner's
  camera logic (to port).
- **Finishing a node unlocks all its neighbours**, except gated ones. Regions are **biomes**, each its own map; the
  **intro region** is one map that becomes **one node** on the next map once left.
- **Selecting a node never starts it**: the camera centres it and a panel shows its details and a **Start** button.
- **Gates are hand-specified**: required skills and / or **Seals**. Nodes also carry **concept tags** (teaches / uses)
  and **recommended skills** -- information for the player and the validator, never gates.
- **Skills:** Place Piece from the start. Other skills' **numbered uses** (permanent, regenerate each level) come from
  **shards** through a **designer-set table per skill** (normally rising). **Itemed uses** (one-time) are level
  rewards and never count towards requirements. A teaching level gives shards as its designer chooses (default: the
  first shard, not enough for a use).
- **Open vs closed skill design** per node: closed = the level's own loadout (teaching levels, hard puzzle levels);
  open = Place Piece + your numbered uses + any itemed uses you choose to bring.
- **Seals** are spent. **Specific** seals (a tag) open one gate; **general** seals open any. Earned at region capstones
  (specific) and optional hard / puzzle levels (general).
- **No replays for now**: a completed node isn't played again.
- **Wishlist, not in this plan:** secret links, bonus goals that give seals.

Each stage builds and passes the tests before the next; the editor must be closed for builds; the owner commits between
stages.

## Stage 1: adventure data

- **`UConnectIt_MapNodeDataAsset`** (`Public/Adventure/`):
  - identity: a stable **`NodeId`** (a GUID set when the asset is created -- the save refers to nodes by it, so renaming
    or moving the asset doesn't lose progress); display name, description, thumbnail, region (gameplay tag);
  - what it plays: the **level** (world) and **match preset** -- *or* a **sub-map** (another hub map; the node is the
    whole region, complete when that map's **exit node** is);
  - **`Unlocks`**: the nodes completing this one reveals;
  - **gate**: required skills (each: at least one numbered use), required **specific seals** (tags), **general seals**
    (count);
  - information: recommended skills, concept tags (teaches / uses);
  - **skill design**: open / closed;
  - **rewards**: shards per skill, itemed uses per skill, specific seals (tags), general seals (count).
- **Skill shard tables**: per skill, the shard thresholds for the 1st, 2nd, 3rd... numbered use (designer-set,
  normally rising) -- one data asset named in project settings (`ConnectIt_ProjectSettings`).
- **Seal tags**: `ConnectIt.Seal.*` in the tag config.
- Validation on save: a node with neither a level nor a sub-map; a closed node with no loadout in its preset; unlocks
  pointing at itself; a skill's thresholds not rising (warning only).
- Tests: shards → numbered uses through a table; a node's requirement check against a progress value.

## Stage 2: progress and the save

- **`UConnectIt_AdventureSaveGame`** (`USaveGame`): completed node ids; **broken seals** (gated node ids opened);
  shards per skill (numbered uses are **derived** from shards and the table, not stored); itemed uses per skill; seals
  held (specific tags, general count); the last-played node id.
- **`UConnectIt_AdventureSubsystem`** (game instance): load / save (one slot to start); queries -- a node's state
  (hidden / available / gated, with what's missing / completed), numbered uses of a skill, seals held; actions --
  **break a seal** (spend the seals a gated node needs; it becomes available for good), **record a result** (on a win:
  mark complete, grant rewards, spend the itemed uses the player took in, save).
- Tests: state of a node through complete / gate / break; rewards applied once; derived uses grow with shards; itemed
  uses don't satisfy a required skill.

## Stage 3: starting a node and coming back

- **Start** from the hub: the subsystem builds the match settings (`UConnectIt_MatchSetupSubsystem` -- level, preset)
  and remembers the node being played.
- **The player's loadout**: closed -- the preset's Player director gives it, as today. Open -- at match start the Player
  director is handed **Place Piece + the player's numbered uses + the itemed uses chosen in the panel** (a numbered
  action's starting uses = numbered + itemed; the itemed part is marked so its consumption can be counted).
- **Coming back**: when the match ends, the result (won / lost) goes to the subsystem, which records it, and the hub map
  loads again, focusing the node just played. A loss changes nothing except itemed uses actually spent (to confirm).
- Tests: open loadout built from a progress value; itemed uses spent in a match are removed on return.

## Stage 4: the hub level

- **`AConnectIt_MapNodeActor`**: holds its `MapNodeDataAsset`; hidden by default; a visual **state** (available,
  gated, completed, the "you were here" focus) set by the hub, shown in Blueprint; hover and select events.
- **The hub manager** (the hub map's GameMode or a world subsystem): on load, finds every node actor in the map
  (data asset → actor), walks the graph from the map's **starting node** -- reveal the start; for each completed node,
  reveal its unlocks; recurse -- sets each revealed actor's state, then focuses the last-played node.
- **Camera** (owner's ported logic): top-down, pan with drag / wheel / mouse at the screen edge, **bounded** to the
  revealed nodes' area plus a margin; **focus** a node on selection.
- **Selection**: select → focus + open the node panel (stage 6); Start and Break Seal live in the panel only.
- **Sub-maps**: a node whose data asset names a sub-map loads that hub map on Start; leaving a sub-map (its exit node
  completed) returns to the parent map with that node complete.
- Debug: a console command / setting to **reveal every node** (the owner's worst-case performance check, and for
  layout work).
- Tests: the reveal walk on a small graph (start only; after completions; a gated node revealed but not playable).

## Stage 5: the map validator

An editor utility (map check on the hub map, plus a button) that walks the data assets from the starting node:

- every data asset reachable from the start has exactly one node actor in the hub map; no actor without a data asset;
- unreachable nodes;
- **gates**: each gate's seals can be earned on paths that don't pass through it (specific seals before their gate;
  general seals counted along the way); each required skill can reach its first threshold from shards available
  before the gate;
- **concept warnings**: a node that *uses* a concept with no node that *teaches* it on any path to it;
- a sub-map with no exit node.

Warnings and errors only -- it never changes data.

## Stage 6: UI and art (owner, editor)

- Node panel: name, description, region, rewards, required (with what's missing) and recommended skills, concepts,
  gate and its seal cost, **Break Seal** / **Start**; the itemed uses to bring (open nodes).
- **Breaking a seal** (owner, 2026-10-23): selecting a gated node opens its panel with a **Break Seal** button,
  **interactable only when the player can afford it** (the subsystem's query says so, and why not). Clicking it breaks
  the seal through the subsystem (spends the seals, records it, saves), plays a **break visual** on the node actor, and
  the node turns available.
- Progress display: shards toward the next use per skill, seals held.
- Node actor visuals per state; region dressing; the camera port.
- The intro region map and its node on the world map.

## Stage 7: vault

Decision notes (the hub as a 3D level with revealed node actors; skills, shards and Seals); `code/` recipe "add an
adventure node / region"; update [adventure-hub](adventure-hub.md) and
[adventure-modules-and-progression](adventure-modules-and-progression.md); log.

## Not in this plan

- Secret links; bonus goals that give seals (both wishlist in `_tasks/active.md`).
- Replays / challenge versions of completed levels.
- The content itself: the intro region's levels (the Level 1 draft), other regions.
- Several save slots.

## Verification

- Each stage: build (editor closed); `Automation RunTests ConnectIt` all pass.
- Owner, PIE: a three-node test map -- start node available; complete it, its two neighbours appear, one gated by a
  seal; earn the seal elsewhere, break it, play it; quit and relaunch, progress kept; the camera stays inside the
  revealed area; selecting never starts a level.

## Settled before approval (owner, 2026-10-23)

1. Nodes are identified in the save by a GUID on the data asset (renames / moves keep progress).
2. Seals are spent with the **Break Seal** button in the selected gated node's panel -- enabled only when affordable;
   clicking breaks the seal, shows a visual and saves; the node stays open. Never automatic on Start.
3. On a loss, only the itemed uses actually spent are lost; nothing else changes.
4. Numbered uses are derived from shards (only shards are saved) -- less data, more error-proof.
