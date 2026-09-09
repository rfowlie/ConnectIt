# game/ — the ConnectIt game module — high level

> The `code/` / `systems/` / `recipes/` folders now hold a partial ingest of
> `../../Source/ConnectIt/` — start there for anything specific. This page stays the
> one-screen orientation.

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

## Board architecture (post-refactor)

The `AConnectIt_BoardManager` **actor is retired**. Board authority is now server-only
UObjects on [[game/code/AConnectIt_GameMode|AConnectIt_GameMode]]:
[[game/code/UConnectIt_BoardRequestMediator|UConnectIt_BoardRequestMediator]] (dispatches
`FTurnActionRequest`s to `HandleXRequest` handlers) and
[[game/code/UConnectIt_BoardRules|UConnectIt_BoardRules]] (pluggable scoring / win
strategies). The board itself is one replicated snapshot on
[[game/code/UConnectIt_BoardStateComponent|UConnectIt_BoardStateComponent]] (on the
GameState) — see [[game/systems/board-state-single-source-of-truth|single-source-of-truth]].
The tag-reactive interpreter that turned board changes into piece spawn/despawn was
removed; its replacement (a game-event queue on the mediator) is **unfinished** — several
`UConnectIt_*GameEvent` bodies are commented out.

`TileRegistry`/`PieceRegistry` (level-authored, deterministic, world-scoped tile/piece
lookups) went through a second relocation: briefly on `AConnectIt_PlayerController`
(per-machine), now on `UConnectIt_BoardRegistrySubsystem` (`UWorldSubsystem`, one canonical
instance per world). The concrete registry subclass + tuned values a designer picks per
level now live as `Instanced` template properties on
[[game/code/index#UConnectIt_LevelConfigDataAsset|UConnectIt_LevelConfigDataAsset]]; the
subsystem `DuplicateObject()`s its own runtime copy from each at `OnWorldBeginPlay` rather
than ever using the template live (a `UDataAsset`'s subobjects are shared/loaded-once,
unsafe to use directly as per-world state). See
[`old/Source/ConnectIt/Docs/ConnectItModule.md#board-registry-subsystem-second-pass`](../old/Source/ConnectIt/Docs/ConnectItModule.md#board-registry-subsystem-second-pass).

## Vault code docs

Partial ingest of `../../Source/ConnectIt/`, anchored to `668872e`. Per-type status in
each index.

- [[game/code/index|code/index.md]] ([[_schema/code|schema]]) — 8 pages
- [[game/systems/index|systems/index.md]] ([[_schema/systems|schema]]) — `place-piece-request`, `board-state-single-source-of-truth`, `game-state-machine`
- [[game/recipes/index|recipes/index.md]] ([[_schema/recipes|schema]]) — `add-a-scoring-rule`, `add-a-board-request-type`

## Still to ingest

- The AI turn (blackboard modifier → the game's own MinMax → a board request).
- Match start / board initialisation, game-over lockout.
- The `Library/` helpers, `UI/` debug widgets, influence map, level config.

## In-repo reference

The module ships extensive docs — treat these as authoritative:

- [`../../old/Source/ConnectIt/Docs/README.md`](../../old/Source/ConnectIt/Docs/README.md) and
  `Conventions.md`, `RuntimeStateAccess.md`, `UIValueCatalogue.md`, `Duplication.md`,
  `LegacyPipeline.md`
- [`../../old/Source/ConnectIt/Docs/Workflows/`](../../old/Source/ConnectIt/Docs/Workflows/) — six
  workflow docs (ServerAuthoritative-ActionRequest, GameplayTag-EventSequencing,
  SingleSourceOfTruth-Replication, SubsystemDiscovery-DualAccessPattern, DebugWidgets)
- [`../../README.md`](../../README.md) — game pitch · [`../../FIXES.txt`](../../FIXES.txt) — running fix notes
