# Adventure content definitions

Design note (2026-10-24). **Drafts to work through with the owner; nothing decided.** The content a first adventure
build needs defined: the player action catalog, the concept vocabulary, shard tables, the level template, and the intro
region's level list. From the [roadmap](adventure-mode-roadmap.md) (hole 6). Vocabulary per the
[player actions decision](../_decisions/2026-10-23-player-actions-tile-powers-board-operations.md): **player action**
(what a participant does), **tile power** (what a tile does), **board operation** (the change either makes); "skill" is
retired.

## 1. Player action catalog (draft)

**Permanent actions** (per-level list; match uses from shards in Adventure, Place unlimited):

| Action | Exists | Notes |
| --- | --- | --- |
| Place Piece | yes | always available, unlimited |
| Swap Pieces | yes | own piece ↔ adjacent piece |
| Board Shift | yes | **defaults to 1 step for now** (owner). A catalog entry per distance is bad design -- how the shift amount is controlled is to be brainstormed later |

**Item actions** (inventory; owner's lean: their own set, not the core actions). Board operations that exist with no
player action yet, as candidates:

| Candidate item | Board operation (exists) | Feel |
| --- | --- | --- |
| Capture | Capture Piece | flip one enemy piece |
| Remove | Remove Piece | take a piece off the board |
| Nudge | Move Piece | move one piece one tile |
| Wipe | Destroy Tile Multiplier | reset a tile's multiplier |
| Boost | Adjust Multiplier | raise a tile's multiplier |
| Force Place | Force Place Piece | place on any tile, even occupied |

## 2. Concept vocabulary (`ConnectIt.Concept.*` tags -- decided as a start, owner 2026-10-24)

What a node can *teach* / *use* (validator input; gates stay hand-set):

| Tag | A node teaches / uses it when... |
| --- | --- |
| `Concept.Action.Permanent.Place` / `.Swap` / `.Shift` | the player has (or must learn) that permanent action |
| `Concept.Action.Item.Capture` / `.Remove` / ... (one per item action) | the level hands out or expects that item action |
| `Concept.Scoring.Line` | completing a line scores and clears it |
| `Concept.Scoring.Multiplier` | tile multipliers matter to the solution |
| `Concept.Scoring.MultiLine` | one move completing two or more lines is the point |
| `Concept.Opponent` | there is an opponent taking turns |
| `Concept.Opponent.Block` | blocking the opponent's line is required |
| `Concept.Board.Locked` | locked tiles are on the board |
| `Concept.Board.Chained` | chained tiles (and their partners) are on the board |
| `Concept.Board.Hole` | inactive tiles / gaps shape the board |
| `Concept.Board.Phase` | the board or rules change mid-level |
| `Concept.Power.Push` / `.Repel` / `.Vacuum` / `.Crumble` | that tile power is on the board |
| `Concept.Goal.Checkpoint` | the win is a sequence of goals |
| `Concept.Goal.Lose` | the level has a lose condition to avoid (incl. No Moves) |

A small fixed list to start; designers add a tag when a level introduces something genuinely new.

## 3. Shard tables

One table per permanent action except Place. **No default numbers** (owner, 2026-10-24): the owner tunes them in the
editor. A teaching level gives the first shard by default.

## 4. Level template (draft)

For each level, before building it: **purpose** (one line); concepts taught / used; actions (closed: the exact kit /
open: required and recommended); goals (win stages, lose conditions, **stuck-board outcome**); opponent (none,
scripted, AI profile); board (size, phases, special tiles); rewards (shards, items, Seals); **difficulty (an integer,
1-100; owner, 2026-10-24)**; notes.
Most fields map straight onto the node's data asset and the level's directors.

## 5. Intro region level list (draft)

From the earlier tutorial drafts; teaches Place, then Swap and Shift (the base actions):

1. **Placing pieces** -- one phased level: 1x1; 2x1 with an occupied tile; locked / chained tiles; a first score.
2. **Scoring and multipliers** -- complete lines, see multipliers grow.
3. **Double lines** -- one move completing two lines.
4. **Opponents** -- 5x1: the opponent blocks one end, finish the other (scripted opponent).
5. **Blocking** -- 4x4: block a scripted opponent, and the block scores your line.
6. **Swap** -- teaching level (closed), then a Swap puzzle.
7. **Shift** -- teaching level (closed), then a Shift puzzle.
8. **Capstone** -- a short match vs a weak AI with Place + Swap + Shift; gives the region's Seal.

## Decided (owner, 2026-10-24)

- **Action catalog** looks good. Shift defaults to 1 step; how shift distance is controlled is brainstormed later (not
  one catalog entry per distance).
- **Shard numbers:** no defaults -- tuned in the editor.
- **Level template:** as drafted; **difficulty is an integer 1-100**.
- **Intro region list:** designed later, after the low-hanging fruit.
- **Concept tags:** the proposed list as a start, with **all actions under `Concept.Action`** --
  `Concept.Action.Permanent.*` and `Concept.Action.Item.*`.

## Shift distance (2026-10-24): A + B decided

**Today** ([decision 2026-10-08](../_decisions/2026-10-08-board-shift-n-steps-fixed-per-action.md)): the shift
operation already moves `Steps` places, but the distance is **fixed per action class** (`ShiftSteps`, defaults-only) and
the server rejects any other value -- so "Shift 2" would be its own class. One use whatever the distance; a full lap is
refused; pinned tiles are jumped, not counted.

Options (Claude):

- **A. A setting on the level's action entry.** The permanent action list entry for Board Shift carries **max steps**
  (default 1) -- one Shift class, the level decides the distance. The server checks `Steps` against the entry, not the
  class.
- **B. The player chooses, up to that max.** When shifting, the player picks 1..max (e.g. drag the line further). One
  use per shift (as today) or one use per step (distance costs more).
- **C. Distance as progression.** Shards (or a separate upgrade) raise Shift's max steps in open levels -- a second way
  an action grows, besides more uses.
- **D. Distance from the board.** Slide until blocked, or move as many places as there are pieces in the line -- a
  different action ("Slide") rather than a setting.
- **E. Longer shifts as items.** A "Big Shift" item action with a larger distance, spent from the inventory.

**Decided (owner): A + B** -- one Shift action, a per-level max steps (default 1), the player picks within it, one use
per shift. **C (distance as progression) and E (longer shifts as items) are wishlist items** (`_tasks/active.md`). This
changes the 10-08 decision's "fixed per action, not chosen by the player" (a new decision when planned).

## Open

1. Which item actions ship first.
2. ~~How Board Shift's distance is controlled~~ -- A + B (decided).
