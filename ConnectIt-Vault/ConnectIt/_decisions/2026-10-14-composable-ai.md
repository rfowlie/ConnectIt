---
Date: 2026-10-14
status: Partly superseded
superseded by: "[2026-10-16-ai-director-pipeline](2026-10-16-ai-director-pipeline.md) (the coordinator is no longer a strategy; mistake chance moved to selectors; the director owns the whole AI setup; the AI profile is gone)"
tags:
  - ai
  - architecture
  - designer-tools
---
## Decision

The AI is built from small, swappable parts, settled with the owner in a back-and-forth on `code/systems/ai-turn`:

- **Strategies only decide** what to do given a context; no side effects. A decision is a **ranked list of candidate moves**,
  capped per strategy (`MaxCandidates`, default 5).
- **The controller submits candidates in order** until the server accepts one (the AI's version of a human retrying after a
  rejection); all rejected = an error and the turn ends. It stays the only place that talks to the server; no veto hook on it.
- **The decision context says what the AI may do now** (`AvailableRequestTypes`: produced by its loadout and usable this
  decision) and how long it has (`TimeBudgetSeconds`).
- **MinMax is a tool**: `ConnectItMinMaxSearch` / the Blueprint node *Find Best Moves (MinMax)* with a settings struct (depth,
  time, weights, each side's move set). The MinMax strategy is a thin wrapper.
- **A coordinator is a strategy** holding child strategies: collects their candidates within its time budget, ranks them with an
  overridable judge (default: child priority order), answers with the best.
- **Strategies may declare what they can play** (`GetSupportedRequestTypes`); an AI profile warns when its loadout and strategy
  don't fit. Validation only.
- **Loadouts stay on the PlayerState** (server-enforced, replicated); showing them to the player is the UI's choice.
- **An AI director on the match preset** changes the AI's loadout and strategy on match events (its turn starting / ending,
  either side's win checkpoints). It lives with the preset because the checkpoints are the preset's.

## Why

The owner wants MinMax usable from designers' own strategies, strategies that are generic and mix-and-match, and an AI that can
change during a match (alternating moves, new moves at a checkpoint) -- which also enables gameplay "tells". Keeping
strategies side-effect free is what makes a coordinator safe: it asks several and discards most answers. A ranked list makes a
rejected move cheap to recover from without re-asking. Putting changes to the AI on a director (data + Blueprint events) means
nobody scripts the controller, which stays the networking-sensitive plumbing.

## What Would Change It

A need for the AI to commit to its next move before the player's turn (move-level tells beyond "which kinds of move it has");
competitive play needing the AI's loadout genuinely secret (owner-only replication); or the MinMax search learning moves other
than Place Piece (a separate task).
