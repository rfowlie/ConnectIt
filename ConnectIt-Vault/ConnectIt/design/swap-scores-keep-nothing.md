# A swap that scores keeps nothing; Scored events say which pieces were cleared

## Context
When a line scores, the piece that completed it stays on the board. That is right for placing a piece, but the owner
wants to try, for balance, a swap that scores leaving nothing behind. Today the behaviour is fixed inside
`FConnectItScoringRule_Lines::ApplyScoringLine`, which doesn't know what kind of operation caused the score.

Looking at it also showed a gap from the board-events change: a Scored event lists every tile of the line, and its
listener can no longer see the operation event that caused it, so visuals cannot tell which pieces to despawn. The
event needs to say so explicitly, whatever the swap rule is.

Owner's choice: the rule is **fixed in the Swap operation** (not a per-level setting).

Step 0 after approval: move this file to `ConnectIt-Vault/ConnectIt/design/swap-scores-keep-nothing.md` and delete
the staging copy.

## Changes
1. **Operation says whether its arriving pieces survive scoring** (`Public/Board/Operations/ConnectIt_BoardOperation.h`,
   `ConnectIt_BoardOperations.h/.cpp`):
   `virtual bool ArrivingPiecesSurviveScoring() const { return true; }` on the base; `_SwapPieces` overrides to
   `false`. Place, Force Place, Capture and Shift keep today's behaviour.
2. **Passed through the resolve step** (`ConnectIt_RuleSet.*`, `ConnectIt_ScoringRule.h`, `ConnectIt_LineScoringRule.*`):
   `ResolveBoardChange(Board, TouchedPositions, bool bArrivingPiecesSurvive, OutEvents = nullptr)` →
   `ApplyScoring(Board, Position, Faction, bool bArrivingPieceSurvives, OutEvents)` → `ApplyScoringLine`, which only
   puts the completing piece back when told to. Points and the +1 multiplier on every tile of the line are unchanged.
3. **Both callers pass the operation's answer**: `ConnectIt_BoardRequestMediator.cpp` (`DispatchRequest`) and
   `ConnectIt_MinMaxRules.cpp` (`ApplyMove`), so the server and the AI's search cannot disagree.
4. **`FConnectItBoardEvent_Scored` gains `ClearedPositions`** (`Public/Board/Events/ConnectIt_BoardEvents.h`): the
   positions whose piece this score removed. `Positions` stays "every tile that took part" (for highlighting).
   A position is listed only if a piece was actually removed there by this score, so when two lines cross at a piece
   that is not kept, it appears in the first event only.

## Implications (for the record)
- A swap can complete the *other* faction's line; that arriving piece is cleared too.
- Scoring by swap now costs a piece and leaves an empty tile with a raised multiplier open to either player.
- Two lines crossing at the swapped piece both still score (lines are found before anything is cleared).
- The AI needs no change: it does not model swaps, and runs the same code when it does.

## Tests (`Private/Tests/ConnectIt_MinMaxTests.cpp`)
- Update the `ResolveBoardChange` / `ApplyScoring` calls for the new parameter.
- `SwapOperation`: resolve with the swap's own answer -- still 4 points, the completing tile is now empty,
  `ClearedPositions` has all 4 tiles; and the operation reports `ArrivingPiecesSurviveScoring() == false` while Place
  reports `true`.
- `ScoredEventPerLine` (placement, two crossing lines): the completing piece stays; each event clears 3.
- New case in the same test: the same cross resolved with "do not survive": the completing tile ends empty, the
  first event clears 4 and the second 3 (no position cleared twice).

## Blueprint (owner)
In the Scored handler of `CI_PieceVisualHandler`, despawn the pieces at `ClearedPositions` (instead of "the line
minus the completing tile").

## Vault
Step 0; short decision note (swap that scores keeps nothing: a balance trial, fixed in the operation; Scored events
list cleared positions); log + indexes. Task rows left to the owner.

## Verification
- Build with the editor closed -- this also compiles the still-unbuilt event-library move -- then
  `Automation RunTests ConnectIt`, all pass.
- Owner, PIE: a placement that scores leaves its piece; a swap that scores leaves the line empty on both machines,
  for whichever faction scored.

## Status (2026-10-07): written, NOT yet compiled

- All edits are in the working tree as planned. The build was refused twice because the editor was open with Live
  Coding active, so neither this change nor the earlier event-library move has been compiled or tested.
- One difference from the plan: the owner had already added `PlacedPosition` to the Scored event; it is kept, with
  its comment updated, alongside the new `ClearedPositions`.
- Next: build with the editor closed, then `Automation RunTests ConnectIt`.
- Decision: [a swap that scores keeps nothing](../_decisions/2026-10-07-swap-that-scores-keeps-nothing.md).
