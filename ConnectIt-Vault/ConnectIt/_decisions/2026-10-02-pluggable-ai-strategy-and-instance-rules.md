---
Date: 2026-10-02
status: Partly superseded
superseded by: 2026-10-02-minmax-evaluation-as-editor-terms (the rules-variant part)
tags:
  - ai
  - minmax
  - architecture
---
## Decision

> **Partly superseded (same day):** variants no longer subclass the rules — evaluation/ordering are editor term
> lists; see [2026-10-02-minmax-evaluation-as-editor-terms](2026-10-02-minmax-evaluation-as-editor-terms.md).
> The pluggable `UConnectIt_AIStrategy` part stands.

**How an AI picks moves is a per-level, inline-instanced `UConnectIt_AIStrategy`** on
`UConnectIt_LevelConfigDataAsset::AIStrategy` (same pattern as the scoring / win / placeable rules).
`AConnectIt_AIController` is strategy-agnostic: it builds a decision context on the game thread, calls the strategy,
applies a minimum think time and a stale-result guard, and submits the returned decision (request type + payload) as
the loadout action whose `ProducesRequestType` matches. `UConnectIt_AIStrategy_MinMax` is the Classic implementation;
`FConnectItAIDifficulty` is gone (its fields live on the MinMax strategy).

**The MinMax search takes a const rules instance** (`TAlphaBeta<TGame>::Run(const TGame&, Root, Params)`), not static
traits. `FConnectItClassicSearchGame` holds the per-search constants (connect length, win threshold, geometry,
editor-exposed `FConnectItMinMaxEvalWeights`); its search functions are virtual, so a variant subclasses it and
overrides only what differs; `UConnectIt_AIStrategy_MinMax::MakeRules` is where a variant is plugged in. Per-function
runtime delegates were considered and not adopted.

Namespaces: `GameIntelligence::Search` (shared async + cancel plumbing) and `GameIntelligence::Search::MinMax`
(alpha-beta). `TGame` keeps its name.

## Why

The owner expects some levels to want a non-MinMax, bespoke opponent; a strategy object makes that a new subclass
(C++ or Blueprint) chosen on the level config, with no controller changes. This **refines** the 09-24
[scope decision](2026-09-24-minmax-scoped-to-classic-adventure-gets-bespoke-ai.md)'s "bespoke per-level AI
controllers" into "bespoke per-level strategies on one controller" -- a controller subclass is still possible if a
level ever needs turn handling to differ.

Instance rules give runtime configuration a home (weights, geometry) and let variants reuse everything they don't
change. Per-function delegates would be called millions of times per decision (no inlining), and Blueprint-bindable
delegates can't run off the game thread at all.

## What Would Change It

A strategy that needs something the controller can't provide through the context (e.g. visibility of other actors, or
a non-board action); a measured virtual-dispatch cost in the search that matters (the variant hook would move to a
template parameter); or score-window work (open task) changing what root results look like.
