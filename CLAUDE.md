# CLAUDE.md — ConnectIt (repo root)

**ConnectIt** is a turn-based, tile-based strategy game built in Unreal Engine 5 (C++).
See [`README.md`](README.md) for the game pitch. The gameplay is assembled from a suite of
first-party plugins under [`Plugins/`](Plugins/) plus the game module in
[`Source/ConnectIt/`](Source/ConnectIt/).

## Knowledge base

The knowledge base for AI co-development of this project lives in
[`ConnectIt-Vault/`](ConnectIt-Vault/) — an Obsidian vault holding high-altitude,
evolving notes about each plugin and system, plus a running log of updates.

**Read [`ConnectIt-Vault/CLAUDE.md`](ConnectIt-Vault/CLAUDE.md) next.** It is the
source of truth for that vault's structure and conventions, and Claude Code loads it
automatically whenever a session touches a file under `ConnectIt-Vault/`.

This file is a thin router. It carries no rules the vault's own `CLAUDE.md` needs — if
the two ever disagree, the vault wins.

> Note: this file replaced an unrelated stray `CLAUDE.md` (a router for a different
> "Second-Brain" workspace) that had been copied here by mistake.
