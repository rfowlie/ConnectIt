---
created: 2026-10-20
question: "How should a level director change the board when a level condition (trigger) fires?"
status: answered
closed-by: "[2026-10-21-director-hub-and-level-changes](../_decisions/2026-10-21-director-hub-and-level-changes.md)"
tags:
  - question
---

# How should a level director change the board when a level condition (trigger) fires?

## Why it matters

The Level director ([three-directors](../design/three-directors.md)) owns level conditions -- triggers that fire once (and
can be re-armed) -- whose reactions alter the board: one-off changes like removing multipliers, toggling tiles, placing
blockers. Board changes today only happen as a player's move through the Mediator (a request, a loadout action, a faction).
A trigger has no player, no loadout and no turn.

## What would answer it

A server entry point (GameMode / Mediator) that carries out a board operation on the level's behalf, decided with: which
faction (if any) it acts as; whether it scores and checks for a win like a move; how its events reach the visual queue;
and whether it may run mid-move (inside a checkpoint reaction) or must wait for the move to finish.

## Current thinking

2026-10-20: the operations already exist (Force Place Piece, Toggle Tile Active, Destroy Tile Multiplier, Remove Piece...).
Lean: route them through the same rule set, board commit and events as a move (so visuals and the win check stay
consistent), with no loadout gate; run them after the current move has finished. Level triggers are not seen by the AI's
search (it can't simulate Blueprint reactions).

## Resolution

Answered and built 2026-10-21. The Level director's **Apply Board Operation (Operation, As Faction, Timing)** goes through
`AConnectIt_GameMode::ApplyLevelOperation` to `UConnectIt_BoardRequestMediator::ProcessLevelOperation`: the move's own board
step (CanApply, the operation, scoring, the win check, commit with events) without the loadout gate or an action use. It
acts as the faction given (or nobody, -1), scores and can win like a move, and its events reach the visual queue like a
move's. Timing: Immediately runs right after the move being carried out (never mid-scoring), Next Turn Start before the next
turn's directors hear it; chains are capped at 64. Recipe: [[script-a-level]].
