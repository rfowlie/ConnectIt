# Adventure hub: the node map, currencies and regions

Design note (2026-10-23), the owner's look-and-feel ideas for Adventure mode. **Settled over five rounds the same day;
nothing built.** Plan: [adventure-hub-plan](adventure-hub-plan.md) (**approved 2026-10-23**, not built). Builds on [adventure-modules-and-progression](adventure-modules-and-progression.md) (level
types, the tutorial module, gating, the shard economy) and [adventure-mode](adventure-mode.md) (what a level can do).

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
