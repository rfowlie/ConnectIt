# Adventure hub: the node map, currencies and regions

Design note (2026-10-23), the owner's look-and-feel ideas for Adventure mode. **Settled over five rounds the same day;
nothing built.** Plans: [adventure-hub-plan](adventure-hub-plan.md); the hub map: [adventure-hub-map-plan](adventure-hub-map-plan.md) (**built 2026-10-29**; decision
[hub map nodes are components](../_decisions/2026-10-29-hub-map-nodes-are-components.md); recipe
[build a hub map](../code/recipes/build-a-hub-map.md)). Builds on [adventure-modules-and-progression](adventure-modules-and-progression.md) (level
types, the tutorial module, gating, the shard economy) and [adventure-mode](adventure-mode.md) (what a level can do).

> **Vocabulary (2026-10-29):** the decisions below are kept as written; read them in the vocabulary settled by
> [player actions, tile powers, board operations](../_decisions/2026-10-23-player-actions-tile-powers-board-operations.md):
> - "skills" are **player actions** ("skill design" is the node's **action design**);
> - "numbered uses" are **permanent uses** (from shards, back every level);
> - "itemed uses" are **item actions** in the player's **inventory** (caps in the game's item inventory asset), spent
>   within the node's **item budget**.
>
> (A "skill test" still means the player's skill.) As built:
> [adventure-hub-plan](adventure-hub-plan.md) Stage 1 and its rename notes.

## Owner's direction

- **A map of nodes.** You start at a central node and can play any node connected to it. Completing a node unlocks
  the nodes connected to it, and so on outward.
- **The hub is a UI**, not a 3D world -- simplest. Side need: a map widget that **scrolls with the mouse position**.
- **Choose your own path**: go in whatever direction you like.
- **Skills are currency.** Place Piece is always yours; every other skill's uses are unlocked with **shards** --
  unless a level has a **closed skill design** (you play with the skills it gives, whatever you've unlocked).
- **A second, gating currency** for big milestones -- like Dark Souls' two bells before Anor Londo, then the four
  lords before the final boss -- so the player has to cover a good share of the map before the larger goals open.
- **Regions / biomes**, like Mario's worlds: many similar levels that focus on one skill or strategy and explore
  puzzles around it. Re-using a map idea across levels is fine.

## How it could be structured (Claude)

- **Three layers:** the **adventure map** (the whole graph) → **regions** (a biome: theme, focus skill, a cluster of
  nodes) → **nodes** (one level each).
- **A node, as data** (one asset for the whole map, or one per region): the level to load (map + match preset),
  position on the hub UI, connected nodes, region, **requirements** (gate currency needed, if any), **rewards**
  (shards per skill, gate currency), and its **skill design** (open or closed).
- **Node states on the map:** locked (not connected to anything completed), available, completed, **gated** (connected
  but its requirement isn't met -- shown with what it needs).
- **Two currencies:**
  - **Shards (per skill)** -- earned from levels, turn into that skill's uses (the existing note: a set number of
    shards becomes numbered uses).
  - **Gate currency** (name to pick: seals, keys, sigils...) -- earned at region milestones (a region's last node, a
    "boss" level), spent or held to open big gates. Held, not spent, matches the Dark Souls feel ("ring both bells").
- **Open vs closed skill design** per node, on the level:
  - *Closed* -- the level's Player director gives the loadout (works today: each level's director sets the starting
    loadout).
  - *Open* -- the player brings Place Piece plus the uses they've unlocked; the level may also add or forbid skills.
- **Save data:** completed nodes, shards per skill, skill uses, gate currency. There's a `ConnectIt_PlayerData` type
  today; whether it is the save is to check.
- **The hub UI:** a scrollable node map (edge-scroll by mouse position, plus drag / wheel), regions as backgrounds,
  lines for connections, node cards (name, region, rewards, requirements, best result).

## Pitfalls to design around

- **Free direction vs difficulty.** If any connected node can be played first, a region can't assume the player
  learned its neighbour's lesson. Either each region teaches its own skill from its first node, or a node's
  requirement says "complete X first" (an explicit prerequisite on top of adjacency).
- **Open levels and unknown kits.** A level that's open to the player's unlocked skills must be beatable with the
  **minimum guaranteed kit** (Place Piece) -- or say what it needs. Otherwise a player who spent their swaps elsewhere is
  stuck.
- **Running out of uses.** If skill uses are consumed permanently, a player can drain them and stall. Safety valves:
  replayable levels that pay shards, closed-design levels that don't need the player's stock, or uses that are a
  per-level allowance rather than consumed.
- **Gates must always be reachable.** Every gate needs enough currency available on paths the player can reach without
  passing that gate.
- **Replay and farming.** Decide whether completed levels pay again (and how much), or only once.
- **One big graph is hard to lay out.** Per-region sub-maps (zoom into a region from the world map), like Mario, keep
  each screen small.

## Decided (owner, 2026-10-23)

- **Two kinds of skill uses.** **Numbered uses** regenerate each level -- part of your permanent skill set.
  **Itemed uses** are one-time and gone. **Itemed uses never count towards a level's prerequisites** (too
  unpredictable). That also removes the "ran out of uses and stalled" pitfall: a requirement only ever looks at what
  you permanently have.
- **Gates being reachable** is for designers to watch and test hard (a map validator can help -- below).
- **No replays for now**: a completed level isn't played again. Later, a completed level could turn into an optional
  **challenge level**.
- **Maps per region, and Metroidvania-style secrets**: regions keep each map small, but a map can have **secret
  connections** to other regions (like Mario's secret exits).
- **Regions are biomes, not skill zones.** Later regions can mix skills and set-ups freely from level to level.
- **The intro region is its own map**; once the player leaves it, **the whole region becomes one node on the next
  map**. (So a node can be a level *or* a whole sub-map.)
- **Finishing a node unlocks all its neighbours**, except those that are gated.
- **Selecting a node never starts it.** The map auto-scrolls to centre the node and opens a panel with its details and
  a **Start** button -- no stray clicks start a level.
- **Gate currency gets a generic working name** until there's an art direction ("Keys" rejected -- a programming term).

## Making sure levels don't use concepts the player hasn't met (Claude, 2026-10-23)

The owner's two options -- skills as gates, or careful map layout -- plus a combination. Ideas:

1. **Concepts, not just skills.** The thing to gate on is a **concept**: a skill (Swap, Shift), but also a board
   mechanic (locked tiles, chained tiles, a tile power, phases). Each node says which concepts it **teaches** and which
   it **uses**. A level that uses chained tiles is "new" to a player who has never seen one, even if their skills are
   fine.
2. **Learned concepts are a permanent set** (like numbered uses): completing a node that *teaches* X adds X. Option one
   generalises: a node that *uses* X is gated until X is learned -- **automatically**, from its tags, so designers don't
   hand-write prerequisites. On the map it shows as "Needs: Swap" with a pointer to where Swap is taught.
3. **A map validator** (editor) walks the graph: every node that uses X must be reachable only through a node that
   teaches X; every gate must have enough Seals reachable without passing it; no node is unreachable. This is how
   option two ("careful construction") stays careful without relying on memory.
4. **The intro region teaches the base skills** (the owner's zone one) as one map: Place Piece, then Swap, Shift and a
   first multi-skill level or two. Leaving it collapses it into one node; from then on the base skills are learned and
   later regions can mix freely -- only *new* concepts (a tile power, chains) still need teaching.
5. **Teach on arrival.** A biome's first node(s) teach the biome's new concept (the first chained-tile level of a
   region with chains). Combined with 2, the rest of the biome unlocks naturally once that node is done.
6. **Closed levels can introduce a concept early** without breaking the rule: a closed-design level gives you the skill
   for that level only -- a taste of Swap before you've earned it, marked as "teaches nothing permanent".
7. **Secret connections respect the same rule**: a secret link into another region still shows that region's nodes as
   "Needs: X" if X isn't learned, so Metroidvania shortcuts can't skip teaching.

Lean: 1 + 2 + 3 + 4 together -- concept tags and automatic gating as the rule, the validator as the safety net, the
intro region as the place the base skills are taught.

## Decided (owner, 2026-10-23, round 3)

- **Concept tags on nodes: yes** (teaches / uses). A concept that turns out too restrictive to gate on (tile powers,
  say) is simply left out of a node's gating.
- **Gates are hand-specified, not automatic.** Automatic gating protects against a designer forgetting a gate, but
  also blocks players when a designer adds a skill or tile that isn't right for that area -- tedious to debug, and tile
  powers may prove too restrictive anyway. *(Claude: the concept tags still earn their keep as **validator input** --
  "node uses Swap but no path to it teaches Swap" becomes a warning, not a gate.)*
- **The hub is a 3D level, not a UI map** (replaces "the hub is a UI"):
  - **Map node actors**, placed and dressed however artists like; each holds a **`MapNodeDataAsset`** with everything
    about the node (level, region, rewards, requirements, concepts, skill design) **and which nodes it unlocks**.
  - Top-down camera, reusing camera movement / focus logic the owner has built (possibly in another project, to port).
  - **On load:** read the starting node's data asset, spawn its node actor; if completed, spawn each node it unlocks;
    repeat until every unlocked node is spawned; then focus the camera on the last-played node.
  - **The camera stays inside the zone of the unlocked nodes** -- keep some mystery.
- **Teaching:** the first zone does the heavy lifting so outer zones can be free. Seeing a node gated by a skill you've
  never met is *good* -- it builds intrigue ("I need to find this skill somewhere").
- **Closed levels** are the core teaching levels, and also hard skill tests (find the exact move sequence, like a chess
  puzzle).
- **Secret links are ordinary links** in the data; what differs is presentation -- a mysterious look, or **teleport
  nodes** whose destination you only see when you take them.
- **Alternate or hidden win conditions can unlock secret connections.**
- **Seals are spent.** Two kinds: **specific** (open one location) and **general** (spend wherever you choose). Care
  needed not to block progression; the validator checks it.

### Decided (owner, 2026-10-23, round 4)

- **The gate currency is called Seals** -- descriptive of its purpose (a seal is broken to pass). Specific seals open
  one location; general seals can be spent anywhere.
- **Secret links are a wishlist item** for now (in `_tasks/active.md`); the core hub plan leaves them out. The notes
  above stay as the starting point.
- **Node actors are placed in the hub map** by artists and **start hidden**; the load walk reveals them by toggling
  hidden. Artists get full freedom, the data asset stores less (no transform). Bonus: loading every node at level start
  gives a ready worst-case performance check.

## Decided (owner, 2026-10-23, round 5): shards, uses, items and Seals

- **Shards → numbered uses: a designer-set table per skill**, normally rising (e.g. 3, 5, 8 shards for the 1st, 2nd,
  3rd numbered use) -- early uses come quickly, later ones reward exploration, and each skill can be tuned.
- **Place Piece is granted from the start** -- the cornerstone skill, the one exception.
- **Required vs recommended skills** (replaces "does an intro level grant a use?"): for every other skill, a node says
  whether having it **blocks** the level or just makes it **easier**:
  - **Required skills** -- a hand-set gate: the node can't be started without at least one numbered use of the skill
    (unlocked through shards). Itemed uses don't count.
  - **Recommended skills** -- shown in the node's info panel ("Recommended: Swap"): not needed, but the level will be
    extremely hard, or borderline impossible, without it.
- **Itemed uses come from level rewards**: a node's data lists item rewards for completing it. They never count towards
  requirements.
- **Seals are earned at region capstones** (a biome's final / "boss" node -- usually **specific** seals for a known
  gate) **and from optional hard / puzzle levels** (closed-design challenges off the main path -- **general** seals).
  **Bonus goals** inside levels giving seals are a **wishlist** item (in `_tasks/active.md`, alongside secret links).
- **A teaching level doesn't grant a use by default** (owner; replaces Claude's lean). How many shards it gives is up
  to the designer; the owner's default: **the first shard** (not enough for a permanent use), with the nearby levels
  giving enough for the first use.

### Notes on round 3 (Claude)

- **Where a node actor's position comes from** (open): the owner's load order spawns actors from data assets, so the
  data asset would hold the transform -- but artists want to place nodes visually. Options: (a) node actors are placed
  in the hub map and start hidden; loading **reveals** them by walking the graph (artists place, data says what
  unlocks); (b) the data asset holds the transform and an editor button writes it from a placed actor. (a) keeps one
  source for "where" (the map) and one for "what" (the data asset).
- **A link that a hidden goal unlocks** needs the level's result to say *which* goal was met, not just "won": the
  existing win-condition groups already name the goal reached; the hub's save records it, and a link can say
  "unlocked by: completing the node" or "by: goal X".
- **The intro region collapsing into one node**: that node's data asset points at the intro map as a whole; completed
  when the intro map's exit node is.
- **The validator** walks the data assets: unreachable nodes; a gate with no reachable way to meet it (Seals counted
  along each path; specific seals must exist before their gate); concept warnings from the tags.

## Design lean (owner, 2026-10-24)

- **The core actions won't be item actions.** Place Piece, Swap, Shift and the like are very likely to stay permanent
  actions (shards → permanent uses); item actions are their own set, held in the inventory and spent through a
  level's item budget. See [player-actions-and-items-plan](player-actions-and-items-plan.md).

## Open

- ~~Shard → use conversion~~ -- designer-set rising table per skill; numbered uses are per level and permanent;
  itemed uses are one-time level rewards (decided rounds 2 and 5).
- ~~Gate currency~~ -- **Seals**, spent; earned at region capstones and optional hard levels (decided rounds 3-5).
- ~~One continuous map or per-region maps~~ -- per-region maps, with secret cross-region links (decided).
- ~~Completing a node unlocks all neighbours?~~ -- yes, except gated ones (decided).
- ~~Concept gating~~ -- decided round 3: tags yes, gates hand-specified, validator warns.
- ~~Node actor placement~~ -- placed in the map, revealed by the load walk (decided round 4).
- ~~How a secret connection is found~~ -- alternate / hidden win conditions (decided round 3).

## The reveal walk -- where the graph starts and how nodes are activated (Claude, 2026-10-28)

Answering the owner's question against the approved plan ([adventure-hub-plan](adventure-hub-plan.md), Stage 4) and
what's built (Stages 1-2: `UConnectIt_MapNodeDataAsset` with `Unlocks` and a `NodeId` GUID; the saved
`FConnectItAdventureProgress` with `CompletedNodes` and `BrokenSeals` as node ids).

**The graph lives in the data assets, not the map.** Each node's data asset lists the nodes it `Unlocks` (soft
references). The map only supplies the actors (each with a map node component pointing at its data asset).

**Where it starts -- not settled yet.** The plan says "the map's starting node" but not where that's named. Options:
- **The hub map's World Settings names its start node** (a data asset) -- the same place a map names its board, preset
  and catalog; one per map, validated. A sub-map's World Settings also names its **exit node**.
- **A "start" flag on one node component** in the map -- artist-friendly, validated to exactly one.

**The walk, each time the hub map loads** (the hub manager):
1. **Index the map:** find every map node component; build data asset → actor (validation catches a data asset with
   two actors, or an actor with none). All node actors start hidden.
2. **Read progress** from the save: completed node ids, broken seals, shards, seals held.
3. **Breadth-first from the start node**, with a visited set (so loops can't hang it):
   - reveal the node's actor;
   - if it is **completed**, queue every node in its `Unlocks`, and reveal the link line to each;
   - if it isn't, it's on the **frontier** -- revealed, but its unlocks stay hidden.
4. **Set each revealed node's state:** completed; **available** (its gate is met, or its seal is broken); **gated**
   (with what's missing, from the requirement check the subsystem already has).
5. **Frame the camera** on the revealed nodes (bounds + margin) and **focus** the last-played node (else the start).

**Nothing about the map's state is saved** -- only completed ids and broken seals. The walk derives everything else on
every load, so a designer adding a node or a link in a later build just shows up for existing saves.

**Live changes in the hub:** breaking a seal updates that node in place (gated → available); finishing a level returns
to the hub map, which reloads, so the walk runs again and reveals what the win unlocked (the "new nodes appear"
moment can be animated from the difference between the last walk and this one -- a later polish).

**Edge cases:** an unlock pointing at a node with no actor in this map (another map's node) → warning, skipped; a
completed node whose data asset was deleted → ignored; the start node missing → error, nothing revealed.

### Decided (owner, 2026-10-28)

- **A map node is a component on any actor**, not an actor class (flexible).
- **Components register themselves with a subsystem** on start. The subsystem keeps **two maps, inverses of each other:
  actor → data asset and data asset → actor.**
- **The start node is a flag on the component** ("Start"). While components register, the subsystem notes the one
  with the flag; then it runs the walk, enabling each node from the **Adventure progress of the active player file**
  (held by the game instance).
- **Edge cases:**
  - "An unlock with no actor" is mostly prevented: only actors carrying a map node component are nodes.
  - A deleted node id in the save only matters while testing; a way to override / renew the save comes later.
  - **Level validation:** a map whose game mode is **`ConnectIt_GameMode_AdventureHub`** with **no** map node component
    flagged Start → **error**.

### Notes (Claude)

- **Timing:** components that register in `BeginPlay` arrive *after* a world subsystem's `OnWorldBeginPlay`. Either
  register when the component is **registered with the world** (`OnRegister`, before any `BeginPlay`) and walk on
  `OnWorldBeginPlay`, or run the walk once registration settles (end of the hub game mode's `StartPlay`, or the next
  tick). Placed actors register early either way; a node spawned later just registers and is placed by the next walk.
- **Decided (owner, 2026-10-28): register in `OnRegister`, walk on `OnWorldBeginPlay`.** Guards that make it safe:
  - `OnRegister` also runs **in editor worlds** -- register only when `GetWorld()->IsGameWorld()` (or the subsystem only
    exists in game worlds of the hub game mode).
  - `OnRegister` can run **more than once** for one component (re-registration, construction scripts): registering is
    add-or-update, and `OnUnregister` removes it.
  - **Late arrivals** (a node spawned at runtime, a streamed sub-level) register after the walk: the subsystem applies
    that node's state on registration when the walk has already run.
  - The **player file's progress must be loaded** by the game instance before the hub map's `OnWorldBeginPlay`.
- **Validation also:** more than one Start flag in a hub map → error; a node data asset used by two components →
  error; an `Unlocks` entry with no component in this map → warning (it can't be revealed here).
- **Sub-maps:** an **Exit** flag on the component the same way, validated to exactly one in a sub-map.

### Revised (owner, 2026-10-28): a component on the hub game mode

The hub system is **`UConnectIt_AdventureHubComponent` on `AConnectIt_GameMode_AdventureHub`**, not a world subsystem
-- it only exists in hub maps. It **discovers** the map node components when it begins play (pull), and later spawns
push themselves to it; this replaces "register in `OnRegister`, walk on `OnWorldBeginPlay`" above (the editor-world
gotcha goes away; the save-loaded-first gotcha stays). Server-only is fine for a single-player hub. Detail:
[adventure-hub-map-plan](adventure-hub-map-plan.md).
