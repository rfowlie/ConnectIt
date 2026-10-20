---
Date: 2026-09-24
status: Superseded (mechanism)
superseded by: 2026-10-06-rules-are-thread-safe-structs
tags:
  - ai
  - minmax
  - scoring
  - architecture
---
## Decision

> **Mechanism superseded 2026-10-06:** rules are now thread-safe structs the game and the search share
> directly -- see [2026-10-06-rules-are-thread-safe-structs](2026-10-06-rules-are-thread-safe-structs.md). The intent here stands.

Classic MinMax's search node wraps a real `FConnectItBoardState` and simulates each candidate
move by calling `UConnectIt_LineScoringRule::ApplyLineScoring` -- a new `public static` function
holding exactly the algorithm `ApplyScoring_Implementation` already ran, extracted so it takes
`ConnectLength` as a parameter instead of reading the instance member. The search calls this
static function directly, never through `IConnectIt_ScoringRule`'s `Execute_ApplyScoring`/
`BlueprintNativeEvent` dispatch.

`IConnectIt_ScoringRule` also gained `GetMinimumConnectLength()` (`BlueprintNativeEvent`,
default 0), implemented by `UConnectIt_LineScoringRule` to return `ConnectLength`, wrapped by
`UConnectIt_BoardRules::GetMinimumConnectLength()` -- mirrors `GetTargetScore()`'s existing role
on the win-condition interface. The win threshold and connect length are both read once, on the
game thread, before a search dispatches; no equivalent extraction was needed for the win
condition or the tile-placeable check -- see Why.

## Why

`UConnectIt_MinMaxTreeBuilder::BuildTreeAsync`/`SolveTreeAsync` run on background tasks
(`UE::Tasks::Launch`) so the search doesn't block the game thread. `ScoringRule`/
`WinConditionRule`/`TilePlaceableRule` are `Instanced` slots that could be a Blueprint-authored
override, and their interface methods are `BlueprintNativeEvent` -- calling one off the game
thread risks running Blueprint VM code on a non-game thread, which Unreal doesn't support.
Extracting the scoring algorithm to a plain static function sidesteps this entirely (no UObject,
no interface dispatch inside the hot recursive path) while staying literally the same algorithm
the real rule runs, not a second reimplementation that can drift -- the actual bug class this
whole rebuild exists to fix (see
[the scope decision](2026-09-24-minmax-scoped-to-classic-adventure-gets-bespoke-ai.md)'s context:
the old code hand-rolled scoring with a hardcoded connect-length of 4 and a hardcoded win-score
constant, both wrong for any level whose config differs from the default).

The win condition and tile-placeable check needed no equivalent extraction:
`UConnectIt_ScoreThresholdWinCondition`'s whole check is one threshold compare against
`ScoreBoard`, read once as a plain float (`GetTargetScore`, already game-thread-only,
already existing) and captured by value into the search's `IsGameOver`/`Evaluate` lambdas --
no need to run the check itself off-thread. `FConnectItBoardState::IsTileValidForPlacement` is
already a plain, thread-safe struct method (no UObject involved at all), so `GenerateChildren`
calls it directly with no change needed.

## What Would Change It

If a Classic-mode level ever needs a Blueprint-authored scoring rule, this stops applying: the
search would silently disagree with that rule's real behavior (falls back to `ConnectLength = 4`
with a logged warning -- see
[the scope decision](2026-09-24-minmax-scoped-to-classic-adventure-gets-bespoke-ai.md)). Accepted
for this pass; not solved.
