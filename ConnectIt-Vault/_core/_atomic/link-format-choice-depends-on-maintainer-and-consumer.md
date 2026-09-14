---
created: 2026-09-11
status: baby
tags:
  - atomic
  - obsidian
---

# Pick a link format by who keeps it correct and who reads it, not by which is "standard"

Two link mechanisms can both separate a link's target from its display text (see
[[wikilink-aliases-separate-target-from-display]]) and still be the wrong and right
choice for the same vault at different times. The question that actually decides it
isn't "which is more standard" — it's two narrower ones: **who keeps the target
accurate as files move**, and **who has to resolve the link to act on it**.

Obsidian wikilinks answer the first question well: the editor rewrites a `[[target]]`
automatically when you rename or move the file it points to, so the link stays correct
with zero manual upkeep. Plain Markdown links answer the second question well: the
target is a literal, explicit path — directly usable by anything that reads the raw
file, no vault-aware resolver required. Neither answer is free. Wikilinks cost you
portability (a plain-text or non-Obsidian reader sees inert `[[...]]` and can't act on
it without first searching the vault by name); Markdown links cost you upkeep (nothing
rewrites the path when the target moves, so it silently rots until someone or something
checks it).

Which cost is worth paying changes with two things: how often files actually move, and
who's reading the links. Early in a vault's life — heavy restructuring, folders renamed
and merged every session — the self-healing property matters most, because a manually
maintained path would rot constantly. This vault ran that way for a while and wikilinks
were the right call. But an AI co-developer reading the vault directly from disk (not
through Obsidian) can't use that self-healing property at all — it has to search the
vault by name to resolve `[[target]]`, the same cost a human pays reading it outside
Obsidian. Once the structure stabilizes and moves become rare, that ongoing resolution
cost stops being worth paying for a self-healing benefit you're rarely cashing in. That's
the actual trade this vault made 2026-09-11: switched its `CLAUDE.md` files from
wikilinks to Markdown links once `_core/` stopped changing shape every session, because
the primary "reader" that benefits from a link — an agent working the file tree
directly — benefits more from an explicit path than from move-safety it wasn't
exercising much anymore.

The transferable rule: don't ask "which linking convention is better" in the abstract.
Ask who reads the links (a GUI with a resolver, or something reading raw text/paths) and
how often the underlying thing being linked to actually moves. The answer to those two
questions picks the mechanism — and it can flip as a project matures, the way it did
here.

## References

- Related: [[wikilink-aliases-separate-target-from-display]]
- Source: the 2026-09-11 vault-wide switch to Markdown links, recorded in `_core/CLAUDE.md`
  and `ABOUT.md`.
