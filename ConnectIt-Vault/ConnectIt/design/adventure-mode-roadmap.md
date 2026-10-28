# Adventure mode: remaining holes and a build roadmap

Design note (2026-10-24), from a gap review with the owner. Plan for the level outcomes:
[level-outcomes-plan](level-outcomes-plan.md) (**approved 2026-10-25**, not built). Originally -- a map of what still needs
designing and a proposed order for building Adventure mode. Builds on [adventure-hub](adventure-hub.md),
[adventure-modules-and-progression](adventure-modules-and-progression.md) and the plans in this folder.

> Owner's in-editor checklist for the start of Adventure: [adventure-editor-checklist](adventure-editor-checklist.md).

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

## Decided (owner, 2026-10-24): low-hanging fruit first

1. **Teaching layer -- plan later.** A big step, not needed to start piecing together the first build. (Task row.)
2. **Level screens (card, goals display, win / lose, rewards reveal) -- the owner's editor UI work, later.** (Task row.)
3. **Not "full board" but "stuck board".** A full board isn't game over -- Swap or Shift can still score and clear it.
   What ends a level is **no participant able to make any move**. Every level must say what that means, so it is **its
   own parameter** (not left to whichever conditions a level happens to list): in Adventure usually a **loss**, with
   exceptions (a "freeze the board" level where stuck = **win**).
4. **Out of uses -- a No Moves lose condition** (owner, clarified): for puzzle levels, the player used up all their
   moves and hasn't won, so they lose. A per-side lose condition, separate from the board-wide stuck rule.
   *(Claude: this folds into 3 if "can make a move" counts uses -- a player with no uses left and no item budget can't
   move; with the opponent unable to move either (or absent), the board is stuck and the level's stuck outcome applies.)*
5. **A simple reusable scripted opponent: yes.**
6. **Content definitions: work through them now.**
7. **Player settings and save -- plan later.** (Task row.)
8. **Task review -- shortly.**

So the next small plan could bundle **stuck board (with uses) + the scripted opponent**; content definitions are
being worked through in [adventure-content-definitions](adventure-content-definitions.md).

## Level outcomes: decided (owner, 2026-10-24, round 2)

- **Stuck-board outcomes: Lose, Win and Draw** -- add Draw now. The setting lives on the **Level director**, default
  **Lose**.
- **Check order (not after every board change -- wasteful):** after each player action, check **No Moves** for that
  player. If it holds, check the opponent too:
  - **both have no moves → Stuck Board** (the level's stuck outcome);
  - **only the player has none** → **force-skip the player's turn** -- unless No Moves is one of the player's lose
    conditions, in which case **they lose**.
- **An item the player holds but the item budget won't allow doesn't count as a move.**
- **Instead of a scripted opponent, first a generic test opponent:** an AI that uses its **permanent actions one at a
  time, rotating each turn** (place, then swap, then shift...), so the board's interactions can be seen in testing
  without a level goal. (Pre-set tutorial moves can come later, with the teaching layer.)

### Notes (Claude)

- ~~"A full round of skips = stuck"~~ -- rejected by the owner (round 3, below).
- **The check also belongs at turn start:** after my action the *next* player may have nothing to do -- checking the
  side about to act when its turn begins catches that (and is when the skip happens).
- **The test opponent:** picks a legal target for its action from the board's seeded random stream (repeatable runs);
  an action with no legal target is skipped in the rotation; with nothing legal it passes like the AI does today.

## Level outcomes: round 3 (owner, 2026-10-24)

- **Stuck Board must genuinely mean stuck.** Two different checks:
  - **No moves now** -- what the player can do *this turn*: actions with a use available now (not on cooldown, turn cap
    not reached, item budget left) that have a legal target on the board. Fails → the turn is force-skipped.
  - **Stuck** -- **every action the side has, including ones on cooldown or at their turn cap**: does any still have
    match uses (or items within the match budget) and a legal target on the board? If **no side** has one, the board is
    stuck → the level's stuck outcome.
- **Efficiency:** the stuck check runs **only when No moves now triggers** -- not after every action.
- So the flow after each player action (and when a turn begins, for the side about to act): no moves now? → if not,
  carry on. If so → is every side stuck? → stuck outcome. Otherwise → if the side has a **No Moves** lose condition and
  can't ever move again, it loses; else its turn is skipped.
- *Claude, to confirm in the plan:* the **No Moves lose condition** = the side is stuck in the sense above (nothing left
  that could ever affect the board) -- not merely "no moves now", so a cooldown never loses a level.

## Level outcomes: round 4 -- the owner's definitions and flow (2026-10-24)

Replaces the round-3 flow above.

**Three states:**

- **No Moves** -- the player can't make any move *now* (turn caps, cooldowns...) and their turn-end requirements aren't
  met: their turn must be **force-ended**.
- **Stuck Player** -- the player can **never** make another move: no unlimited action, and every available action's
  match uses are spent.
- **Stuck Board** -- **both** players have no way to change the board.

**The flow** after each action, and when a turn begins:

1. The side has a move now → carry on.
2. If not, and **No Moves** is one of the level's conditions → run that condition (it may win, lose, or e.g. change
   phase).
3. If No Moves isn't a condition, or running it didn't end the match → check **Stuck Player** for this side.
4. Not stuck → **force-end the turn**. Stuck → check the **opponent** for Stuck Player too; both stuck → the **Stuck
   Board** outcome (the Level director's setting: Lose / Win / Draw, default Lose).

**Puzzle levels use Stuck Player, not No Moves**: e.g. the player has only Swap with 3 match uses and must reach a
score -- once the swaps are spent without reaching it, the player is stuck and loses (Stuck Player as a lose
condition). A simpler way to the same outcome: a **Turns condition** -- reaching turn 4 loses (three swaps used, no win).

### Notes on round 4 (Claude)

- **A Turns condition is the cheaper tool for most puzzles** and it's board-only (the board carries its turn number
  since tile powers), so the AI's search sees it too. Stuck Player covers what turns can't (several actions with
  different use counts, items). Both are worth having.
- **Stuck Player as a lose condition** (puzzles) is checked at step 3: stuck + the condition listed → it runs (loss).
- ~~Gap: one side stuck, the opponent not~~ -- **decided (owner): the stuck side's turns keep being force-ended while
  the opponent plays on**, unless Stuck Player is one of its conditions (then it runs).
- **Stuck Board counts legality -- decided (owner):** "neither side has an action with uses left **and** a legal
  target", so a full board with unlimited Place counts as stuck.
- No Moves and Stuck Player read uses from the PlayerState, so they're server-side checks, not rule-set conditions the
  AI's search evaluates.

## Open

- ~~Which hole first?~~ Low-hanging fruit first: stuck board, out of uses, scripted opponent, content definitions;
  teaching layer, level screens, settings later (decided).
- Is Milestone 0 a hard stop before more building, or done alongside Milestone 1?
- Vertical-slice size: is the intro region alone enough, or should one non-tutorial region be in it?
