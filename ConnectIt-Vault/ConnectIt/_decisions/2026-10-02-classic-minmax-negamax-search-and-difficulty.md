---
Date: 2026-10-02
status: Active
superseded by:
tags:
  - ai
  - minmax
  - architecture
---
## Decision

Classic MinMax searches with a **depth-first negamax + alpha-beta** pass that generates children on demand, with
move ordering and **iterative deepening under a time budget** — not a stored tree built first and solved second. The
generic search lives in `UnrealGameIntelligence` (`Public/Search/GI_AlphaBetaSearch.h`, `GI_AsyncSearch.h`), added
**alongside** the older `MinMax/` templates rather than replacing them; ConnectIt plugs in a traits type
(`FConnectItClassicSearchGame`) whose evaluation is always **from the side to move**, with the AI as the root side.

Integration stays as the 09-24 rebuild had it: the AI controller submits a PlacePiece request **directly to the
Mediator** (no plugin-level submit API). Difficulty is per level (`FConnectItAIDifficulty` on the level config):
max depth + time budget + minimum think time, plus an optional chance to pick among the top N moves.
Classic is assumed to be **2 factions, 1 placement per turn**; the AI refuses to decide otherwise.

## Why

The 09-24 search scored the wrong side (root children placed the opponent's piece), evaluated from a perspective that
flipped every ply while min/max assumed a fixed one, and clamped every losing position to 0. It also stored ~113k board
copies at depth 3 before searching, so alpha-beta could prune evaluation but never generation. Negamax with a
side-to-move evaluator removes the perspective bug class structurally, and on-demand generation makes pruning count
(measured: depth 4 inside 1.5 s on a 7x7 mid-game board, ~460k nodes/s).

The owner chose direct Mediator submission (smaller, game-only) over a plugin submit API, and to keep the old plugin
templates for reference.

## What Would Change It

A Classic variant with more than one placement per turn or a third faction (needs turn-aware or max-n search); a level
with a Blueprint-authored scoring rule (the search can't call it off the game thread — see
[the static-rules decision](2026-09-24-minmax-calls-real-rules-as-static-thread-safe-functions.md)); or the AI needing
to go through the actions component like a human, at which point a plugin submit API becomes worth it.
