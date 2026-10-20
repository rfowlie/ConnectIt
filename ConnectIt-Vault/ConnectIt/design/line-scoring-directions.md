# Lines scoring rule: choose which directions score

## Context
The Lines scoring rule always checks all four axes (row, column, both diagonals). The owner wants the rule to expose
which directions score -- horizontal, vertical, diagonal, in any combination -- and only score lines in an allowed
direction. This is the "allowed axes" setting sketched in `design/mid-level-rule-changes.md` (the rotating-direction
Adventure level), now wanted as a per-level setting in its own right.

Owner's choice: **three checkboxes** (a bitflag enum), default all three, so today's behaviour is unchanged.
Diagonal covers both diagonals.

Step 0 after approval: move this file to `ConnectIt-Vault/ConnectIt/design/line-scoring-directions.md` and delete the
staging copy.

## Changes

### Scoring rule (`Public/Board/Rules/ConnectIt_LineScoringRule.h`, `Private/Board/Rules/ConnectIt_LineScoringRule.cpp`)
```cpp
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EConnectItScoringDirection : uint8
{
    None       = 0 UMETA(Hidden),
    Horizontal = 1 << 0,
    Vertical   = 1 << 1,
    Diagonal   = 1 << 2,   // both diagonals
};
ENUM_CLASS_FLAGS(EConnectItScoringDirection);

// on FConnectItScoringRule_Lines
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring",
    meta = (Bitmask, BitmaskEnum = "/Script/ConnectIt.EConnectItScoringDirection"))
int32 ScoringDirections = Horizontal | Vertical | Diagonal;
```
- The static `GetScoringDirections()` (always all four axes) is replaced by a member
  `GetAllowedAxes()` returning only the axes the mask allows (small inline array, no heap: the AI's search calls
  scoring for every position it tries). Horizontal = the existing "row" axis `{0,1}`, Vertical = the "column" axis
  `{1,0}`, Diagonal = `{1,1}` and `{1,-1}`.
- `FindScoringLines` walks only the allowed axes. Nothing else in scoring changes (points, multipliers, cleared
  pieces, Scored events).
- With no direction ticked nothing can ever score; documented on the property.

### AI (`Public/MinMax/ConnectIt_MinMaxRules.h`, `Private/MinMax/ConnectIt_MinMaxRules.cpp`)
- `FConnectItMinMaxGeometry::Build` takes the Lines rule (or null) instead of just the connect length, and builds
  line windows along the rule's allowed axes only. So Line Potential stops valuing lines that cannot score.
- The constructor's rule-set warning also mentions a Lines rule with no direction ticked.
- The search itself needs nothing: it already calls the same scoring rule.

### Already in place (no work)
- The setting replicates to clients with the rest of the rule set (`AConnectIt_GameState::MatchRules`).
- It can be changed during a match through `AConnectIt_GameMode::ModifyRules`, which is what the rotating-direction
  level will use; the AI rebuilds its geometry from the current rules every decision. No trigger or UI is built here.

## Tests (`Private/Tests/ConnectIt_MinMaxTests.cpp`)
- `ConnectIt.Board.Rules.LineDirections`: with Horizontal only, a completed row scores and a completed column and
  diagonal do not; with Diagonal only, both diagonals score and a row does not; default scores all.
- Geometry: on a 7x7 board with connect length 4, all directions give 88 line windows, Horizontal only gives 28.
- Existing tests unchanged (default mask = all).

## To confirm in the editor (owner)
- Level config → Rules → Scoring Rule (Lines) shows **Scoring Directions** as a three-way multi-select, all ticked.
- That "Horizontal" and "Vertical" match what you see on screen. They follow the code's existing "row" and "column"
  axes; if the board is drawn the other way round the two labels need swapping (a one-line change).

## Vault
Step 0; update `design/mid-level-rule-changes.md` (the allowed-axes setting now exists); log + index. No new
decision note: the approach was already settled in that design note. Task rows left to the owner.

## Verification
- Build (editor closed); `Automation RunTests ConnectIt` all pass; Throughput reported against the last figure.
- Owner, PIE: a level set to Horizontal only scores rows and ignores columns and diagonals; vs AI on that level, the
  AI builds and blocks rows.

## Status (2026-10-08): built (compiled and tested 2026-10-13, log 2026-10-13-1826: build clean, `ConnectIt.Board.Rules.LineDirections` passes). Earlier note, kept for the record -- written, not yet compiled

- All edits are in the working tree as planned. The build was refused because the editor was open with Live Coding
  active, so this has not been compiled or tested.
- Next: build with the editor closed, then `Automation RunTests ConnectIt` (new test:
  `ConnectIt.Board.Rules.LineDirections`).
