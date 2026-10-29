---
Date: 2026-10-16
status: Partly superseded
superseded by: "[2026-10-20-three-directors-and-match-conditions](2026-10-20-three-directors-and-match-conditions.md) (the AI director becomes one of three directors named by class in the preset; conditions authored in directors; MinMax budget is a default)"
tags:
  - ai
  - architecture
  - designer-tools
  - win-conditions
summary: 'The AI director owns the AI setup (loadout, strategies, selector, think times) on the match preset; the AI profile is removed; the coordinator is a fixed pipeline pulling strategies'' best-so-far answers at the deadline; selectors pick (mistake chance lives there); win conditions are data assets copied out at match start'
---
## Decision

Settled with the owner in a back-and-forth on the composable AI built two days earlier, and built in stages 2026-10-16..19
([design/ai-director-pipeline](../design/ai-director-pipeline.md)):

- **The AI director owns the AI's setup**: starting loadout, strategies (instanced, list order = priority), selector, think
  times. One director per adventure level, or per way of playing against the AI. It lives on the match preset; a "mode" is a
  preset naming its director. Built-in events (match start, AI turn start / end, opponent turn start, checkpoints with the goal
  asset) are conveniences; it has world access for anything else. It reacts to committed state, never to visual event tags.
- **The AI profile is removed.** Anything it held, a director holds; the menu picks presets.
- **The coordinator is not a strategy**: one fixed pipeline inside the controller. It starts every strategy at once, polls (no
  delegates), concludes when all have finished or at the think time with each strategy's **best answer so far**, waits for at
  least one move (with a warning naming the slow strategies), stops the rest, and runs the selector.
- **Strategies only decide** and keep a best-so-far answer (`PublishBestSoFar` / `GetBestSoFar`). MinMax publishes each
  completed search depth through a thread-safe progress holder.
- **Candidates carry optional fields**: confidence (0..1, comparable across strategies), strategy score (raw), tags, source label.
- **Selectors pick**: an instanced class on the director, swappable during a match. Decisions only (no strategy objects); a
  built-in endorsement step merges duplicates and counts them. **Mistake chance is a selector feature** (Top N), not a strategy
  setting. First four: First Available (default), Top N, Greedy Scorer, Weighted by Confidence.
- **The controller still submits**: candidates in order until one is accepted; all rejected = error and end the turn.
- **Win conditions are data assets** wrapping the existing thread-safe condition structs; a preset's rules list goal assets per
  side, and the match copies the conditions out at start (`ResolveWinGroups`). Checkpoints carry the asset reached. A variant
  is another asset.

## Why

The composable AI put too many jobs in the wrong places: the coordinator was a strategy (so it had a strategy's contract but
did collection and judging), mistake chance sat on MinMax (and would double up under a coordinator), and "who the opponent is"
was split between a profile asset and a director that could change it. Giving the director the setup, the coordinator a fixed
pipeline and the selector the choice gives each class one job, keeps designers out of the controller, and lets a per-level
director script the AI. Best-so-far answers make the think time a real deadline instead of a cancel. Goals as assets give them
an identity that survives reordering, and reuse across presets.

## What Would Change It

Selectors needing to know which strategy proposed a move (beyond the label); a menu needing arbitrary director settings (see
[design/match-menu-options](../design/match-menu-options.md)); a strategy that can't express a best-so-far answer and must not
be cut off; goals that need per-preset tweaks often enough that sharing assets hurts.

Supersedes parts of [2026-10-14-composable-ai](2026-10-14-composable-ai.md) (coordinator as a strategy, MinMax mistake chance,
director changing only loadout / strategy) and [2026-10-02-ai-opponent-is-a-profile-asset](2026-10-02-ai-opponent-is-a-profile-asset.md);
changes how [2026-10-13-win-condition-groups-as-ordered-stages](2026-10-13-win-condition-groups-as-ordered-stages.md) is authored
(goal assets instead of inline groups; ordering, checkpoints and the win are unchanged).
