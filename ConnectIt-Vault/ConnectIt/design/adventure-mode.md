# Adventure Mode

Design note (2026-10-08), design/ is schema-less. **Nothing here is decided or built beyond what already exists.**
It collects what Adventure mode is today, the levers a level can use without new code, and a first pool of prospective
levels from a brainstorm. The pool is a menu, not a plan: 1-2 get picked later.

## What it is today

- Single player: a human against one AI opponent (`AConnectIt_GameMode`, `MatchType = Adventure`; Online is human vs
  human). Classic ConnectIt (place-piece only) keeps the MinMax AI; Adventure levels are meant to feel authored, with
  a different, still-undesigned AI approach -- see
  [MinMax scoped to Classic](../_decisions/2026-09-24-minmax-scoped-to-classic-adventure-gets-bespoke-ai.md).
- It is also the planned fallback playable demo if the networking route stalls (2026-09-18 check-in).
- A level is a map plus a `UConnectIt_LevelConfigDataAsset`: rule set (scoring, win condition), player loadout, AI
  profile (strategy + its loadout), registries.

## Levers a level has with no new code

- **Map:** board shape and size; holes by leaving tiles out.
- **Per tile** (designer data, [tile data interface](../_decisions/2026-09-20-designer-tile-data-via-blueprint-interface.md)):
  starting multiplier, active or dead (a dead tile isn't placeable and breaks lines), pinned or free to shift
  (`bCanShift`), and a starting piece for either faction.
- **Rules:** line length (3+), target score (also overridable per match).
- **Loadouts** (player's and AI's can differ): Place, Swap (numbered, starting uses, held cap) and Shift (one action
  class per direction and [step count](board-shift-steps.md)); per-turn caps, cooldowns, and a turn-end tree
  (Any/All of "used X times").
- **AI:** profile = MinMax depth and named weights + a loadout.

## Constraints to design around

- The AI search only places pieces (as of 2026-10-07), so an AI loadout is effectively Place-only.
- One AI opponent today; no per-faction starting score; win conditions are Score Threshold and Piece Count; each side has an ordered list
  of goals (groups of conditions that must all hold), and the player and the AI are set separately (no fallback).
- A full board with no winner has no defined outcome ([question](../_questions/full-board-outcome.md)).
- Mid-level rule changes, Crumble, switch/pole tiles are designed but not built
  ([rules that change](mid-level-rule-changes.md), [tile powers](tile-powers.md),
  [switch and pole](switch-and-pole-tiles.md)).

## Prospective levels -- no new code needed

| # | Name | Pitch | Levers |
|---|---|---|---|
| 1 | **Sliding Walls** | Rows of dead tiles split the board; shifting rotates whole tile data, so the walls move and open or close lanes. | inactive tiles, Shift |
| 2 | **Anchored Gears** | A cross of pinned tiles that shifts rotate around; lines are forced through the anchors. | `bCanShift`, Shift in 4 directions, steps 1 and 2 |
| 3 | **Shift Puzzle** | Rows pre-built but scrambled; Shift only. Align them to score before a passive AI catches up. | starting pieces, Shift-only loadout, shallow AI |
| 4 | **Defuse** | The AI starts one placement from a huge line on a x5 tile; the player has one shift to break it, then must outscore. | starting pieces, multipliers, capped Shift |
| 5 | **Swap Heist** | A dense board of AI pieces; the player has 3 swaps (no refills) against an AI that can only place. | numbered Swap, asymmetric loadouts |
| 6 | **The Gauntlet** | The AI has several pre-placed one-move threats and a low target score; block and build at once. | starting pieces, target score |
| 7 | **Hotspot Rush** | x5 tiles in the centre and a short target; scoring clears the tile and raises its multiplier, so the centre stays contested. | multipliers, target score |
| 8 | **Corridors** | A maze of dead tiles on a connect-3 board; lines only fit in narrow lanes. | map shape, inactive tiles, line length 3 |
| 9 | **Double Time** | Two placements per turn for both sides. | turn-end tree, per-turn cap |
| 10 | **Pick Your Poison** | A turn ends after 2 placements OR 1 shift, so each turn is a choice. | turn-end tree Any-group |
| 11 | **Cooldown Cannon** | Shift is powerful (steps 2-3) but on a 3-turn cooldown: a tempo game. | cooldown, `ShiftSteps` |
| 12 | **Rival Ladder** | One map, three AI opponents: a cautious blocker, a greedy scorer, a deep thinker. | AI profiles and weights |
| 13 | **Connect-3 Sprint** | A tiny board, connect-3, target ~10: teaches scoring and clearing in under a minute. | line length 3, small map |
| 14 | **Marathon** | Connect-5 on a big board with scattered x3 tiles and a high target; slow, positional. | line length 5, large board |

Update (2026-10-08): level 5 grew into two swap-only puzzle levels, see [swap-puzzles](swap-puzzles.md). That added a
**Piece Count** win condition (Equal 0 = clear the board) and separate **Player / AI win group lists** on the level config's
Rules (each side lists its goals in order; completing a goal is a checkpoint, completing the last is the win; no defaults, no
fallback), both built. Phased levels ("score first, then clear the board with a new loadout") are two groups plus a level
Blueprint on the checkpoint delegate: [win-condition-stages](win-condition-stages.md).

## Prospective levels -- small builds first

- **Raid (captures):** the capture operation exists but no action sends it; needs one action class.
- **Rotating scoring axis:** designed; needs the allowed-axes setting on the line rule, a turn trigger, and the
  AI evaluation to respect the active axis.
- **Crumble / switch and pole:** designed, not built.
- **Fill the board:** needs the full-board question answered and a win condition for it.

## Shortlisting notes (not decisions)

- The urgent task "Build 3 unique loadouts + matching maps" could use three that differ in loadout and feel -- e.g. 13
  (Place only), 1 or 2 (Place + Shift), 5 (Place + Swap); they double as friend-playtest builds.
- 1 and 2 show what makes ConnectIt different from Connect 4 (shifting tile data, pinned and dead tiles).
- 12 is the cheapest to verify: one map, three AI profiles.
- Which two to build depends on what the first Adventure levels are *for* (playtest checkpoint, fallback demo, portfolio
  showcase) -- not yet answered.

## Open

- What the first Adventure levels are for (above), and which 1-2 to build.
- How an Adventure AI that isn't "play optimally" is built (toolkit vs bespoke controller) -- still open per the
  scope decision.
- Whether Adventure needs a campaign structure (order, unlocks) or just a level list from the main-menu catalog
  ([match setup](ai-profiles-turn-plumbing-and-match-setup.md)).
