# CLAUDE.md — ConnectIt Vault

This is the knowledge base for AI co-development of **ConnectIt**, a turn-based
tile-based strategy game built in Unreal Engine 5 (C++). The vault sits inside the game
repo at `ConnectIt-Vault/` and is an Obsidian vault.

## What this vault is (and is not)

- **It is** the high-altitude, *evolving* layer: short "what is this / why does it exist
  / what changed and why it matters" notes an AI co-developer can load quickly, plus a
  running log of updates over time.
- **It is not** the exhaustive API reference. Each plugin's / the module's detailed
  `README.md` + `Systems.md` (+ the game module's `Docs/Workflows/`) were **moved to
  `../old/`** pending review — under `../old/Plugins/<PluginName>/Docs/` and
  `../old/Source/ConnectIt/Docs/`. The vault's `code/` pages link to those; they don't
  duplicate them.
- Vault notes *describe* the code — they are not the code. When a note and the code
  disagree, the code (`../Plugins/`, `../Source/`) is right; fix the note.

## Folder map

Every content folder below is a **section** — it carries a `_section.md` marker naming
its lead doc and the schemas that govern its subfolders. Discovery rule: a folder is the
schema set if it is named `_schema/`; a folder is a section if it contains `_section.md`.

Each **custom plugin** in `../Plugins/` has a section here, at the vault root, with the
same name as the plugin. Plus three cross-cutting sections: `game/`, `architecture/`,
`devlog/`.

| Folder | Status | What it covers |
|---|---|---|
| [[UnrealGameMechanics/high-level\|UnrealGameMechanics]] | active (enabled) | Reusable gameplay backbone: gated GameEvent phase-barrier + tag queue, turn bookkeeping, actor pooling, Enhanced Input tag binder, scoring/state-machine helpers. |
| [[UnrealGridMechanics/high-level\|UnrealGridMechanics]] | active (enabled) | Game-agnostic 2D grid/board framework: grid coords/directions, grid + shape math libraries, tile/piece actors, registries, hover/cursor subsystems, replicated board state, row/col shift, editor validator. |
| [[UnrealTurnBasedMechanics/high-level\|UnrealTurnBasedMechanics]] | active (enabled) | Network-replicated turn-based match framework: per-controller action stack, replicated match state machine (ready-check, timers, forfeit/reconnect), pluggable turn-order strategies, GameMode/State/PlayerState/Controller bases. |
| [[UnrealGameIntelligence/high-level\|UnrealGameIntelligence]] | active (enabled) | AI/decision toolkit: influence-map debug-visualisation framework + header-only MinMax / alpha-beta / move-ordering game-tree templates + utility-score debug interface. |
| [[UnrealCodingUtils/high-level\|UnrealCodingUtils]] | active (enabled) | Tiny dependency-free helper grab-bag; currently one class (`IsAuthoritative(UActorComponent*)`). |
| [[UnrealAIMechanics/high-level\|UnrealAIMechanics]] | dormant (not enabled) | Weighted-scoring "Utility AI" action picker; core selection method is still a stub. |
| [[UnrealUIMechanics/high-level\|UnrealUIMechanics]] | stub (not enabled) | Empty generated module skeleton — no classes yet. |
| [[ConnectIt/high-level\|game/]] | — | The ConnectIt game module itself (`../Source/ConnectIt/`): game-specific systems, rules, and how it consumes the plugin suite. |
| [[architecture/overview\|architecture/]] | — | Cross-cutting: the plugin dependency graph, layering rules, what "game-agnostic" means here. |
| [[devlog/README\|devlog/]] | — | Dated decision / session records. |

`../Plugins/AdvancedSessions-5-5/` is third-party (vendored) and deliberately has **no**
folder here.

`_schema/` holds the **section rulebooks** — one file per *kind* of folder, plus
[`_schema/section.md`](_schema/section.md) (the `_section.md` marker spec) and
[`_schema/README.md`](_schema/README.md) (how the whole system works and how to reuse it
in another vault). Rules are referenced, never restated here.

## Per-code-unit folder shape (plugins + `game/`)

```
<Name>/
  _section.md      # marker: type: code-unit, lead: high-level.md, schemas: [updates, …]
  high-level.md    # what / why / status / API surface / deps / consumers / rough edges
  updates/         # dated change record for this code unit — see _schema/updates.md
  code/            # opt-in: per-type help pages          — see _schema/code.md
  systems/         # opt-in: per-flow narratives + diagrams — see _schema/systems.md
  recipes/         # opt-in: step-by-step task procedures  — see _schema/recipes.md
```

- **`_section.md`** — the section marker. See [`_schema/section.md`](_schema/section.md).
- **`high-level.md`** — kept short. The durable picture of the code unit: purpose,
  current status, key modules, public API surface, dependencies, consumers, a link to the
  in-repo `Docs/`, and known rough edges. Update it when something durable changes.
- **`updates/`** — the dated trail of changes, split `raw/` (verbatim source material) and
  `wiki/` (synthesis). Governed by [`_schema/updates.md`](_schema/updates.md), which
  carries the rules and the `raw/` + `wiki/` templates. Created on first use — the folder
  is not pre-scaffolded.
- **`code/` · `systems/` · `recipes/`** — the code-ingestion family, **opt-in per
  plugin**: present only once that plugin's source has been ingested, and listed in its
  `_section.md` `schemas:` when so. They share one git-anchored provenance model (no
  `raw/` copy of the source). `UnrealTurnBasedMechanics` is the first section to adopt
  them; the others stay `schemas: [updates]` until ingested.

`architecture/` and `devlog/` are sections too (`schemas: []`) — a lead doc and local
convention, no schema-governed subfolders.

## Conventions

- This file stays named exactly `CLAUDE.md` (Claude Code auto-loads nested `CLAUDE.md`
  files; anything else must be pointed at by hand).
- A `_` prefix marks a folder or file as *structural*: `_schema/` is the rulebook set,
  `_section.md` marks a section. Everything without the prefix is content.
- `_logs/` (vault root) is the vault-wide, machine-written record of every
  request/change made in a session — one flat note per run, governed by
  [`_schema/logs.md`](_schema/logs.md). Distinct from `devlog/`: `devlog/` is a curated,
  human-facing narrative of *notable* decisions and direction; `_logs/` is comprehensive
  and mechanical, not curated.
- Section rules live in `_schema/<kind>.md`, never in this file. A section that deviates
  adds `<kind>/README.md` starting `Extends _schema/<kind>.md. Differences:`, or notes it
  in its `_section.md` body.
- Use Obsidian `[[wikilinks]]` between notes; link to in-repo code/docs with relative
  paths (`../Plugins/...`, `../Source/...`).
- Dated files use `YYYY-MM-DD-<slug>.md`.
