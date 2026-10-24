# Adventure mode: remaining holes and a build roadmap

Design note (2026-10-24), from a gap review with the owner. **Nothing decided yet** -- a map of what still needs
designing and a proposed order for building Adventure mode. Builds on [adventure-hub](adventure-hub.md),
[adventure-modules-and-progression](adventure-modules-and-progression.md) and the plans in this folder.

## Where things stand (2026-10-24)

- **Complete (9 plans):** AI director pipeline, conditions and legal moves, three directors, level world settings, one
  board change at a time, phased levels, tile powers, tile lock and chained tiles, first tile powers. **None has been
  playtested in PIE.**
- **In progress:** adventure hub (Stage 1 built), player actions and items (Stages 1-2 built; the hub's Stage 2 waits
  on it).
- **Approved, not started:** top-down camera.
- **Open questions:** full-board outcome, out-of-uses condition, level-director turn (kept as a question), rotating-rule
  randomness, lose conditions online, board-shift loop visuals, action gameplay tags.

## Holes that need design before an adventure level can be played end to end

1. **Teaching layer (the biggest).** Teaching levels need to *say* things and *point at* things: "select Place Piece",
   highlight the target tile, explain a refused click, step-by-step prompts. The Level director can script the steps,
   but it is **server-only** -- there is no channel yet from a director to the player's UI (text, highlighted tiles,
   arrows, "next" prompts). Needs a design: e.g. replicated level guidance on the GameState, or guidance events in the
   board event list so they play in order with the moves.
2. **The level's own flow and UI.** Before: a level card (name, goals, skills available). During: a goals / checkpoint
   display for staged and phased levels (the board already carries win progress). After: win / lose screen,
   **rewards reveal** (shards toward the next use, seals, items), retry, back to the hub. The hub plan covers the
   return trip, not these screens.
3. **Full-board outcome** (open question). An adventure level must know what a full board with no winner means --
   lose, draw, or retry.
4. **Out of uses** (open question). Closed puzzle levels ("you have three swaps") need a clear end when uses run out.
5. **Scripted opponents.** Tutorials need an opponent with pre-set moves. A Blueprint strategy per level works today; a
   reusable "scripted moves" strategy (a list of positions, with a fallback) would save a Blueprint per level.
6. **Content definitions.** The **skill catalog** (which skills exist and their actions), the **concept vocabulary**
   for node tags, the **shard tables'** numbers, the **level template** (purpose, concepts, skills -- from the
   progression note), and the **intro region's level list** (the Level 1 draft plus scoring, double lines, opponents,
   blocking).
7. **Player settings and persistence.** Camera toggles (edge scroll, auto-focus), sensitivity -- the camera plan
   expects the game to pass settings in; nothing stores them yet. Also: new game / continue in the main menu.
8. **Task list housekeeping.** Several rows in `_tasks/active.md` predate the last month's work and look done or
   superseded (AI turn check, MinMax rebuild, rules that change mid-level, building loadouts for playtesting) --
   worth a pass so the list reflects reality.

## A build roadmap (proposed)

- **Milestone 0 -- stabilise.** Play what's built: one PIE pass over each complete plan's Verification; finish the
  owner's pending editor work (the display-board rewire of the piece visuals, lock / chain / power visuals). Tidy the
  task list. Everything after this builds on these systems, so problems found now are cheapest.
- **Milestone 1 -- finish the infrastructure in flight.** Player actions and items → adventure hub Stages 2-5 →
  top-down camera (before the hub's Stage 4 needs it).
- **Milestone 2 -- design the slice's missing pieces.** The teaching layer, the level flow / rewards UI, the
  full-board answer (holes 1-4), and a scripted-moves strategy if wanted.
- **Milestone 3 -- vertical slice.** The **intro region**: 4-6 levels (placing pieces as one phased level, first score,
  double lines, opponents, blocking), its hub map, then collapsing into one node on a small world map with two next
  nodes and one Seal gate. End to end: new game → hub → levels → rewards → break a seal. Playtest with someone else.
- **Milestone 4 -- production.** The level template and a per-level checklist, the validator in the loop, then region
  two (Swap) and onward, a playtest per region.

## Open

- Which hole to design first (lean: the teaching layer -- the intro region can't teach without it).
- Is Milestone 0 a hard stop before more building, or done alongside Milestone 1?
- Vertical-slice size: is the intro region alone enough, or should one non-tutorial region be in it?
