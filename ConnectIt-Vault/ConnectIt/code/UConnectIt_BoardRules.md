---
schema: code
kind: UCLASS
role: primary
source:
  - Source/ConnectIt/Public/Board/Rules/ConnectIt_BoardRules.h
  - Source/ConnectIt/Private/Board/Rules/ConnectIt_BoardRules.cpp
  - Source/ConnectIt/Public/Board/Rules/ConnectIt_ScoringRule.h
  - Source/ConnectIt/Public/Board/Rules/ConnectIt_WinCondition.h
  - Source/ConnectIt/Public/Board/Rules/ConnectIt_TilePlaceableRule.h
  - Source/ConnectIt/Public/Board/Rules/ConnectIt_LineScoringRule.h
  - Source/ConnectIt/Public/Board/Rules/ConnectIt_ScoreThresholdWinCondition.h
  - Source/ConnectIt/Public/Board/Rules/ConnectIt_UnoccupiedTilePlaceableRule.h
reconciled: 2026-09-14
commit: 5047e21
---

# UConnectIt_BoardRules (+ scoring / win-condition / placement strategies)

`UObject`, **server-only**, on [[AConnectIt_GameMode|AConnectIt_GameMode]].
Holds the board's three **pluggable strategies** — an `IConnectIt_ScoringRule`, an
`IConnectIt_WinCondition`, and (added 2026-09-14) an `IConnectIt_TilePlaceableRule` —
mirroring `UnrealTurnBasedMechanics`'
[[UnrealTurnBasedMechanics/code/ITurnOrderInterface|ITurnOrderInterface]] pattern. Each
strategy slot is `TObjectPtr<UObject>` + `Instanced` + `meta=(MustImplement=…)` — **not**
`TScriptInterface`, which doesn't support the inline class-pick/edit Details-panel
behaviour `Instanced` is for (that only applies to a real `FObjectProperty`).

## When you touch this

- Changing how a placement scores, or what "won" means.
- Adding a new scoring shape or win condition (a new implementer — no other file changes).

## Entry points

- **Strategy slots (`EditAnywhere`, `Instanced`, `MustImplement`,
  `TObjectPtr<UObject>`):** `ScoringRule` (defaults to `UConnectIt_LineScoringRule`),
  `WinConditionRule` (defaults to `UConnectIt_ScoreThresholdWinCondition`),
  `TilePlaceableRule` (defaults to `UConnectIt_UnoccupiedTilePlaceableRule`) — all
  assigned in `Initialise()` if unset.
- `Initialise()` — once, from `AConnectIt_GameMode` (a plain UObject has no `BeginPlay`).
- `IsTilePlaceable(State, Position) → bool` — wraps
  `IConnectIt_TilePlaceableRule::Execute_IsTilePlaceable`; error-logs + returns `false`
  if unset. The mediator's `HandlePlacePieceRequest` gate.
- `ApplyScoring(MutableState, Position, FactionSlot, OutScoringPositions) → float` — wraps
  `IConnectIt_ScoringRule::Execute_ApplyScoring`; error-logs + returns 0 if unset.
- `CheckWinCondition(MutableState)` — wraps `IConnectIt_WinCondition::Execute_CheckWinCondition`.
- `GetTargetScore()` — server-only; production UI should read the **replicated**
  `FConnectItBoardState::TargetScore` instead (`CheckWinCondition` stamps it every check).
- Debug: `GetActiveScoringRuleName`, `GetActiveWinConditionName`.

### `IConnectIt_ScoringRule` (implement to add a scoring shape)

- `ApplyScoring(FConnectItBoardState& MutableState (ref), FGridPosition Position, int32
  FactionSlot, TArray<FGridPosition>& OutScoringPositions (ref)) → float` —
  BlueprintNativeEvent. Called **after** the piece is written into `MutableState` at
  `Position`; mutate further consequences (clear tiles, bump multipliers, update
  `ScoreBoard`), append completed-line tiles to `OutScoringPositions` (caller empties it
  each call), return points awarded.
- `UConnectIt_LineScoringRule` — the default; N-in-a-row (`ConnectLength`, `ClampMin 3`,
  default 4) in the four grid directions through the placed piece. `EditInlineNew,
  DefaultToInstanced`.

### `IConnectIt_WinCondition`

- `CheckWinCondition(FConnectItBoardState& MutableState (ref))` — set `bGameOver` /
  `WinningFactionSlot` when met.
- `GetTargetScore() const → float` — feeds `FConnectItBoardState::TargetScore` for UI;
  return 0 for non-score-based conditions (UI hides the progress bar).

### `IConnectIt_TilePlaceableRule` (implement to add a placement rule) — added 2026-09-14

- `IsTilePlaceable(const FConnectItBoardState& State, FGridPosition Position) const →
  bool` — BlueprintNativeEvent, read-only (must not mutate `State`). Called from
  `UConnectIt_BoardRequestMediator::HandlePlacePieceRequest` before any tile data is
  written.
- `UConnectIt_UnoccupiedTilePlaceableRule` — the default; delegates straight to
  `FConnectItBoardState::IsTileValidForPlacement` (active && !occupied) — today's
  previously-hardcoded logic, unchanged in behaviour, just now swappable. `EditInlineNew,
  DefaultToInstanced`.
- **Client-side prediction still uses the raw `IsTileValidForPlacement` directly**
  (`ConnectIt_PlacePieceAction`, `AConnectIt_GameState`,
  `UConnectIt_BoardStateLibrary`) — `BoardRules` is server-only and structurally
  unreachable from any client. A custom `TilePlaceableRule` can make the server reject a
  placement the client's own hover/selection UI showed as valid — known limitation, not
  fixed.

## Collaborators

- Called by [[UConnectIt_BoardRequestMediator|UConnectIt_BoardRequestMediator]]
  handlers on a working `FConnectItBoardState` before it's committed via `SetBoardState`.

## Gotchas

- Everything here is **server-only** and not replicated — UI must go through
  `FConnectItBoardState::TargetScore` / `AConnectIt_GameState`, never these objects.
- Scoring runs on a *mutable working copy* of the board state, then the mediator commits
  it — the `FConnectItBoardChangeEvent` carries `bLineScored` / `PointsScored` /
  `ScoringLinePositions` / `bGameWon`.
- `ApplyScoring` is defined around **one** just-completed position — multi-position
  changes (swap) have no well-defined call (see the mediator's `HandleSwapPiecesRequest`).

## Cross-impact

A new strategy is self-contained: implement the interface, assign it on `UConnectIt_BoardRules`
(or leave the default). If it needs new per-move result data, that goes on
`FConnectItBoardChangeEvent` + `EnqueueBoardEventTags`.

## Changes

- 2026-09-14 — added the third pluggable strategy, `IConnectIt_TilePlaceableRule` /
  `UConnectIt_UnoccupiedTilePlaceableRule`, closing the mediator's long-standing TODO
  ("make `IsTileValidForPlacement()` a BoardRule function"). Corrected all three strategy
  slots from `TScriptInterface` (which silently didn't support `Instanced`'s inline
  class-pick/edit behaviour) to `TObjectPtr<UObject>` + `Instanced` + `MustImplement` —
  the pattern this page's own precedent comment already claimed but the code didn't
  actually use. See
  [[ConnectIt/_discussions/2026-09-12-board-mediator-and-action-config-design|_discussions/2026-09-12-board-mediator-and-action-config-design]].
- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[ConnectIt/CLAUDE|ConnectIt overview]].
- [[add-a-scoring-rule|recipes/add-a-scoring-rule]]
