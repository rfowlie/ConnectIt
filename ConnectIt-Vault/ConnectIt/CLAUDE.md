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

## Current state lives elsewhere, not here

This file **never carries a "Current state" or single-next-action section.** One existed
here through 2026-09-14 and was deliberately removed — it always drifted from reality
between sessions and became actively misleading. It will not come back; don't
reintroduce one, and don't let `/session-close` or any other skill write one back in.

For what's actually happening right now, read (newest first):

- **[`_tasks/active.md`](_tasks/active.md)** — the real open-work list.
- **The newest note in [`_sessions/`](_sessions/)** — the last session's reckoning, if one
  ran.
- **The newest note in
  [`Development/optimal-co-developer/_meetings/`](../Development/optimal-co-developer/_meetings/__README.md)**
  — the last check-in's summary and `## Next`.
- **[`_decisions/`](_decisions/)** — recent settled calls, including explicit deferrals
  ("captured as next step after X ships").

`/session-open` reconstructs the map from exactly these on demand, every time — that's the
intended shape, not a workaround for this section's absence.

## How it consumes the suite

- **Turn-based framework** — GameMode / GameState / PlayerState / controllers derive from
  the `ATurnBased*` bases in [UnrealTurnBasedMechanics](../UnrealTurnBasedMechanics/CLAUDE.md).
- **Board / grid** — tiles, pieces, registries, board-state base, grid math from
  [UnrealGridMechanics](../UnrealGridMechanics/CLAUDE.md).
- **Scaffolding** — phase-barrier / tag-queue sequencing, pooling, input tag binding,
  scoring helpers from [UnrealGameMechanics](../UnrealGameMechanics/CLAUDE.md).
- **AI / debug** — implements the influence-map + utility-score debug interfaces from
  [UnrealGameIntelligence](../UnrealGameIntelligence/CLAUDE.md); carries its **own** MinMax
  (not the plugin templates).
- **Utils** — authority checks from [UnrealCodingUtils](../UnrealCodingUtils/CLAUDE.md).

The full suite dependency graph and layering is the **Map** section of
[code/__INDEX.md](code/__INDEX.md).

## Sections

- **`code/`** — per-type pages, `code/systems/` flows, `code/recipes/` tasks,
  `code/__INDEX.md` (inventory of every public type + the suite map + known issues).
  Governed by [`_core/_schema/_code.md`](../_core/_schema/_code.md).
- **`_decisions/`** — one dated note per settled non-obvious call (board-architecture
  overhaul, registry relocation, legacy-pipeline retirement, …). Governed by
  [`_core/_schema/_decisions.md`](../_core/_schema/_decisions.md).
- **`_logs/`** — ConnectIt-scoped maintenance passes ([schema](../_core/_schema/_logs.md)).
- **`_discussions/`** — one dated note per discussion (calls, brainstorms, transcribed
  voice memos); topics link back to their most recent prior mention instead of a folder
  hierarchy. [`__INDEX.md`](_discussions/__INDEX.md) tracks the growing list. Governed by
  [`_core/_schema/_discussions.md`](../_core/_schema/_discussions.md).
- **`_tasks/`** — status tables (`active` / `suspended` / `complete`) for ConnectIt-domain
  work. Governed by [`_core/_schema/_tasks.md`](../_core/_schema/_tasks.md).
- **`_sessions/`** — one dated note per work session, bookended by
  [`/session-open`](../Development/_skills/session-open.md) and
  [`/session-close`](../Development/_skills/session-close.md) (authored under
  `Development/_skills/` since the procedure is generic; the session notes themselves live
  here, scoped to this domain). `/session-close` fills this note's own `## Close`, never
  this file's own "current state" — see above. Governed by
  [`_core/_schema/_sessions.md`](../_core/_schema/_sessions.md).
- **`_questions/`** — one note per open, unresolved ConnectIt-scoped question, living
  until it closes. Governed by [`_core/_schema/_questions.md`](../_core/_schema/_questions.md).
- **`design/`** — game design notes: how each action is meant to work and its gameplay
  ramifications, feature sketches, open design questions. **No schema yet** — started
  2026-09-17, deliberately unstructured while it's still just "start writing notes down";
  a shape (or a real `_design` schema) can get imposed once there's enough here to need
  one. Not `_`-prefixed for exactly that reason.

## Start here

[code/__INDEX.md](code/__INDEX.md) — inventory, suite map, known issues.
