---
schema: recipes
task: Change how a just-placed piece scores by adding a new IConnectIt_ScoringRule implementation.
touches:
  - a new UObject implementing IConnectIt_ScoringRule
  - Source/ConnectIt/Public/Board/Rules/ConnectIt_BoardRules.h (assign the strategy, or leave default)
  - FConnectItBoardChangeEvent fields (only if the rule produces new per-move result data)
reconciled: 2026-09-07
commit: 668872e
---

# Recipe: add a scoring rule

## Goal

A new scoring shape (e.g. match a fixed pattern, score squares, weight by multiplier
differently) that runs on every placement, without touching the mediator or board state
component.

## Prerequisites

- Understand the flow: the mediator writes the piece into a working `FConnectItBoardState`,
  then calls `UConnectIt_BoardRules::ApplyScoring` — see
  [[game/systems/place-piece-request|place-piece-request]] §5.
- `UConnectIt_LineScoringRule` is the reference implementation (N-in-a-row).

## Steps

1. **Create the class.** `UObject` + `IConnectIt_ScoringRule`, marked
   `UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)` (so it can be
   assigned inline on `UConnectIt_BoardRules`).
2. **Implement `ApplyScoring_Implementation`:**
   ```cpp
   float UMyRule::ApplyScoring_Implementation(
       FConnectItBoardState& MutableState,   // piece already written at Position
       FGridPosition Position,
       int32 FactionSlot,
       TArray<FGridPosition>& OutScoringPositions);
   ```
   - Inspect `MutableState` around `Position` for your pattern.
   - Mutate consequences into `MutableState` (bump `FConnectItTileData::Multiplier`, clear
     tiles via `SetTileData`, update `ScoreBoard`).
   - Append every scoring tile to `OutScoringPositions` (the caller empties it each call).
   - Return total points awarded (0 if nothing scored).
3. **Assign it.** On `UConnectIt_BoardRules` (the instance
   `AConnectIt_GameMode::HandleMatchHasStarted` constructs) set `ScoringRule` to your
   class — in the GameMode Blueprint's `BoardRules` default, or leave `UConnectIt_BoardRules::Initialise`
   to keep the default and swap per level config.
4. **(only if new result data)** If your rule needs to surface something new to visuals
   (beyond `PointsScored` / `ScoringLinePositions`), add fields to
   `FConnectItBoardChangeEvent`, set them where the mediator assembles the event, and add
   an `EnqueueBoardEventTags` branch + tag.

## Verify

- PIE, make a placement that should score under your rule: `AConnectIt_GameState::GetFactionScore`
  changes by the expected amount; `FConnectItBoardChangeEvent::bLineScored` /
  `PointsScored` / `ScoringLinePositions` are populated (check `UDWidget_ConnectIt_BoardStateComponent`).
- `UConnectIt_BoardRules::GetActiveScoringRuleName()` returns your class.
- A non-scoring placement still returns 0 and leaves `OutScoringPositions` untouched.
- Win condition still fires when the threshold is crossed (scoring feeds
  `CheckWinCondition` on the same working state).

## Pitfalls

- **Mutating the live board** — you get a `FConnectItBoardState&` that is the *working
  copy*; the mediator commits it via `SetBoardState`. Don't call `SetBoardState` yourself.
- **Not clearing / appending `OutScoringPositions` correctly** — the caller relies on "empty
  = nothing scored."
- Forgetting `EditInlineNew, DefaultToInstanced` → you can't assign it inline on
  `UConnectIt_BoardRules`.
- Assuming one line — `ScoringLinePositions` is the union across simultaneously completed
  lines.
- `ApplyScoring` is single-position by contract; multi-position effects (like a swap)
  don't have a clean call here (see the mediator's `HandleSwapPiecesRequest` gap).

## See also

- [[game/code/UConnectIt_BoardRules|UConnectIt_BoardRules]] ·
  [[game/code/ConnectItStructs|ConnectItStructs]]
- In-repo: `Docs/README.md` → rules; `Docs/Duplication.md`.
