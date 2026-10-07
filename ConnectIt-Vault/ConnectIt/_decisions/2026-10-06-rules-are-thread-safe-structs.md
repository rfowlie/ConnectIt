---
Date: 2026-10-06
status: Active
superseded by:
tags:
  - rules
  - ai
  - architecture
---
## Decision

The match's rules are **plain, thread-safe polymorphic C++ structs**, not UObject interfaces:
`FConnectItScoringRule` (`_Lines`), `FConnectItWinCondition` (`_ScoreThreshold`), `FConnectItTilePlaceableRule`
(`_Unoccupied`), held as `TInstancedStruct`s in one **`FConnectItRuleSet`**. The level config holds the template
(`Rules`), the GameMode plays each match on its own value copy (per-match changes such as the menu's target score go
there), the Mediator calls it, and the AI's search copies it and calls the *same* rule code.
`IConnectIt_ScoringRule`, `IConnectIt_WinCondition`, `IConnectIt_TilePlaceableRule`, their UObject classes,
`UConnectIt_BoardRules` and the search-only bridges (`FConnectItWinCheck` / `MakeSearchWinCheck`, static
`ApplyLineScoring`, `GetMinimumConnectLength`) are gone.

The win condition base is generic: `GetWinningFaction` and `GetProgress` (0-1 per faction, stamped into replicated
`FConnectItBoardState::WinProgress`); score-based conditions additionally answer `GetTargetScore` / `SetTargetScore`.
Nothing line-specific is on the scoring base -- line-only AI pieces ask the rule set whether scoring is
`FConnectItScoringRule_Lines`.

## Why

The rules existed twice: UObject interfaces with BlueprintNativeEvents for the real game (game thread only), and
thread-safe copies bolted on for the AI's background search -- each new rule needed a wrapper and could drift. The
interfaces also baked in "win = reach a score" and "scoring = straight lines". One implementation that is thread-safe
by construction removes the wrappers and makes any future rule automatically visible to the AI.

**Cost accepted:** rule logic can no longer be written in Blueprint (designers still pick and configure rules in the
Details panel; new kinds of rule are C++ structs).

## What Would Change It

A real need for Blueprint-authored rules (they would have to stay game-thread-only and invisible to the search), or
Phase B (moves and the pipeline as shared thread-safe types -- see
[rules-as-structs-and-shared-simulation](../design/rules-as-structs-and-shared-simulation.md)) changing what a rule is
handed.

Supersedes the mechanism in
[2026-09-24-minmax-calls-real-rules-as-static-thread-safe-functions](2026-09-24-minmax-calls-real-rules-as-static-thread-safe-functions.md)
and [2026-10-04-minmax-win-detection-from-win-condition](2026-10-04-minmax-win-detection-from-win-condition.md)
(their intent -- the search uses the game's real rules -- stands).
