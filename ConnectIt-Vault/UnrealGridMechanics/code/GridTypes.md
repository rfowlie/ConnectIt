---
schema: code
kind: USTRUCT
role: primary
source:
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/GridMechanicsBaseStructs.h
  - Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/GridMechanicsBaseEnums.h
reconciled: 2026-09-06
commit: 0ed7243
---

# GridTypes (`FGridPosition`, `EGridDirection`, `FGridMovement`, `FGridPair`)

The value types the whole plugin is built on. `FGridPosition` is an integer `(X, Y)` cell
coordinate with a full operator set; `EGridDirection` is an 8-way compass enum with
wraparound arithmetic.

## When you touch this

- Any grid math. `FGridPosition` is the currency; `EGridDirection` names directions.

## Entry points

- **`FGridPosition`** — `X`, `Y` (`EditAnywhere`); `GetDistance(Other)` = **Chebyshev**
  (`max(|dX|,|dY|)`); operators `==`, `<` (row-major), `+`, `+=`, `-`, `* uint8`,
  `/ float` (rounds). `GetTypeHash` provided → usable as a `TMap`/`TSet` key.
- **`EGridDirection`** — `Up, UpRight, Right, DownRight, Down, DownLeft, Left, UpLeft`,
  then `Max` (`Hidden`). Free operators: `Dir + Dir`, `Dir + int32`, `Dir - Dir`,
  `Dir -= int32` — all modulo `Max` (8), negative-safe. **Note:** `operator-=` returns a
  value (does not mutate in place despite the name); there is no `operator+=`.
- **`FGridDirectionVector`** — `(Row, Column)` int pair; the unit-vector form of a
  direction (see `UGridMechanics_GridLibrary::GetGridDirectionVector`).
- **`FGridMovement`** — `GridTile` + `ParentTile` + `Cost`; `==` compares `GridTile`;
  hashable by tile. Pathfinding/movement result unit.
- **`FGridPair`** — `{AGridTileBase*, AGridUnitBase*}` convenience bundle.
- **`FGridPositionArray`** — `TArray<FGridPosition>` wrapper (for nesting in containers /
  Blueprint).

## Collaborators

Consumed everywhere:
[[UnrealGridMechanics/code/UGridMechanics_GridLibrary|UGridMechanics_GridLibrary]],
[[UnrealGridMechanics/code/UGridMechanics_ShapeLibrary|shape lib]],
the registries, board state, shift, and `UnrealTurnBasedMechanics` (grid-tile selection).

## Gotchas

- `GetDistance` is **Chebyshev**, not Manhattan/Euclidean — diagonals cost 1.
- `EGridDirection::operator-=` is misleadingly named — it does **not** compound-assign;
  capture the return value.
- `operator/` rounds to nearest int, `operator*` takes `uint8` only.
- `FGridMovement` equality ignores `Cost` and `ParentTile`.

## Cross-impact

These types are load-bearing across the whole plugin and `UnrealTurnBasedMechanics`. Any
change to `FGridPosition`'s layout or hash ripples into every `TMap<FGridPosition, …>`
(registries, shift `PositionRemap`, board state).

## See also

- In-repo: `old/Plugins/UnrealGridMechanics/Docs/README.md` → *Top-level (Structs/Enums)*.
