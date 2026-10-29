---
Date: 2026-10-20
status: Active
superseded by:
tags:
  - ai
  - architecture
  - designer-tools
  - win-conditions
summary: 'Built: MinMax default time budget; "match condition" rename; conditions get a context; Not Enough Pieces To Score, No Legal Move; per-move legality, AI pass. Designed: Level / Player / AI directors owning their conditions (authored in directors, evaluated in the rules), lose conditions'
---
## Decision

Settled with the owner on 2026-10-20 after setting up the first match presets. Part 1 is built
([design/conditions-and-legal-moves](../design/conditions-and-legal-moves.md)); part 2 is designed, not built
([design/three-directors](../design/three-directors.md)).

**Built (part 1):**

- **MinMax's own time budget is a default** (`DefaultTimeBudgetSeconds`): the decision's budget (the director's think time
  share) wins whenever set. Its request-type settings stay (a ban or an unusable move may need them).
- **Conditions are "match conditions"**, not win conditions: the types were renamed (`FConnectItMatchCondition`,
  `UConnectIt_MatchConditionAsset`...) with redirects, because the same building block serves goals, losses and level
  triggers. Condition assets stay (they force per-level organisation and give the director something to compare).
- **Conditions are asked with a context** (board + the match's rules), so a condition can read the rules (e.g. the scoring
  rule's minimum pieces). New conditions: **Not Enough Pieces To Score**, **No Legal Move: X** (one condition with a Move
  picker, board-only -- not the loadout or uses left).
- **Each kind of move answers "any legal one on this board?"** (Place and Swap so far; others Unknown = assume possible). It
  drives No Legal Move, a Blueprint helper for greying out actions, the AI's `Available Request Types`, and an **AI pass**
  when nothing is legal. Temporary unavailability is a pass, not a loss.
- **Conditions are a game-rules tool and aren't limited by what MinMax can model**; the AI may need other strategies as
  levels get more varied, which the director pipeline already allows.

**Designed (part 2):**

- **Three directors** -- Level, Player, AI -- on a shared base: the same events, their own condition lists, a broadcast when a
  condition is met. The match preset names the three classes; each director's configuration lives in its Blueprint's Class
  Defaults (the inline director in the preset was clunky and risked being silently overwritten by the Blueprint).
- **Conditions are authored in the directors, evaluated in the rules**: copied into the rule set at match start, so the
  server, the AI's search and clients evaluate them as now. Each director owns its context's conditions (Player: the
  player's win / lose; AI: the AI's; Level: triggers that fire once and can be re-armed).
- **Lose conditions**: any one holding = that side loses; win goals are checked first; a Lose Condition Met event, then Game
  Won for the other side; an Opponent Lost match-end reason; the search treats a loss as game over; online undecided.

## Why

Setting up the presets showed the inline director cluttering the preset and the risk of two places configuring one
director. Losing (a stuck exhaustion-level player) needed conditions that see more than the board and an "or" that ordered
goals can't express. A Level and a Player director give level scripting and side-specific reactions the same home the AI
already has, and keeping evaluation in the rule set keeps the thread-safe, replicated path unchanged.

## What Would Change It

A director needing to evaluate conditions the rule set can't hold (UObject state) -- that would need server-only conditions
the search can't see; versus play needing lose conditions (see
[lose-conditions-online](../_questions/lose-conditions-online.md)); a level needing "out of uses" as a condition
([out-of-uses-condition](../_questions/out-of-uses-condition.md)).

Partly supersedes [2026-10-16-ai-director-pipeline](2026-10-16-ai-director-pipeline.md) (the AI director edited inline in
the match preset; win conditions authored in the preset's rules).
