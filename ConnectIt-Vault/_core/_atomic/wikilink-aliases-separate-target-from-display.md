---
created: 2026-09-11
status: child
tags:
  - atomic
  - obsidian
---

# A link's target and its display text are two independent things

Markdown gives you two ways to link one note to another, and both split the link into
jobs that don't have to agree: **what it resolves to**, and **what it looks like on the
page**.

```
[[ConnectIt/CLAUDE|ConnectIt]]        <- Obsidian wikilink
 ^^^^^^^^^^^^^^^^^  ^^^^^^^^^
 resolves to this    renders as this

[`board-of-advisors/`](board-of-advisors/CLAUDE.md)   <- standard Markdown link
 ^^^^^^^^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^^^^^^^^^^^^
 renders as this        resolves to this
```

Wikilink: everything before `|` is the target, matched by Obsidian against a note name
or vault-relative path; everything after is the alias, pure display text. Markdown link:
the same split, just reordered and spelled differently — `[display](path)`. Without an
alias, a link displays its target verbatim: fine when the target *is* a good label,
useless when it isn't. This vault names every domain's router file `CLAUDE.md` — the
filename is load-bearing (Claude Code auto-loads it), not descriptive — so a bare
`[[ConnectIt/CLAUDE]]` would resolve fine but render as the word "CLAUDE" in every row of
a nine-domain table. The alias keeps the target fully qualified while the visible text
carries the meaning.

**The two forms diverge on who keeps the target honest.** A wikilink's target is
resolved by Obsidian's own note-matching, and Obsidian rewrites it automatically when you
rename or move the target file. A Markdown link's target is a literal path string with no
such upkeep — nothing rewrites it if the file moves. Concrete evidence from this vault:
`Development/CLAUDE.md` uses Markdown links throughout, and several have gone stale
(`_schema/`, `game-projects/`, and a couple of single-word paths that don't match the
folder structure the note's own display text describes) — while the wikilink-based
`CLAUDE.md` routers survived several rounds of restructuring without rotting the same way.

The general shape, beyond either syntax: whenever a system forces a name to serve two
masters — a machine-resolvable identity and a human-readable label — and those two pull
in different directions, look for a seam that lets you specify them separately rather
than compromising on one name that's mediocre at both. A URL with anchor text, a variable
name with a UI-facing label, a file path with a "display name" field — same shape as the
target/alias split here. And when you get to choose which mechanism enforces that split,
prefer the one an editor keeps correct for you over the one that's only as accurate as
the last person who typed the path by hand.

## References

- Source: [[ConnectIt/CLAUDE]] — the Domains table is the concrete case that prompted this
  (nine `CLAUDE.md` routers, one per domain, each aliased to its domain name).
- Source: `Development/CLAUDE.md` — the Markdown-link comparison case, and the evidence
  that its un-rewritten paths have gone stale.
- Related: [[link-format-choice-depends-on-maintainer-and-consumer]] — which of the two
  to actually pick, and why that answer changed for this vault.
