---
Date: 2026-10-07
status: Active
superseded by:
tags:
  - ai
  - minmax
---
## Decision

What the MinMax AI values is **two small structs of named float weights** on `UConnectIt_AIStrategy_MinMax`
(`FConnectItMinMaxEvaluationWeights`: Score Difference, Line Potential; `FConnectItMinMaxOrderingWeights`: Tile
Multiplier, Adjacent Pieces), and the logic those weights scale is **plain handwritten code** in
`FConnectItMinMaxRules`:

- `EvaluateState` and `EvaluateMove` are written out by hand, each calling helper functions that take exactly what
  they need (`ScoreDifference(Board, Side)`, `LinePotential(Board, Side)`, `TileMultiplierAt(Board, TileIndex)`,
  `CountAdjacentPieces(Board, TileIndex)`).
- `EvaluateMove` decides per kind of move which tiles matter (today: a Place Piece operation's tile).
- The polymorphic term structs (`FConnectItMinMaxEvalTerm`, `FConnectItMinMaxOrderTerm` and their subclasses) and the
  two editor lists of them are removed.

Supersedes [MinMax evaluation as editor terms](2026-10-02-minmax-evaluation-as-editor-terms.md).

## Why

Every term was forced through one signature. The ordering terms really ask "given a tile, how many neighbours / what
multiplier", but had to take a move and dig a position out of it, which tied them to Place Piece. The generic loop
over terms had one caller each and bought nothing for two terms apiece. Handwritten functions let each piece of logic
take the inputs it needs and let a new kind of move choose which tiles it is judged by.

A struct of named floats was chosen over a `TMap<GameplayTag, float>`: a tag cannot be switched on, the map would be
resolved to plain floats per decision anyway, and named fields show defaults and tooltips with no tags to maintain or
mistype. The owner chose to convert evaluation as well as ordering, for consistency.

## What Would Change It

Wanting designers to add a new judgement factor without a code change, or different AI profiles needing different
*sets* of factors rather than different weights. (Today a new factor is a weight, a helper and one line.)
