# CLAUDE.md — ConnectIt (game module)

`ConnectIt` is the game itself: a turn-based, tile-based strategy game where players
compete by connecting tiles to form patterns and score points (Connect 4 / Go lineage).
This module holds the game-specific rules, scoring, board setup, and AI, and wires the
plugin suite into an actual networked game. It is the only domain with game-specific
logic — everything reusable lives in a plugin.

## Domain

- **Source:** `../Source/ConnectIt/` — one game module (not a plugin). Sub-areas:
  `Framework/` (Controller / Data / Game State Machine / GameMode / GameState / Interface
  / PlayerState / Subsystem), `Board/` (+ `Board/Rules/`), `Action/`, `GameEvent/`,
  `GameIntelligence/InfluenceMap/`, `Grid/`, `Library/`, `MinMax/`, `UI/`.
- **Status:** active — the shipping target. Branch `0.3.1-Vault-Schema-Update`.
- **Content:** `../Content/_ConnectIt/`, `../Content/_ConnectItNetworked/`.
- **Config:** `../Config/` — `DefaultGameplayTags.ini` is the tag vocabulary the
  phase-barrier and turn systems key off.

## How it consumes the suite

- **Turn-based framework** — GameMode / GameState / PlayerState / controllers derive from
  the `ATurnBased*` bases in [[UnrealTurnBasedMechanics/CLAUDE|UnrealTurnBasedMechanics]].
- **Board / grid** — tiles, pieces, registries, board-state base, grid math from
  [[UnrealGridMechanics/CLAUDE|UnrealGridMechanics]].
- **Scaffolding** — phase-barrier / tag-queue sequencing, pooling, input tag binding,
  scoring helpers from [[UnrealGameMechanics/CLAUDE|UnrealGameMechanics]].
- **AI / debug** — implements the influence-map + utility-score debug interfaces from
  [[UnrealGameIntelligence/CLAUDE|UnrealGameIntelligence]]; carries its **own** MinMax
  (not the plugin templates).
- **Utils** — authority checks from [[UnrealCodingUtils/CLAUDE|UnrealCodingUtils]].

The full suite dependency graph and layering is the **Map** section of
[[ConnectIt/code/__INDEX|code/__INDEX.md]].

## Sections

- **`code/`** — per-type pages, `code/systems/` flows, `code/recipes/` tasks,
  `code/__INDEX.md` (inventory of every public type + the suite map + known issues).
  Governed by [[_core/_schema/_code|_core/_schema/_code.md]].
- **`decisions/`** — one dated note per settled non-obvious call (board-architecture
  overhaul, registry relocation, legacy-pipeline retirement, …). Governed by
  [[_core/_schema/_decisions|_core/_schema/_decisions.md]].
- **`logs/`** — ConnectIt-scoped maintenance passes ([[_core/_schema/_logs|schema]]).

## Start here

[[ConnectIt/code/__INDEX|code/__INDEX.md]] — inventory, suite map, known issues.

## Current state

- **Board architecture** is post-refactor: `AConnectIt_BoardManager` (the world actor) is
  **retired**. Board authority is server-only UObjects on
  [[ConnectIt/code/AConnectIt_GameMode|AConnectIt_GameMode]] —
  `UConnectIt_BoardRequestMediator` + `UConnectIt_BoardRules`. The board is one replicated
  snapshot on `UConnectIt_BoardStateComponent` (on the GameState). `TileRegistry` /
  `PieceRegistry` now live on `UConnectIt_BoardRegistrySubsystem` (`UWorldSubsystem`),
  `DuplicateObject`'d per world from `Instanced` templates on
  `UConnectIt_LevelConfigDataAsset`. See [[ConnectIt/decisions/__INDEX|decisions/]].
- **Retired / dead — do not document as live:** the tag-reactive interpreter pipeline
  (its game-event-queue replacement on the mediator is an empty stub); the whole ConnectIt
  shift pipeline; the legacy MVVM pipeline (`UConnectIt_State_*`, `UConnectIt_GameFacade`,
  `UConnectIt_GameViewModel`, `AConnectIt_GameMode_Play`).
