---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/GridMechanics_ShapeLibrary.h
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Private/GridMechanics_ShapeLibrary.cpp
reconciled: 2026-09-06
commit: 0ed7243
---

# UGridMechanics_ShapeLibrary

`UBlueprintFunctionLibrary`. Detects lines and simple shapes in a set of grid positions —
the "did this move complete a line / a square" question.

## When you touch this

- Win/score checks after a placement or a shift.
- Feeding shape data to scoring (`FShapeConfiguration` carries a `Name` + `Score`).

## Entry points

- `GetLongestLine(OutPositions, InPositions, StartingPosition, GridDirection)` — the run
  through `StartingPosition` along one direction (Blueprint).
- `GetLongestLines(OutLines, InPositions, StartingPosition, MinimumLength)` — C++-only;
  all directions, filtered by length.
- `GetLinesOfLength(OutLines, InPositions (TSet), GridDirection, RequiredLength,
  bExactLength)` — C++-only; every line of (at least / exactly) `RequiredLength` along a
  direction.
- `IsSquare(Positions, OutCorners)` — true + the 4 corners if the set forms an
  axis-aligned square.
- `FShapeConfiguration` — `{ TArray<FGridPosition> ShapeConfiguration, FName Name,
  int32 Score }`.

## Collaborators

- Operates on [[UnrealGridMechanics/code/GridTypes|GridTypes]].
- Complements
  [[UnrealGridMechanics/code/UGridMechanics_GridLibrary|UGridMechanics_GridLibrary]]'s
  connection *counting* — this one returns the actual cells.

## Gotchas

- **Does not consider ownership** — "positions should be pre-filtered before calling"
  (header comment). Pass only the cells belonging to one side.
- Two of the four functions are C++-only (`TArray<TArray<>>` / `TSet` params) — not
  reachable from Blueprint.
- `bExactLength` flips `GetLinesOfLength` between "≥ N" and "== N".

## Cross-impact

Self-contained. Scoring code that consumes `FShapeConfiguration.Score` owns the
name→score mapping, not this library.

## See also

- In-repo: `old/Plugins/UnrealGridMechanics/Docs/README.md` → *Library*.
- [[UnrealGridMechanics/code/UGridMechanics_GridLibrary|UGridMechanics_GridLibrary]]
