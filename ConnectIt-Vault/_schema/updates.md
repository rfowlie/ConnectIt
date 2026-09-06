# _schema/updates.md

An `updates/` folder is the **change record for one code unit** — e.g. a plugin, library,
or module. It answers "what changed here, and why does it matter," in two forms:

- **`updates/raw/`** — the source material for each change, captured verbatim.
- **`updates/wiki/`** — a synthesised, human-readable write-up of that same change.

It sits next to that code unit's `high-level.md`. `updates/` keeps the dated trail;
durable facts from an update get folded back into `high-level.md`.

## Layout

```
updates/
  raw/<YYYY-MM-DD-slug>.md     source material for one change, immutable once written
  wiki/<YYYY-MM-DD-slug>.md    synthesis of that same change (same filename)
```

- `<slug>` is kebab-case, a few words: `board-manager-refactor`, `score-board-setup`.
- Same-day collisions: add `-2`, `-3`.
- **Created on first use.** Don't pre-scaffold empty `raw/` and `wiki/` folders.

## Rules

- **`raw/` is verbatim and immutable.** Paste changelogs, `git log` / `git diff` output,
  commit ranges, PR text, design-conversation transcripts, scratch notes — as-is. Never
  tidy or summarise in place; corrections go in a later entry.
- **Every `raw/X.md` has a matching `wiki/X.md`** (same filename). A new raw entry means a
  new wiki entry.
- **No wiki claim without raw support.**
- **Fold durable facts into `high-level.md`.** If an update changes the standing picture
  of the code unit (API, status, dependency), update its `high-level.md` too — `updates/`
  is the trail, `high-level.md` is the current state.

## Templates

### `raw/<YYYY-MM-DD-slug>.md`

```markdown
---
date: <YYYY-MM-DD>
slug: <short-slug>
unit: <code unit name>
---

# <YYYY-MM-DD> — <short title>

## Source

- Type: <commit range | PR | design conversation | manual notes>
- Reference: <hash range / PR link / etc.>

## Verbatim

<Paste the raw material here — logs, diffs, transcript, notes. Do not rewrite it.>
```

### `wiki/<YYYY-MM-DD-slug>.md`

```markdown
---
date: <YYYY-MM-DD>
slug: <short-slug>
unit: <code unit name>
raw: "[[raw/<YYYY-MM-DD-slug>]]"
---

# <YYYY-MM-DD> — <short title>

## Summary

<One or two sentences: what this update is.>

## What changed

- <bullet>

## Why it matters

<Impact on the code unit's behaviour, its consumers, or how we build on it.>

## Follow-ups

- [ ] <anything left open>

## Links

- Raw: [[raw/<YYYY-MM-DD-slug>]]
- Related: <in-repo paths, other notes>
```

## `updates/` vs `logs/`

Both split raw / wiki, but they track different things:

- **`logs/`** ([`_schema/logs.md`](logs.md)) — append-only record of *automated
  maintenance passes* over the vault, keyed by the tool that produced them.
- **`updates/`** — *manually* recorded evolution of a *code unit*, keyed by date.

A code unit has an `updates/` folder; it does not have a `logs/` folder.

## Deviations

A code unit whose `updates/` needs different rules adds `updates/README.md` starting
`Extends _schema/updates.md. Differences:` and lists only the deltas.
