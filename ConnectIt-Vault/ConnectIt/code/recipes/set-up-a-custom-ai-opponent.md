---
schema: recipes
task: Give a level an AI opponent that plays differently -- a tuned built-in one, a Blueprint-made strategy, or a Blueprint AI controller -- and wire it into a match preset.
touches:
  - a UConnectIt_AIProfile asset (loadout + strategy)
  - a Blueprint subclass of UConnectIt_AIStrategy (only for a bespoke strategy)
  - a Blueprint subclass of AConnectIt_AIController (only if you need one; see step 4)
  - the match preset (AI Profile, AI Win Groups), the level config (Default Match Preset), the level's GameMode (AI Controller Class)
reconciled: 2026-10-14
commit: ee45a98
---

# Recipe: set up a custom AI opponent

## Goal

A level whose opponent behaves the way you designed, for designers working in the editor. Read
[[ai-turn]] first if you want the flow behind it; the short version is: **the profile says who the opponent is, the strategy
picks its moves, the controller only does the turn timing and submitting.**

## First, which of these do you actually need?

| I want... | Do this | Section |
|---|---|---|
| An easier / harder / more mistake-prone classic opponent | A new **AI profile** using the MinMax strategy, different numbers | A |
| An opponent with its own rules of thumb ("two random pieces per turn, only one may score") | A **Blueprint strategy** + a profile that uses it | B |
| The opponent to swap or shift pieces | A **Blueprint strategy** (the built-in MinMax only places) + a loadout with those actions | B |
| A different *thinking animation / extra component* on the opponent actor | A **Blueprint AI controller** | C |
| To change how the AI times its turn, retries, or submits | Not possible in Blueprint today: ask for a C++ hook | C |

Most levels need A or B. **You rarely need a custom controller**: its decisions are not overridable, and which opponent plays
comes from the profile, not from the controller class.

## Prerequisites

- A level with a match preset that is played as **Adventure** (the GameMode's `Match Type` is Adventure, which is what spawns the AI).
- An **action loadout** for the AI (an asset like a player's): the AI may only do what its loadout's actions allow, and the server
  holds it to that exactly like a human. A Place Piece action to place, a Swap action to swap, and so on, with the per-turn
  limits and turn-end requirements you want.

## Steps

### A. Tune the built-in opponent (no Blueprint)

1. Content Browser: create a **Data Asset** of class `ConnectIt_AIProfile` (e.g. `AIP_Classic_Hard`).
2. Fill it in: `Display Name` / `Description` (shown in menus), `Loadout` (the AI's action loadout), and `Strategy`: pick
   **MinMax (Classic)**.
3. Set the MinMax numbers: `Max Depth` (how many plies it looks ahead), `Time Budget Seconds` (stops deepening when time runs
   out), `Top Moves Considered` + `Mistake Chance` (how often it plays a deliberately weaker move), `Min Think Seconds`
   (it never moves sooner than this), and the `Evaluation Weights` / `Ordering Weights` (what it values; ordering is speed only).
4. Go to step 5.

### B. Make a Blueprint strategy

1. Content Browser: **Blueprint Class** with parent `ConnectIt_AIStrategy` (e.g. `BPAIS_TwoRandomPieces`). Set `Min Think
   Seconds` in Class Defaults.
2. Open **Functions > Override > Begin Decision**. It receives the **Context**:
   - `Board`: a copy of the current board state; `Own Slot`: your faction slot.
   - `Rules`: the match's rules (scoring and win groups), a copy.
   - `Own Loadout` / `Opponent Loadout`: what each side may do. `Loadout Grants Request Type` answers "can this side place / swap /
     shift?".
   - `Turn Number`, and `Decision Index This Turn`: **0 for the first move of the AI's turn, 1 for the second...** Reset anything you
     remember between moves when it is 0.
3. Pick a move. Useful nodes (all under **ConnectIt**):
   - **Get Placeable Positions (Board State)**: every tile a piece may go on right now.
   - **Simulate Operation**: "what would this move do?" (score gained, lines completed, resulting board) without making it. Pass
     it `Context > Rules` and `Context > Own Slot`.
   - The **Board State** getters: tile owner, multiplier, score, positions of a faction's pieces...
4. Build the move as a board operation: **Make Instanced Struct**, struct type **Place Piece** (or Swap Pieces, Shift...), and
   set its `Position` (etc.). Leave `Faction` alone: the server stamps the real one.
5. Hand it back: make a **ConnectIt AI Decision** struct (`Payload` = the instanced struct; `Summary` = a line for the log saying
   why), then call **Finish Decision** with it. **Always call Finish Decision, on every path**, including "nothing good, take the first legal move".
6. If you use a **Delay** or other latent work, check **Is Decision Active** before finishing; a cancelled decision's late answer
   is ignored anyway. Optionally override **Cancel Decision** to stop your own work.
7. Create the profile as in A, but pick your Blueprint class as the `Strategy`.

### C. Make a Blueprint AI controller (only if you need one)

1. Content Browser: **Blueprint Class** with parent `ConnectIt_AIController`.
2. In Class Defaults set **AI Display Name** (the name the opponent shows). You can add components and event-graph logic to it
   (for instance, a "thinking" indicator that starts when its turn does), **but the turn logic itself (timing, deciding,
   submitting) is C++ and cannot be overridden**. `Get Strategy` returns the controller's own copy of the strategy if you need to
   read it.
3. Open the level's **GameMode** Blueprint, Class Defaults, **ConnectIt > Config > AI Controller Class**: pick your controller.
   (The level must actually use that GameMode: World Settings > GameMode Override.)
4. Which opponent it is does not change: the profile (step 5) still supplies the loadout and the strategy.

### Wire it into the level (all variants)

5. Open the **match preset** the level plays as (the level config's `Default Match Preset`; menus can offer others) and set:
   - **AI Profile**: the profile from A or B (the menu's own AI choice overrides it, if the menu offers one).
   - **Rules > AI Win Groups**: **the AI's goals**, in order. Set them: the AI never borrows the player's, and a preset with none
     means the AI can never win (you get a warning). Also check **Player Win Groups** and **Player Loadout**.
6. Confirm the level config's `Default Match Preset` points at that preset, and the GameMode's `Match Type` is **Adventure**.

## Verify

- Play in editor and watch the Output Log, filter `ConnectIt_AIController`. A healthy turn reads: `using AI profile '...'`,
  `seeded action state from loadout '...'`, then each turn `decided -- <your Summary>` and `'<request type>' as <action> --
  accepted`.
- Warnings that mean something is not set up: `no AI profile`, `AI profile has no Strategy` (it falls back to a default MinMax),
  `strategy ... returned no move`, `REJECTED`, `decision is stale`, `hit the per-turn decision cap`, and from the GameMode
  `no AI Win Groups`.

## Pitfalls

- **Never calling Finish Decision** (or finishing with an empty payload): the AI sits there until the turn timer runs out.
- **`REJECTED`**: the AI's loadout has no action that produces that operation type, the action is out of uses / at its per-turn
  cap, or the operation can't apply (occupied tile). The AI does not retry. Check the loadout first.
- **Two moves per turn** is loadout configuration (the Place action's per-turn requirement / turn-end rule), not strategy: the
  controller simply asks again until the turn can end (at most 16 times). Use `Decision Index This Turn` to tell the moves apart.
- **Strategy variables reset every match** and belong to one AI: each controller works on its own duplicate of the profile's
  strategy.
- **The built-in MinMax only searches Place Piece.** Giving the AI swaps does nothing unless a strategy uses them.
- **No AI Win Groups** is the easy one to forget in a new preset: the AI then plays but cannot win.
- Custom controllers do not change behaviour. If your change "had no effect", you almost certainly wanted a strategy.
- Level scripting in Blueprint (a level script reacting to checkpoints) is separate: see
  [win-condition-stages](../../design/win-condition-stages.md).

## See also

- Flow: [[ai-turn]]
- Design: [blueprint-ai-strategy-surface](../../design/blueprint-ai-strategy-surface.md) (the exhaustion-AI recipe),
  [stage-and-match-preset](../../design/stage-and-match-preset.md),
  [swap-puzzles](../../design/swap-puzzles.md) (a level that gives the AI its own goal)
- [[add-a-board-request-type]] (if the AI should be able to play a new kind of move)
