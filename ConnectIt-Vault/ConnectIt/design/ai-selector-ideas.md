# AI selector ideas

A catalogue of ways an AI could pick its move from its strategies' answers, kept for design consideration. Recorded from the
2026-10-16 discussion that settled the director pipeline ([ai-director-pipeline](ai-director-pipeline.md)). Four were built
first (marked **built**); the rest are ideas, nothing more.

A selector sees **decisions only**: every strategy's candidates merged in priority order, each with how many strategies proposed
it (endorsements) and the candidate's optional fields (confidence 0..1, strategy score, tags, source label). It never sees
the strategy objects. How to write one: [[combine-ai-strategies]].

## Order and chance (no outcome needed)

- **First available** (**built**, the default): candidates in the order handed over -- the top-priority strategy's best.
- **Top N with mistake chance** (**built**): usually the best; with a chance, one of ranks 2..N. The difficulty knob that moved
  off MinMax (putting it on the selector avoids a double mistake when it lived on a strategy and on whatever combined them).
- **Weighted by confidence** (**built**): random, better moves likelier, weight exp(confidence / temperature). A temperature
  gives smooth difficulty from always-best to anything-goes. Only as good as the strategies' confidence calibration.

## Immediate outcome (one Simulate Operation per candidate)

- **Greedy scorer** (**built**): most points for the AI right now (its own score change, not the operation's raw points).
- **Win if you can**: any candidate that wins now goes first. Could be an option layered on every selector rather than its own.
- **Most lines**: most lines completed (`Num Scored Events`), regardless of points.
- **Setting up**: prefers moves after which the AI has a completable line next turn.

## Looking at the reply (costlier: simulate the opponent's best answer to each candidate)

- **Don't give it away**: rejects candidates after which the opponent can score or win immediately.
- **Block first**: prefers candidates that remove the opponent's completable lines.

## Context-aware

- **Rubber band**: strongest move when behind, a weaker one when far ahead. Adaptive difficulty without touching the strategies.
- **Style / tell-driven**: prefers candidates tagged with a style the director asked for this turn ("swap-heavy turn"). Needs
  strategies that set `Tags`.
- **Anti-repeat**: avoids the same region or move type as last turn; the selector keeps a little memory.
- **Consensus**: prefers candidates with the most endorsements.

## A possible generalisation (not built)

Almost every idea above is **"measure each candidate, then pick by a policy"**: a measure (points, lines, threat, confidence,
endorsements) plus a pick (best, top N with chance, weighted random). That suggests one base selector combining **weighted
measures plus a pick policy**, the same shape as the MinMax weights. Decision for now: ship concrete selectors and only
generalise if the same thing keeps getting written.

## Open

- Whether `Tags` on candidates earn their keep: "prefer blocking" may be easier to tune than confidence calibrated across
  strategies, but only if strategies tag consistently.
