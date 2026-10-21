# Phased levels: changing the whole match at runtime

Design note (2026-10-21). **Nothing decided beyond the goal; nothing built.** Open thread:
[phased-levels-runtime-change](../_questions/phased-levels-runtime-change.md).

## Goal (owner)

A level made of **phases**. When an event fires (a win checkpoint, a level trigger...), the level moves to its next
phase and **anything can change**: the board's layout, the pieces on it, the rules, each side's goals and loadout.
Losing in any phase restarts the whole level, which is an easy way to add difficulty. The tutorial is built as separate
levels for organization ([adventure-modules-and-progression](adventure-modules-and-progression.md)), but phased levels
should be convenient to make later.

## What can change mid-match today

| Part of the match | Changeable at runtime? | How |
| --- | --- | --- |
| Pieces, tile active / multiplier | Yes, one tile at a time | Level director **Apply Board Operation** (Force Place, Remove, Toggle, Destroy Multiplier...) |
| Scoring rule | Yes | Level director **Set Scoring Rule** |
| A side's loadout | Yes | That side's director (Player / AI) |
| Level triggers | Yes | Level director re-arms them |
| AI strategies / think time | Yes | AI director |
| **Board layout** (which tiles exist, where) | **No** | Built once at match start from the tile actors in the map (`InitialiseBoardState`, from the tile registry) |
| **Win / lose conditions** | **No** | Copied from the directors' classes into the rule set at match start, fixed for the match |
| **Scores, goal progress** | **No** (only by moves) | `ScoreBoard`, `GroupsCompleted` on the board |
| Camera framing | Blueprint | -- |

So a phase change needs three new abilities: **relay the board**, **swap conditions**, **reset or keep score/progress**.

## The shape (proposed)

- **A phase is data**: a struct (or asset) listing what the phase sets -- each entry optional, "leave as is" by default:
  - **Layout**: the tiles that exist and their starting data (position, active, multiplier, shiftable, starting piece,
    later a tile power).
  - **Scoring rule.**
  - **Each side's win and lose conditions** for this phase.
  - **Each side's loadout.**
  - **Score**: keep or reset.
- **The Level director owns the list of phases** (Class Defaults) and a **Go To Phase (index)** call. Typical wiring: a
  phase's last win goal is a checkpoint; `On Win Checkpoint Reached` → Go To Phase(next). The final phase's last goal is
  the win.
- **A phase change is one board request** made after the chain that caused it (like every Level director change): one
  committed snapshot whose event list starts with a **Phase Changed** event carrying the whole new layout, so the
  visuals (and the display board from the tile-powers design) can tear down the old board and build the new one from
  the event alone.
- **Losing**: any phase's lose condition ends the match as today; "restart the level" is the existing match restart.

## The main open question: how does a layout come into existence?

The board's tiles are actors placed in the map today. Three ways to change them at runtime:

| Option | How | For | Against |
| --- | --- | --- | --- |
| **A. One big grid, masked** | Place every tile any phase uses; each phase turns tiles on / off (a new "absent" state, not just inactive) | Smallest change; tiles still placed and seen in the editor | Every phase must fit one grid; absent tiles must truly disappear (visuals, scoring, shift, AI); clunky for very different shapes |
| **B. Layout as data** | A **board layout asset** (cells + starting data); tile actors are spawned / pooled from it at match start and at each phase change | Any shape per phase; levels become data (many levels could share one map); the layout travels in the event, which the display board needs anyway | Biggest change: the tile registry, grid definition, piece registry and board seeding move from "find placed actors" to "build from data"; a way to author layouts in the editor |
| **C. Streamed sub-levels** | Each phase's tiles live in a sub-level; load / unload and re-register | Designers place tiles per phase in the editor as today | Streaming + replication timing + registry re-registration is fragile; heaviest runtime cost |

Claude's lean: **B**, possibly arriving through A. B is the one that makes "change everything" a single data swap, and it
also makes ordinary levels cheaper (a map stops being the board). An editor tool can still let a designer lay tiles out
visually and save them as a layout asset.

## Decided (owner, 2026-10-21): layout as data, opt-in, built from tile operations

- **Layout as data (B)**, but **authored in the editor**: a **grid layout tool** in a sandbox level lets a designer lay
  tiles out visually and save them as a layout asset.
- **Not the default.** A level's placed tiles stay its starting layout, as today. A phased level opts in through its own
  Level director logic: after event X, request a layout change with the new layout; the board state updates; the
  visuals follow.
- **A layout change is built from two board operations**: **Remove Tiles** and **Add Tiles**. **Set Layout** compares
  the current board with the new layout and carries out a Remove Tiles, then an Add Tiles. The two operations are
  useful on their own too -- e.g. a player action that adds or removes tiles within a range.
- **The board state is the source of truth for which tiles exist**; tile actors follow it. When the visuals receive a
  board with a different set of tiles, they reconcile: spawn (or take from a pool) the tiles that appeared, remove the
  ones that went, update values on the rest. The tile registry (position → actor) is updated by that same reconcile.
  At match start the placed actors still seed the board, as today.

### Notes from the discussion (Claude)

- **Tracking tile data as data is not the hard part**: a tile's gameplay data already *is* data --
  `FConnectItTileInitialData` (multiplier, active, shiftable, starting piece; later a tile power), which placed tiles
  hand to the board through `IConnectIt_TileDataProvider`. A layout asset is a list of {position, that struct}. The tool
  reads it from placed tiles through the same interface, so the board and the tool can't disagree.
- **What a data asset loses compared with a sub-level** is anything about a tile *actor* that isn't gameplay data: a
  different tile class or mesh, a skin, props around the board. Handle by letting a layout entry name a tile class (or
  a visual variant tag); scenery that isn't a tile stays in the map, untouched by phases.
- **Streaming sub-levels** would keep everything in placed actors, but then the board would be rebuilt from actors
  that are loading and unloading, on every machine, at different times -- the board state stops being the single
  source of truth during a phase change. With layouts as data the server changes the board once and every machine's
  visuals follow the replicated board, the same as for every other change.
- **Re-registering tiles** is a non-issue under this model, as the owner says: the registry becomes something the
  visual reconcile keeps up to date, not a source the board reads from.

## Also to settle

- ~~Tiles in both layouts~~ -- **decided (owner): they keep their current piece and multiplier; only tiles that appear
  or disappear change.** So Set Layout is exactly Remove Tiles + Add Tiles, no "update tiles" step. (A phase that wants
  shared tiles reset can follow with ordinary operations: Remove Piece, Destroy Multiplier.)
- ~~Pieces arriving with added tiles~~ -- **decided (owner): they are ordinary arrivals.** Add Tiles is an operation like
  any other, so it goes through the normal chain (settle powers, then scoring): pieces on added tiles set off Tile Enter
  powers and score if they complete lines. Whether an arriving piece stays after scoring is the operation's own answer,
  as for every operation. Consequence for designers: a phase layout that already holds a complete line scores the moment
  it appears -- avoid it, or use it on purpose.
- **Removing a tile with a piece on it**: the piece goes too (a Tile Exit for tile powers).
- **Range limits** for player add/remove actions: adjacent to existing tiles? within the layout's bounds?

- Score between phases: keep, reset, or per phase.
- Win progress (`GroupsCompleted`): reset per phase, or phases are simply the ordered win groups of one list.
- Pieces carried over when only the rules change (a phase with no layout entry keeps the board).
- Camera framing per phase (a layout bound the camera reads?).
- The AI's search: a layout change happens between moves, so the search never sees one mid-thought; its look-ahead
  can't foresee one (same as level triggers).
- How it fits the tile-powers plan: tile powers per tile would become part of the layout data.
