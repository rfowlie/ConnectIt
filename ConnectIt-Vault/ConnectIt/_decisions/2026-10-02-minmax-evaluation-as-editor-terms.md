---
Date: 2026-10-02
status: Active
superseded by:
tags:
  - ai
  - minmax
  - architecture
---
## Decision

What the MinMax AI values is **editor data on `UConnectIt_AIStrategy_MinMax`**: two lists of weighted, polymorphic
term structs -- `EvaluationTerms` (`TArray<TInstancedStruct<FConnectItMinMaxEvalTerm>>`) and `OrderingTerms`
(`FConnectItMinMaxOrderTerm`). The search's score = win/loss check, then Σ weight × term; move ordering likewise.
Designers pick, order and weight terms; programmers add new term structs in C++. The rules object
(`FConnectItClassicSearchGame` renamed **`FConnectItMinMaxRules`**) is an internal, `final`, non-virtual game model
built per decision -- not subclassed, not configured. The strategy's `MakeRules` hook is gone.

## Why

The previous shape made "change how the AI evaluates" a C++ rules subclass *plus* a strategy subclass overriding
`MakeRules` -- two classes for one idea, and nothing a designer could touch. The plain C++ rules layer has to exist
(the search runs off the game thread; a UObject can't be called there), but only evaluation and ordering actually
vary. Term structs are plain data: the rules copy them on the game thread and the search calls their virtual
functions directly -- no UObject, no reflection in the hot path, no config-to-runtime factory pair per type.
Blueprint-authored terms are deliberately impossible (thread safety and ~10^5–10^6 calls/s).

## What Would Change It

A need for a term to read data the board doesn't carry (it would need adding to the rules object or the decision
context), a profiler showing the per-term virtual calls matter, or Adventure needing a different game model (its own
rules class and probably its own strategy -- legitimately a new pair).

Supersedes the "variants subclass the rules and override virtual functions" part of
[2026-10-02-pluggable-ai-strategy-and-instance-rules](2026-10-02-pluggable-ai-strategy-and-instance-rules.md); the
pluggable `UConnectIt_AIStrategy` part of that note stands.
