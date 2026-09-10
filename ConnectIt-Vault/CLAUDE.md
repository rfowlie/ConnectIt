# CLAUDE.md — ConnectIt Vault

`@_core/CLAUDE.md`

The knowledge base for AI co-development of **ConnectIt**, a turn-based tile-based
strategy game in Unreal Engine 5 (C++). The vault sits inside the game repo at
`ConnectIt-Vault/` and is an Obsidian vault.

- **It is** the high-altitude, evolving layer over the code — short "what is this / why /
  what changed" notes an AI co-developer loads quickly.
- **It is not** the exhaustive API reference. When a note and the code disagree, the code
  (`../Plugins/`, `../Source/`) is right — fix the note.
- **Why it's shaped this way** and the decision history: [`ABOUT.md`](ABOUT.md).
- **How the structure works** (domains, sections, schema): [`_core/`](_core/CLAUDE.md) —
  loaded every session via the `@import` above.

## Domains

Vault root = [`_core/`](_core/CLAUDE.md) (the operating system) + these domains, each a
stand-alone folder with its own `CLAUDE.md`. The 7 plugins are game-agnostic and
independently extractable; `ConnectIt` is the only game-specific domain.

| Domain | What it is |
|---|---|
| [[ConnectIt/CLAUDE\|ConnectIt]] | The game module — rules, scoring, board setup, AI; wires the suite together. Start here. |
| [[UnrealGameMechanics/CLAUDE\|UnrealGameMechanics]] | Suite base: gated GameEvent phase-barrier + tag queue, turn bookkeeping, actor pooling, Enhanced Input tag binder, scoring / state-machine helpers, debug-widget base. |
| [[UnrealGridMechanics/CLAUDE\|UnrealGridMechanics]] | Game-agnostic 2D grid/board: coords + directions, grid/shape math, tile/piece actors, registries, hover subsystems, replicated board state, row/col shift, editor validator. |
| [[UnrealTurnBasedMechanics/CLAUDE\|UnrealTurnBasedMechanics]] | Network-replicated turn-based match: per-controller action stack, replicated match state machine (ready-check, timers, forfeit/reconnect), pluggable turn order, `ATurnBased*` framework bases. |
| [[UnrealGameIntelligence/CLAUDE\|UnrealGameIntelligence]] | AI toolkit: influence-map debug-visualisation framework + header-only MinMax templates + utility-score debug interface. |
| [[UnrealCodingUtils/CLAUDE\|UnrealCodingUtils]] | Tiny dependency-free helper library — component-level authority checks. |
| [[UnrealAIMechanics/CLAUDE\|UnrealAIMechanics]] | Weighted-scoring "Utility AI" action picker. **Dormant** — not enabled, core selection method stubbed. |
| [[UnrealUIMechanics/CLAUDE\|UnrealUIMechanics]] | UI widgets plugin. **Stub** — empty module skeleton, not enabled. |

`../Plugins/AdvancedSessions-5-5/` is third-party (vendored) — no domain here.
