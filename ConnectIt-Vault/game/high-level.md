# game/ — the ConnectIt game module — high level

> Seed note. Filled from the repo README, `ConnectIt.uproject`, and the plugin
> exploration; **expand it from `../../Source/ConnectIt/`** as the game-specific systems
> get documented.

## Purpose

`ConnectIt` is the game itself: a turn-based, tile-based strategy game where players
compete by connecting tiles to form patterns and score points, inspired by Connect 4 and
Go (per [`../../README.md`](../../README.md)). This module holds the game-specific rules,
scoring, board setup, and AI, and it wires the plugin suite together into an actual game.

## Status

Active — this is the shipping target. Lives in `../../Source/ConnectIt/` (a game module,
not a plugin). Current work is on branch `0.3.0`; recent commits touch the board manager,
score board, game-over flow, and a rules "interpreter".

## How it consumes the plugin suite

- **Turn-based framework** — GameMode / GameState / PlayerState / controllers derive from
  the `ATurnBased*` bases in
  [[../UnrealTurnBasedMechanics/high-level|UnrealTurnBasedMechanics]].
- **Board / grid** — tiles, pieces, board state, and grid math come from
  [[../UnrealGridMechanics/high-level|UnrealGridMechanics]].
- **Gameplay scaffolding** — phase-barrier / tag-queue sequencing, pooling, input tag
  binding, scoring helpers from
  [[../UnrealGameMechanics/high-level|UnrealGameMechanics]].
- **AI / debug** — implements the influence-map and utility-score debug interfaces from
  [[../UnrealGameIntelligence/high-level|UnrealGameIntelligence]]; carries its **own**
  MinMax / game-tree implementations rather than the plugin's templates (see that note's
  rough edges).
- **Utils** — `IsAuthoritative` from
  [[../UnrealCodingUtils/high-level|UnrealCodingUtils]].

## Where things live

- Code: `../../Source/ConnectIt/`
- Content: `../../Content/_ConnectIt/`, `../../Content/_ConnectItNetworked/`
- Config: `../../Config/` — notably `DefaultGameplayTags.ini` (the tag vocabulary the
  phase-barrier and turn systems key off), `DefaultInput.ini`, `DefaultEngine.ini`.

## To document next

- The board-manager / score-board / game-over flow (recent refactors).
- The rules "interpreter" and how a turn resolves end to end.
- The game's own MinMax AI and whether it should converge with the plugin templates.
- Networked vs local play differences (`_ConnectItNetworked/`).

## In-repo reference

- [`../../README.md`](../../README.md) — game pitch (currently truncated)
- [`../../Source/ConnectIt/`](../../Source/ConnectIt/) — the code
- [`../../FIXES.txt`](../../FIXES.txt) — running fix notes
