# Classic MinMax rebuild

Design + implementation note (2026-09-24), design/ is schema-less. Scoped to **Classic
ConnectIt** (place-piece-only, per the
[scope decision](../_decisions/2026-09-24-minmax-scoped-to-classic-adventure-gets-bespoke-ai.md));
Adventure mode's own AI is a deliberately separate, still-open question (see Adventure mode
below). Implemented same session, not just planned.

## The problem (audit findings)

`AConnectIt_AIController` did nothing end to end: `BeginMakeDecision` (its intended decision
hook) was never called from anywhere, in C++ or Blueprint, and there was no code path from
"MinMax found a good move" to an actual `FTurnActionRequest`. The MinMax code itself had two
diverged node models (`ConnectIt::FMinMaxNode`/`UConnectIt_MinMaxManager` and
`FConnectItMinMaxNode`/`UConnectIt_MinMaxTreeBuilder`) that disagreed even on their own
"empty tile" sentinel (`-1` vs `0`, contradicting the newer one's own doc comment), both
duplicated `UConnectIt_LineScoringRule`'s scoring via `UGridMechanics_ShapeLibrary::GetLongestLines`
with a hardcoded connect-length of 4 (ignoring the level's real, designer-configurable
`ConnectLength`), duplicated the win check against a hardcoded `ConnectIt_Score_Max = 100`
instead of the real `IConnectIt_WinCondition`, and the older model's move-order heuristic
hardcoded a 7×7 board. A 310-line fully-commented-out file (`ConcreteMinMaxExample.h`) was a
prior, abandoned attempt at unifying the two node models.

## What was built

> **Partly superseded 2026-10-02** by [the search redesign](classic-minmax-search-redesign.md): the stored-tree
> build/solve, the node model's faction convention and the evaluator below were found wrong (off-by-one on whose move
> the root children are, alternating evaluation perspective) and are replaced. The scoring extraction, request bridge,
> turn hook, turn-end loop and dead-code removal stand.

- **One node model**, wrapping a real `FConnectItBoardState` (not a parallel hand-rolled tile
  map) -- `FConnectItMinMaxNode` in `ConnectIt_MinMaxTreeBuilder.h`. `GenerateChildren()`
  enumerates every currently-placeable tile (`FConnectItBoardState::IsTileValidForPlacement`,
  already a plain thread-safe struct method) -- Classic has exactly one action, so there's no
  action-type branching at all, which is the direct payoff of scoping to Classic.
- **Real scoring, thread-safely.** `UConnectIt_LineScoringRule::ApplyScoring_Implementation`'s
  algorithm was extracted to a `public static ApplyLineScoring(...)`, callable with no UObject
  involved -- see
  [the thread-safety decision](../_decisions/2026-09-24-minmax-calls-real-rules-as-static-thread-safe-functions.md)
  for why (the search runs on a background task; a `BlueprintNativeEvent`-based rule could be
  Blueprint-authored, and calling one off the game thread isn't safe). A new
  `IConnectIt_ScoringRule::GetMinimumConnectLength()` (mirrors `GetTargetScore()`) lets the
  connect length be read polymorphically, once, on the game thread.
- **A real decision-to-request bridge.** `AConnectIt_AIController::OnMyTurnStarted()` (called by
  a new `UConnectIt_AIActionsComponent`, installed the same way
  `UConnectIt_TurnBasedActionsComponent` is on the player controller -- see below) resolves
  `ConnectLength`/`WinScoreThreshold`/`AISearchDepth`/`AIThreadDepth` from the real, resolved
  `UConnectIt_BoardRules` and `LevelConfig`, runs `UConnectIt_MinMaxTreeBuilder`, picks the
  highest-scoring root move, resolves its own loadout's PlacePiece-producing action class (by
  scanning for `ProducesRequestType`, not a hardcoded reference -- matches the
  [class-keyed action config](../_decisions/2026-09-18-action-config-keyed-by-class-not-tag.md)
  convention), and calls `AConnectIt_GameMode::ProcessBoardRequest` directly (server-side, no
  RPC -- the AI only ever exists on the server).
- **The turn-started hook.** `ATurnBasedActionsComponent` has no bindable "my turn started"
  delegate, only the protected `OnTurnStarted` `BlueprintNativeEvent` meant for a component
  subclass to override. New `UConnectIt_AIActionsComponent` overrides it to call
  `AConnectIt_AIController::OnMyTurnStarted()` after the base's default handling runs. Installing
  it required a small plugin change: `ATurnBasedAIController`'s constructor didn't accept/forward
  `FObjectInitializer` (needed for the `SetDefaultSubobjectClass` override technique), unlike
  `ATurnBasedPlayerControllerBase`, which already did for exactly this reason -- brought the two
  in line.
- **The turn-end loop.** Nothing on the AI's own action stack ever completes (it never
  pushes/activates a real action instance), so the usual `HandleActionCompleted` →
  `CheckAutoEndTurn` trigger never fires for it. `SubmitBestMove` checks
  `ActionsComponent->CanEndTurn()` itself after a successful submit (safe to check immediately --
  `ConsumeActionUse` already updated `PlayerState` synchronously, no replication lag to wait out
  the way a human client's limbo does) and either ends the turn or decides again from the
  post-move board, capped at `MaxDecisionsPerTurn` (16) to avoid looping forever against an
  unreachable turn-end configuration.
- **Dead code removed:** `ConnectIt::FMinMaxNode`/`UConnectIt_MinMaxManager` (the older, unused
  node model) and `ConcreteMinMaxExample.h`/`.cpp` deleted outright.
- **Left alone, deliberately:** `UConnectIt_GameRulesLibrary` (its `IsGameOver`/
  `ConnectIt_Score_Max` are now unused by MinMax, but it's `BlueprintCallable` and might still be
  Blueprint-referenced -- not confirmed dead, not in the approved scope to chase down this pass).
  The plugin's own unused MinMax template variants (`BuildNodeRecursive2`/`EvaluateNodeRecursive`/
  `MinMax::TMinMaxManager`/`MinMaxABMoveOrder.h`) -- reusable scaffolding for other consumers, not
  ConnectIt-specific dead weight.

## Known limitation, accepted

The search assumes Classic-mode levels use the default C++ rule classes. If
`GetMinimumConnectLength()` returns 0 (unset, or a Blueprint override that doesn't implement it),
the search falls back to `ConnectLength = 4` with a logged warning -- it will silently disagree
with a real Blueprint-authored scoring rule's actual behavior. Not solved this pass; see
[the thread-safety decision](../_decisions/2026-09-24-minmax-calls-real-rules-as-static-thread-safe-functions.md).

## Adventure mode's own AI -- deliberately not designed here

Discussed at length (2026-09-24 check-in and this session) but left open. The candidate shape: a
three-tier system -- Classic MinMax (fixed, this note) / a shared, per-level-configurable
"Utility AI" controller (enable the dormant `UnrealAIMechanics` plugin, implement its stubbed
`SelectEvaluatedAction`, write one `UAI_UtilityGameActionEvaluator` subclass per action type,
levels configure which evaluators + weights via an `Instanced` array on `LevelConfig`, mirroring
the [`BoardReactions` precedent](../_decisions/2026-09-20-board-changes-layered-primitives-requests-reactions.md))
/ a fully bespoke `AIController` subclass as an escape hatch for a level that wants something the
utility framework can't express. `UnrealAIMechanics`' data model (no lookahead, weighted
considerations scored per currently-available action) suits "interesting, not optimal" far better
than another search -- confirmed by direct audit of that plugin (`FGameActionEvaluated`,
`UAI_UtilityGameActionEvaluator`, `UAI_UtilityGameActionController` are real; only the actual
pick-a-winner step, `SelectEvaluatedAction`, is a one-line stub). The owner wanted more time to
weigh the toolkit-vs-bespoke-per-level tradeoff before committing -- not decided.

`AConnectIt_AIController::CheckAndApplyForcedMove` (still a stub, returns `false`) is presumably
an Adventure-mode hook (forced moves), untouched by this rebuild.

## Verification

Owner rebuilds (plugin + game, editor closed): the plugin constructor change affects
`ATurnBasedAIController`, used only by `AConnectIt_AIController`. In PIE, a Classic-mode match vs.
the AI: confirm a log line on the AI's turn (`BeginDecision`/`SubmitBestMove`), the search
completes at the configured `AISearchDepth`, the move is legal, `ProcessBoardRequest` returns
true. Play several turns to confirm the AI reacts to the human and that its search-time
scoring/win-detection agrees with the real board (no more "search says this wins but the real
rule disagrees"). Clear `Content/_ConnectIt/_Framework/Controllers/ConnectIt_AIController_Game.uasset`'s
now-orphaned `BuildTree`/`EvaluateTree`/`MinMaxManager` graph wiring (targeted the deleted
`UConnectIt_MinMaxManager`) before testing. Confirm `AConnectIt_GameMode::AIControllerClass` and
`UConnectIt_LevelConfigDataAsset::EnemyLoadout` are actually set on the level used (both have
defensive unset-guards, so a silent no-op is possible). I can't run the editor or PIE from here.
