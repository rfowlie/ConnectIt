---
Date: 2026-10-22
status: Active
superseded by:
tags:
  - architecture
  - board
  - level-design
---
## Decision

A phased level changes the match mid-play through **events and data, each change a board change**:

- **A phase is an event, not a bundle.** The Level director's **Begin Phase** (a `ConnectIt.Phase.*` gameplay tag, shared by
  every level) is a board change. It writes `CurrentPhase` on the board, plays `Phase Changed`, and once committed every
  director hears **On Phase Changed**. Each director changes what it owns: the level its goals, layout and scoring rule; a
  side its loadout.
- **Layouts are data, opt-in.** A `UConnectIt_BoardLayout` asset holds cells: position, tile data, optional tile class and
  visual variant. A level's placed tiles stay its starting layout. A layout is authored with the layout tool in a sandbox
  level.
- **Set Layout is one board change:** a Change Layout operation removes, then adds, in one `Apply`. It is worked out against
  the board when applied, with one win check on the final board. Shared tiles keep their piece and multiplier. Starting
  pieces on added tiles are ordinary arrivals. Remove Tiles and Add Tiles also exist alone.
- **The board is the source of truth for which tiles exist.** Tile actors follow it on every machine from the events: the
  tile registry's Spawn Layout Tiles / Unregister Tiles At, and the piece registry for pieces.

## Why

- **Phases as events:** what changes at a phase is different in every level. Events keep "change anything" in the
  directors' scripts, instead of a bundle type that would have to grow a field for everything a phase might touch.
- **Layouts as data:** a phase change is then a single server-side change that every machine's visuals follow, like every
  other change. Streaming sub-levels would rebuild the board from actors loading at different times on each machine.
- **One change per layout:** two (remove, then add) would each be win-checked, and the in-between board could meet a lose
  condition (No Legal Move).
- **Phase on the board:** so clients and UI can read it, in order with everything else.

## What Would Change It

- A level needing a phase to change several things **atomically**, with no chance for anything between them. Today Begin
  Phase, the goals change and the layout are three changes in a row; a combined operation would merge them.
- Layouts that need non-tile actors (props) per phase -- sub-levels would come back into the picture for scenery.
- Player actions that add / remove tiles (the operations exist; range rules are undecided).

Plan: [phased-levels-plan](../design/phased-levels-plan.md). Question:
[phased-levels-runtime-change](../_questions/phased-levels-runtime-change.md). Builds on
[one board change at a time](2026-10-21-one-board-change-at-a-time.md).
