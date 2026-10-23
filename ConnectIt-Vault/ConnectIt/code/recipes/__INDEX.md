---
schema: recipes
reconciled: 2026-10-19
commit: fd369a4
---

# game (ConnectIt module) — recipes index

Step-by-step task procedures. Governed by [[_core/_schema/_code|_core/_schema/_code.md]].

| Task | When you'd do it | Status |
|---|---|---|
| [[add-a-scoring-rule\|add-a-scoring-rule]] | Change how a placement scores (a new `IConnectIt_ScoringRule`) | current |
| [[set-up-a-custom-ai-opponent\|set-up-a-custom-ai-opponent]] | Designer-facing: the AI opponent as an AI director Blueprint named by the match preset -- tune the built-in one, make a Blueprint strategy, or a Blueprint AI controller | current |
| [[combine-ai-strategies\|combine-ai-strategies]] | Choose an AI's strategies and selector: several strategies at once, the stock selectors (First Available, Top N, Greedy Scorer, Weighted by Confidence), your own selector | current |
| [[direct-the-ai-during-a-match\|direct-the-ai-during-a-match]] | Change the AI's loadout / strategies / selector during a match from the AI director's events (turns, checkpoints, anything), incl. the tell flow | current |
| [[add-a-win-condition\|add-a-win-condition]] | Set up match conditions: goals and ways to lose as condition assets on the Level director (both sides), the condition types (incl. Not Enough Pieces To Score, No Legal Move), reacting; a new condition type or legal-move support in C++ | current |
| [[script-a-level\|script-a-level]] | The Level director: both sides' goals, triggers, board operations, scoring-rule and goal changes on the level's behalf (with timing), working with the other directors | current |
| [[script-a-phased-level\|script-a-phased-level]] | A level in phases: layouts made with the layout tool, Begin Phase / Set Win Conditions / Set Layout at a checkpoint (in that order), other directors on On Phase Changed, the pitfalls | current |
| [[add-a-tile-power\|add-a-tile-power]] | A kind of tile with a power: an effect struct (C++), a `ConnectIt.TilePower.*` tag, a tile catalog entry (tile class + power: trigger set, Turn Start countdown, multiplier floor and ceiling), tile hooks that call Finish, builder tiles in the level; level-wide on the Level director | current |
| [[lock-a-tile-or-chain-tiles\|lock-a-tile-or-chain-tiles]] | Lock a tile (in play, no placing) from a builder tile, a layout or the Level director; chain a tile to a partner (a Sync power); which operations reach locked tiles (Move Piece and Adjust Multiplier don't) | current |
| [[add-a-board-request-type\|add-a-board-request-type]] | Add a new server-validated board mutation (destroy multiplier, swap, capture, …) | current |

Candidate recipes not yet written (`stub`): add a game state-machine phase, wire a new
debug widget, add an influence map for the AI, finish the
game-event queue.

## Related

- Types: [[ConnectIt/code/__INDEX|code/__INDEX.md]]
- Flows: [[ConnectIt/code/systems/__INDEX|systems/__INDEX.md]]
