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
stand-alone folder with its own `CLAUDE.md`. The 7 plugins and `ConnectIt` are the
game-side domains (the plugins game-agnostic and independently extractable, `ConnectIt`
the only game-specific one); `Development` is the owner's personal operating-system
domain, unrelated to the game's own code.

| Domain | What it is |
| --- | --- |
| [ConnectIt](ConnectIt/CLAUDE.md) | The game module — rules, scoring, board setup, AI; wires the suite together. Start here. |
| [UnrealGameMechanics](UnrealGameMechanics/CLAUDE.md) | Suite base: gated GameEvent phase-barrier + tag queue, turn bookkeeping, actor pooling, Enhanced Input tag binder, scoring / state-machine helpers, debug-widget base. |
| [UnrealGridMechanics](UnrealGridMechanics/CLAUDE.md) | Game-agnostic 2D grid/board: coords + directions, grid/shape math, tile/piece actors, registries, hover subsystems, replicated board state, row/col shift, editor validator. |
| [UnrealTurnBasedMechanics](UnrealTurnBasedMechanics/CLAUDE.md) | Network-replicated turn-based match: per-controller action stack, replicated match state machine (ready-check, timers, forfeit/reconnect), pluggable turn order, `ATurnBased*` framework bases. |
| [UnrealGameIntelligence](UnrealGameIntelligence/CLAUDE.md) | AI toolkit: influence-map debug-visualisation framework + header-only MinMax templates + utility-score debug interface. |
| [UnrealCodingUtils](UnrealCodingUtils/CLAUDE.md) | Tiny dependency-free helper library — component-level authority checks. |
| [UnrealAIMechanics](UnrealAIMechanics/CLAUDE.md) | Weighted-scoring "Utility AI" action picker. **Dormant** — not enabled, core selection method stubbed. |
| [UnrealUIMechanics](UnrealUIMechanics/CLAUDE.md) | UI widgets plugin. **Stub** — empty module skeleton, not enabled. |
| [Development](Development/CLAUDE.md) | The owner's personal knowledge vault: the virtual advisory board, career/owner knowledge, and the game-projects group (its own nested `CLAUDE.md` tree, self-contained). |

`../Plugins/AdvancedSessions-5-5/` is third-party (vendored) — no domain here. `Unsorted/`
is a top-level folder with no `CLAUDE.md` yet — not a domain under the model until it gets
one.
