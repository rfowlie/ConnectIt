---
schema: recipes
task: Make an AI opponent out of several strategies -- e.g. MinMax plus a quick Blueprint fallback -- with a coordinator that collects their moves within a time budget and picks among them.
touches:
  - a UConnectIt_AIStrategy_Coordinator, configured inline on an AI profile (or a Blueprint subclass of it, to change how it picks)
  - the child strategies (any UConnectIt_AIStrategy: MinMax, Blueprint strategies, other coordinators)
  - Source/ConnectIt/Public/AI/ConnectIt_AIStrategy_Coordinator.h (RankCandidates, the judge)
reconciled: 2026-10-14
commit: 5262fc9
---

# Recipe: combine AI strategies

## Goal

An opponent whose move comes from several strategies at once. A **coordinator** is itself a strategy: it asks every child,
collects their candidate lists until its time budget runs out (or all have answered), ranks all candidates with its **judge**
(`RankCandidates`), and answers with the best. Because it is a strategy, coordinators can contain coordinators, and the AI
controller doesn't know the difference.

## Prerequisites

- The children you want, each working on its own (see [[set-up-a-custom-ai-opponent]]).
- An AI profile to put the coordinator on.
- Know that a child should only offer moves from `Context > Available Request Types`: the controller tries the coordinator's
  candidates in order, and a rejected one just falls through to the next.

## Steps

1. Open the AI profile. **Strategy**: pick **Coordinator**.
2. **Children**: add the strategies, **highest priority first**, each configured inline (e.g. 1: MinMax (Classic), 2: your
   Blueprint "take any free tile" strategy).
3. **Time Budget Seconds**: how long to collect answers (0 = wait for every child). **Child Budget Share**: the share of it each
   child is told it has (`Context > Time Budget Seconds`); MinMax stops searching when its share runs out, leaving the rest for
   the judge.
4. **Max Candidates** (on every strategy): how many moves are kept. Keep the coordinator's at least as high as you want
   fallbacks.
5. That's the default judge: **child priority order** -- the first child's candidates first, then the next child's, a move
   already offered skipped. To judge differently, make a **Blueprint Class** with parent `ConnectIt_AIStrategy_Coordinator`
   and override **Rank Candidates** (it receives the context and one entry per child: `Child Index`, `Child`, `Answered`,
   `Decision`; it returns one Decision, best first). Examples below.

### Other judges (override Rank Candidates)

- **Weighted random** -- an unpredictable opponent. Give your coordinator subclass an array of weights (one per child). In
  Rank Candidates: pick a child at random by weight among those that answered with a move; put that child's candidates first,
  then everyone else's in priority order (so a rejected move still has fallbacks).
- **Re-score by simulation** -- comparable across very different children. For every candidate of every child, call
  **Simulate Operation** (Board = `Context > Board`, Rules = `Context > Rules`, Faction = `Context > Own Slot`). Score it
  your own way -- e.g. `Points Scored`, or `Num Scored Events` x 100 plus the change in your score minus the opponent's -- and
  return all candidates sorted by that score. Costs one simulation per candidate: cheap for a handful, not for hundreds.
- **First good enough** -- e.g. "MinMax unless it found nothing better than X, then the aggressive child": read each child's
  `Decision > Candidates` (summaries carry MinMax's score) and choose which child's list goes first.
- **Phase by state** -- e.g. defensive children first when the AI is ahead (`Get Faction Score (Board State)` on
  `Context > Board`), aggressive first when behind.

## Verify

- Output Log, filter `ConnectIt_AIController`: `decided -- N candidate(s), best: [ChildName] ...` -- the bracket says which child
  the chosen move came from.
- A slow child (MinMax with a long budget) is cut off when the coordinator's budget runs out: its moves are then missing from the
  list, but the turn still happens on time.

## Pitfalls

- **No children** = no move (logged as an error).
- **A child's own budget longer than the coordinator's** is pointless: it is cancelled when the coordinator concludes. Use the
  share instead.
- **If nothing has answered when the budget runs out**, the coordinator keeps waiting for the first answer -- a child that never
  calls Finish Decision can still stall the turn.
- **Children must not have side effects** (changing loadouts, playing sounds): the coordinator asks all of them and discards
  most answers. Things that change the AI belong to the match's AI director ([[direct-the-ai-during-a-match]]).
- Repeats are removed by exact operation match; two children offering the same tile is one candidate.

## See also

- Flow: [[ai-turn]]
- Design: [composable-ai](../../design/composable-ai.md)
- Recipes: [[set-up-a-custom-ai-opponent]], [[direct-the-ai-during-a-match]]
