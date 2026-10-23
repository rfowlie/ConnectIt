---
schema: recipes
task: Give a match an AI opponent that plays the way you designed -- a tuned built-in one, a Blueprint-made strategy, several strategies with a selector, or a Blueprint AI controller -- through an AI director Blueprint named by the match preset.
touches:
  - Source/ConnectIt/Public/AI/ConnectIt_AIDirector.h (a Blueprint subclass: its Class Defaults are the AI's setup)
  - Source/ConnectIt/Public/Framework/Data/ConnectIt_MatchPreset.h (the match preset: AI Director Class, Player Director Class)
  - Source/ConnectIt/Public/Framework/Data/ConnectIt_WorldSettings.h (the map's World Settings: Level Config, Default Match Preset)
  - Source/ConnectIt/Public/Framework/GameMode/ConnectIt_GameMode.h (the level's GameMode: Match Type, AI Controller Class)
  - Source/ConnectIt/Public/AI/ConnectIt_AIStrategy.h (a Blueprint subclass, only for a bespoke strategy)
  - Source/ConnectIt/Public/Framework/Controller/ConnectIt_AIController.h (a Blueprint subclass, only if you need one; see C)
reconciled: 2026-10-23
commit: 2f74abd
---

# Recipe: set up a custom AI opponent

## Goal

A match whose opponent behaves the way you designed, for designers working in the editor. Read [[ai-turn]] first if you want
the flow behind it; the short version is: **an AI director Blueprint says who the opponent is (its loadout, its strategies,
its selector; its goals are the Level director's), the match preset names it, the strategies propose moves, the selector picks one, the controller
only does the timing and submitting.**

## First, which of these do you actually need?

| I want... | Do this | Section |
|---|---|---|
| An easier / harder / more mistake-prone classic opponent | The **AI director** with MinMax, different numbers, a Top N selector | A |
| An opponent with its own rules of thumb ("two random pieces per turn, only one may score") | A **Blueprint strategy** in the director's Strategies | B |
| The opponent to swap or shift pieces | A **Blueprint strategy** (the built-in MinMax only places) + a loadout with those actions | B |
| Several strategies at once, or a different way of picking among their moves | More **Strategies** + a **Selector** | [[combine-ai-strategies]] |
| The AI to change during the match (alternating moves, new moves at a checkpoint, tells) | Events on the **AI director Blueprint** | [[direct-the-ai-during-a-match]] |
| Several opponents for the menu ("Classic Easy", "Classic Hard") | One **AI director Blueprint** (or a child Blueprint) per opponent, one **match preset** naming each | A, step 6 |
| A different *thinking animation / extra component* on the opponent actor | A **Blueprint AI controller** | C |
| To change how the AI times its turn, retries, or submits | Not possible in Blueprint today: ask for a C++ hook | C |

**You rarely need a custom controller**: its decisions are not overridable, and which opponent plays comes from the preset's
director, not from the controller class.

## Prerequisites

- A level whose match is played as **Adventure** (the GameMode's `Match Type` is Adventure, which is what spawns the AI).
- A **match preset** for it (Data Asset of class `ConnectIt_MatchPreset`; the map's **World Settings > ConnectIt > Default
  Match Preset** names it, next to its **Level Config**, the board).
- An **action loadout** for the AI (an asset like a player's): the AI may only do what its loadout's actions allow, and the server
  holds it to that exactly like a human. A Place Piece action to place, a Swap action to swap, and so on, with the per-turn
  limits and turn-end requirements you want.
- **Condition assets** for the AI's goals ([[add-a-win-condition]]).
- A **Player director** Blueprint for the human side (its `Starting Loadout`; its goals are the Level director's
  Player Win / Lose Conditions) -- the preset needs both.

## Steps

### A. The built-in opponent, tuned (no Blueprint)

1. Content Browser: **Blueprint Class** with parent `ConnectIt_AIDirector` (e.g. `CIAD_Classic_Hard`). Open its **Class
   Defaults** -- everything below is set there. As it comes it is the classic opponent: one **MinMax (Classic)** strategy and
   the **First Available** selector.
2. **Starting Loadout**: the AI's action loadout. (The AI's goals are not here: they are the **Level director's AI Win / Lose
   Conditions** -- see [[add-a-win-condition]].)
3. Under **Strategies > MinMax (Classic) > Search**: `Max Depth` (how many moves it looks ahead), `Default Time Budget
   Seconds` (only used when nothing else gives a time; under a director the director's think time decides), the `Evaluation Weights` / `Ordering Weights` (what it values; ordering is speed only), and
   optionally `Own` / `Opponent Request Types` (empty = what each side may do). `Max Candidates` = how many fallback moves it
   hands over; `Label` = its name in logs.
4. **Mistakes**: set **Selector** to **Top N (Mistake Chance)**: `Top Moves Considered` + `Mistake Chance` (how often it plays
   one of the 2nd..Nth best moves on purpose). Or **Weighted by Confidence** for a smoother feel (`Temperature`: near 0 =
   almost always best, higher = more random).
5. Think time on the director: `Think Time Budget Seconds` (how long the strategies may think per move; MinMax is told
   `x Strategy Budget Share` of it), `Min Think Seconds` (it never moves sooner than this).
6. Several difficulties for the menu: a director Blueprint per difficulty (a child Blueprint of a shared one changes only what
   differs), a preset per difficulty naming it (e.g. `CIMP_Classic_Easy`, `CIMP_Classic_Hard`), listed in the level catalog's
   **Match Presets**. Go to step 7.

### B. Make a Blueprint strategy

1. Content Browser: **Blueprint Class** with parent `ConnectIt_AIStrategy` (e.g. `BPAIS_TwoRandomPieces`).
2. Open **Functions > Override > Begin Decision**. It receives the **Context**:
   - `Board`: a copy of the current board state; `Own Slot`: your faction slot.
   - `Rules`: the match's rules (scoring and the goals), a copy.
   - `Own Loadout` / `Opponent Loadout`: what each side may do. `Loadout Grants Request Type` answers "can this side place / swap /
     shift?".
   - `Turn Number`, and `Decision Index This Turn`: **0 for the first move of the AI's turn, 1 for the second...** Reset anything you
     remember between moves when it is 0.
   - `Available Request Types`: **the kinds of move the AI may make right now** (its loadout allows them and they are usable this
     decision). Only offer these; `Is Operation Available` checks one operation.
   - `Time Budget Seconds`: how long you have (0 = no limit). When it runs out your best answer so far is used.
3. Pick a move. Useful nodes (all under **ConnectIt**):
   - **Get Placeable Positions (Board State)**: every tile a piece may go on right now.
   - **Simulate Operation**: "what would this move do?" (score gained, lines completed, resulting board) without making it. Pass
     it `Context > Rules` and `Context > Own Slot`.
   - The **Board State** getters: tile owner, multiplier, score, positions of a faction's pieces...
   - **Find Best Moves (MinMax)**: the built-in search as a node -- pass the Context and a `MinMax Search Settings` (depth,
     time, weights, which moves each side has); **On Complete** gives the ranked moves. **Make Decision From Search** turns them
     into a decision (with scores and confidences). Change the settings on the fly (search deeper when losing...).
4. Build the move as a board operation: **Make Instanced Struct**, struct type **Place Piece** (or Swap Pieces, Shift...), and
   set its `Position` (etc.). Leave `Faction` alone: the server stamps the real one.
5. Hand it back: **Make Single Move Decision** (`Operation` = the instanced struct; `Summary` = a line for the log saying why),
   or build a **ConnectIt AI Decision** with several **Candidates** (best first: each a `Payload` + `Summary`, and optionally
   `Confidence` 0..1, `Strategy Score`, `Tags`) so a rejected move falls back to the next. Then call **Finish Decision**.
   **Always call Finish Decision, on every path**, including "nothing good, take the first legal move".
6. Slow work (a **Delay**, a search): call **Publish Best So Far** whenever you have a better answer, so there is something to
   play if the think time runs out first. Check **Is Decision Active** before finishing; a cancelled decision's late answer
   is ignored anyway. Optionally override **Cancel Decision** to stop your own work.
7. Put it in the director: the AI director Blueprint's Class Defaults > **Strategies**, add an entry, pick your Blueprint class.

### C. Make a Blueprint AI controller (only if you need one)

1. Content Browser: **Blueprint Class** with parent `ConnectIt_AIController`.
2. In Class Defaults set **AI Display Name** (the name the opponent shows). You can add components and event-graph logic to it
   (for instance, a "thinking" indicator that starts when its turn does), **but the turn logic itself (timing, deciding,
   submitting) is C++ and cannot be overridden**. `Get Director` returns the controller's own copy of the director.
3. Open the level's **GameMode** Blueprint, Class Defaults, **ConnectIt > Config > AI Controller Class**: pick your controller.
   (The level must actually use that GameMode: World Settings > GameMode Override.)
4. Which opponent it is does not change: the AI director still supplies the loadout, strategies and selector.

### Wire it into the level (all variants)

7. In the **match preset** set **Directors > AI Director Class** to your AI director Blueprint, and **Player Director Class**
   to the human side's (required), and **Level Director Class** (required: it holds both sides' goals). The AI's goals are the
   Level director's **AI Win Conditions**: it never borrows the player's, and none means it can never win (you get a warning).
8. Confirm the map's **World Settings > ConnectIt > Default Match Preset** points at that preset (and **Level Config** at its
   board), and the GameMode's `Match Type` is **Adventure**.
9. Right-click the preset > **Asset Actions > Validate**: it checks the directors too (a starting loadout, at least one strategy,
   a loadout that grants moves the strategies play, the goal lists).

## Verify

- Play in editor and watch the Output Log, filter `ConnectIt_AIController`. A healthy turn reads: `AI director '...'`,
  `seeded action state from loadout '...'`, then each turn `decided -- N candidate(s), best: [<label>] <summary>` and
  `'<request type>' ... -- accepted`.
- Warnings that mean something is not set up: `the match preset has no AI Director Class` (the default director has no
  loadout, so every move is rejected), `no AI loadout`, `no strategy of director ... proposed a move`, `REJECTED`, `decision is stale`,
  `hit the per-turn decision cap`, from the coordinator `think time is up but no strategy has a move yet`, and from the
  GameMode `the Level director has no AI Win Conditions`.

## Pitfalls

- **Never calling Finish Decision** (and never publishing): the AI waits for that strategy until it answers. With other
  strategies in the list, their moves are used when the think time ends; alone, the turn runs to its timer.
- **`REJECTED`**: the AI's loadout has no action that produces that operation type, the action is out of uses / at its per-turn
  cap, or the operation can't apply (occupied tile). The controller then tries the next candidate; if **all** are rejected it
  logs an error and ends the AI's turn. Offer only `Available Request Types` and moves that apply to the board.
- **Two moves per turn** is loadout configuration (the Place action's per-turn requirement / turn-end rule), not strategy: the
  controller simply asks again until the turn can end (at most 16 times). Use `Decision Index This Turn` to tell the moves apart.
- **Strategy variables reset every match** and belong to one AI: each controller makes its own director from the class,
  strategies included.
- **The built-in MinMax only searches Place Piece.** Giving the AI swaps does nothing unless a strategy uses them. Validation
  warns when the loadout grants moves no strategy plays, or none they do (`Get Supported Request Types`).
- **MinMax's think time comes from the director** (`Think Time Budget Seconds` x `Strategy Budget Share`); its own `Default
  Time Budget Seconds` is only for when nothing gives one (e.g. the Find Best Moves node with a bare context). When time
  runs out its deepest finished ranking is used.
- **An AI with no legal move passes**: `Available Request Types` only lists kinds of move that can apply somewhere on the
  board, and with none left the AI ends its turn (`no legal move -- passing` in the log).
- **No AI Win Conditions on the Level director** is the easy one to forget: the AI then plays but cannot win.
- Custom controllers do not change behaviour. If your change "had no effect", you almost certainly wanted the director or a
  strategy.

## See also

- Recipes: [[combine-ai-strategies]], [[direct-the-ai-during-a-match]], [[add-a-win-condition]], [[script-a-level]]
- Flow: [[ai-turn]]
- Design: [three-directors](../../design/three-directors.md), [ai-director-pipeline](../../design/ai-director-pipeline.md),
  [blueprint-ai-strategy-surface](../../design/blueprint-ai-strategy-surface.md) (the exhaustion-AI recipe),
  [stage-and-match-preset](../../design/stage-and-match-preset.md),
  [swap-puzzles](../../design/swap-puzzles.md) (a level that gives the AI its own goal)
- [[add-a-board-request-type]] (if the AI should be able to play a new kind of move)
