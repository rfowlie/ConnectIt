---
schema: recipes
task: Choose an AI's strategies and selector -- several strategies proposing moves at once (e.g. MinMax plus a quick Blueprint fallback) and a selector picking among them, including writing your own selector.
touches:
  - the AI director's Strategies and Selector (on the match preset, or a Blueprint director's defaults)
  - the strategies (any UConnectIt_AIStrategy: MinMax, Blueprint strategies)
  - a Blueprint subclass of UConnectIt_AISelector (only for your own way of picking)
reconciled: 2026-10-19
commit: fd369a4
---

# Recipe: choose strategies and a selector

## Goal

An opponent whose move comes from several strategies at once, picked the way you want. Each decision the **coordinator** (fixed
machinery inside the AI controller) starts every strategy in the director's **Strategies** list, collects their answers until
all have finished or the think time runs out, and hands them to the director's **Selector**, which orders the moves. The
controller then submits them in that order, falling back to the next on a rejected move.

## Prerequisites

- An AI director on the match preset ([[set-up-a-custom-ai-opponent]]).
- The strategies you want, each working on its own.
- Know that a strategy should only offer moves from `Context > Available Request Types`: a rejected candidate just falls
  through to the next.

## Steps

1. Open the director (the match preset's **AI Director**, or your Blueprint director's Class Defaults).
2. **Strategies**: add them, **highest priority first**, each configured inline (e.g. 1: MinMax (Classic), 2: your Blueprint
   "take any free tile" strategy). Give two of the same class different **Labels** so logs tell them apart.
3. Think time: **Think Time Budget Seconds** (how long the strategies may think; 0 = wait for all of them), **Strategy Budget
   Share** (the share each strategy is told it has, `Context > Time Budget Seconds`; MinMax stops searching there).
4. **Selector**: pick one of the stock selectors below, or your own (step 6).
5. **Max Candidates** (on each strategy and on the selector): how many moves are kept. Keep the selector's at least as high as
   the fallbacks you want.
6. Your own selector: **Blueprint Class** with parent `ConnectIt_AISelector`, override **Rank Candidates**. It gets the Context
   and the **endorsed candidates**: every move of every strategy, once each, in priority order (strategy order, then each
   strategy's own order), each with `Candidate` (payload, summary, confidence, strategy score, tags, source label),
   `Endorsements` (how many strategies proposed it) and `First Strategy Index`. Return the candidates you'd play, best first.
   Pick it as the director's Selector.

### The stock selectors

| Selector | Picks | Settings |
|---|---|---|
| **First Available** (default) | The top-priority strategy's best move; the rest as fallbacks | -- |
| **Top N (Mistake Chance)** | Usually the best; with `Mistake Chance`, one of ranks 2..N instead | `Top Moves Considered`, `Mistake Chance` |
| **Greedy Scorer** | The move gaining the AI the most points right now (simulates each candidate). Comparable across very different strategies; blind beyond this move | -- |
| **Weighted by Confidence** | At random, better moves likelier: weight exp(confidence / temperature) | `Temperature`, `Missing Confidence` (for candidates without one) |

### Other ideas for your own selector

The fuller list, with what each needs, is [ai-selector-ideas](../../design/ai-selector-ideas.md). Quick ones:

- **Consensus**: sort by `Endorsements`, most first.
- **Win if you can**: **Simulate Operation** each candidate (Board = `Context > Board`, Rules = `Context > Rules`, Faction =
  `Context > Own Slot`); any whose result board is won by you goes first.
- **Style / tell-driven**: prefer candidates whose `Tags` match a style your director chose this turn.
- **Rubber band**: strongest move when behind (`Get Faction Score (Board State)` on `Context > Board`), a weaker one when far
  ahead.

## Verify

- Output Log, filter `ConnectIt_AIController`: `decided -- N candidate(s), best: [Label] ...` -- the bracket says which strategy
  the chosen move came from.
- Filter `ConnectIt_AICoordinator`: `out of think time -- using the best so far of: ...` names strategies that were cut off;
  their partial answers (MinMax's deepest finished ranking) were still used.

## Pitfalls

- **No strategies** = no move (logged as an error; validation catches it).
- **A strategy's own budget longer than the director's** is pointless: it is cut off when the think time ends. Use the share.
- **If nothing has a move when the think time runs out**, the coordinator keeps waiting for the first answer (one warning
  naming the slow strategies) -- a strategy that never finishes or publishes can still stall the turn if it's the only one.
- **Strategies and selectors must not have side effects** (changing loadouts, playing sounds): most answers are discarded.
  Things that change the AI belong to the director ([[direct-the-ai-during-a-match]]).
- **Confidence is only as good as each strategy's calibration**: MinMax derives it from its ranking; your strategies set their
  own (or none). Outcome-based selectors (Greedy Scorer) judge for themselves.
- Repeats are merged by exact operation match; two strategies offering the same tile is one candidate with 2 endorsements.

## See also

- Flow: [[ai-turn]]
- Design: [ai-director-pipeline](../../design/ai-director-pipeline.md), [ai-selector-ideas](../../design/ai-selector-ideas.md)
- Recipes: [[set-up-a-custom-ai-opponent]], [[direct-the-ai-during-a-match]]
