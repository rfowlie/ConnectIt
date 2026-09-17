---
name: process-atomic
description: >-
  When the user wants the vault's atomic-notes pool kept in order. Two passes: (1)
  enforce the single-pool rule — find any atomic/ or _atomic/ folder that isn't
  _core/_atomic/, move its notes in, delete the invalid folder; (2) sweep every _data/
  wiki/ note not yet marked atomic-summary: true, extract atomic-note-worthy ideas,
  link/update an existing atomic note or create a new one, then mark the wiki note swept.
  Use for "process atomic", "/process-atomic", "check for duplicate atomic folders",
  "merge atomic notes", "sweep the wikis for atomic notes".
---

# Process Atomic

Two passes over the vault's atomic-notes machinery: first enforce
`_schema/_atomic.md`'s single-pool rule, then keep that pool fed from `_data/wiki/`.

## Purpose

`_schema/_atomic.md` says a second atomic folder anywhere is an error to be merged and
deleted, but nothing checked for one — Pass 1 is that check, run as a fix. Separately,
`_schema/_data.md` now tracks whether a `wiki/` note has been swept for ideas worth
promoting to `_core/_atomic/` (its `atomic-summary` field) — Pass 2 is the sweep that
field exists for: find every un-swept note, extract what's genuinely there, link or
create atomic notes accordingly, and mark it done. Neither pass invents an idea, a
collision resolution, or a merge that isn't actually supported by what's on disk.

**Ordering:** `/process` always runs `process-data` before this skill — Pass 2 depends on
`_data/wiki/` notes (and their `atomic-summary` field) being current. Running
`/process-atomic` on its own is fine too; it just sweeps whatever `wiki/` notes exist at
the time.

## Inputs to read first

1. Every folder in the vault named `atomic` or `_atomic` — glob for both, at any depth
   (Pass 1).
2. [`_core/_atomic/`](../_atomic/) — the canonical pool: for Pass 1, to check filename
   collisions before moving anything in; for Pass 2, to check whether an idea already has
   a note before creating a duplicate.
3. [`_schema/_atomic.md`](../_schema/_atomic.md) — the note shape (Pass 1's sanity check
   before moving a file) and, for Pass 2, the "what becomes an atomic note" rule (a
   fleshed-out source insight or a recurring theme — never an invented one) and the word
   cap (~1000 words).
4. Every `**/_data/wiki/*.md` in the vault (Pass 2) — per
   [`_schema/_data.md`](../_schema/_data.md) / [`TSchemaWiki`](../_templates/TSchemaWiki.md).

## Arguments

- None. This always sweeps the whole vault — a partial run would leave Pass 1's invariant
  ("exactly one pool") unverified, and Pass 2's `atomic-summary` bookkeeping inconsistent.

## Pass 1 — consolidate duplicate atomic folders

1. List every `atomic/` or `_atomic/` folder found that is **not** `_core/_atomic/`.
   If none, say so and move on to Pass 2.
2. For each offending folder, for every file in it (skip its own `__README.md` — that's
   folder-specific documentation, not a note to merge):
   - If no file of that name exists in `_core/_atomic/`, move it there unchanged.
   - If a file of that name **already exists** in `_core/_atomic/`, do **not** overwrite
     either copy — leave both in place and flag the collision for the user to resolve by
     hand.
3. After moving what it safely can, delete the offending folder **only if it is now
   empty**. If collisions left files behind, leave the folder (now holding only the
   collided files) and say so explicitly.
4. Never touch `_core/_atomic/`'s own contents beyond adding the moved-in files.

## Pass 2 — sweep `_data/wiki/` for atomic notes

1. Glob every `**/_data/wiki/*.md` in the vault.
2. Skip any note whose `atomic-summary` is already `true`. A note **missing** the field
   entirely (predates `TSchemaWiki`) is treated as `false` — sweep it, and flag that it
   predates the template so the user can backfill the rest of its frontmatter if they
   want.
3. For each note to sweep, read it in full and identify each distinct idea in it that's
   genuinely atomic-note-worthy per `_schema/_atomic.md` — a fleshed-out source insight or
   a recurring theme, never a padded-out restatement of the whole note and never an idea
   the note doesn't actually contain. A note can legitimately yield zero ideas.
4. For each idea found, scan `_core/_atomic/` for an existing note already covering it
   (by title and topic tags — same "resolve against what's already there" approach
   `/process-discussion` uses for topic-continuity):
   - **Match found:** link it from the wiki note's `## Atomic notes swept` section. If
     this source adds genuinely new, relevant information the existing atomic note
     doesn't already have, fold it in — updating the note in place while keeping it one
     idea, self-contained, and under the ~1000-word cap (trim rather than exceed it). Add
     this wiki note to the atomic note's `## References` if it isn't already listed there.
   - **No match:** create `_core/_atomic/<kebab-slug>.md` per
     [`TSchemaAtomic`](../_templates/TSchemaAtomic.md) — `created:` today, `status: baby`,
     topic tags from the idea, `## References` → `Source: [[<this wiki note>]]`. Link it
     from the wiki note's `## Atomic notes swept` section too.
5. Once every idea in a note has been resolved (linked or created), set that note's
   `atomic-summary: true` — even if step 3 found nothing to promote.
6. Never edit anything under `_data/raw/` — Pass 2 only ever writes to `_data/wiki/` notes
   and `_core/_atomic/`.

## Output format

### 1. Merged (Pass 1)
Per offending folder: `folder path → files moved`, and whether the folder was deleted.

### 2. Swept from `_data/` (Pass 2)
A table: `wiki note | ideas found | atomic notes created | atomic notes updated/linked`.
List only notes actually swept this run, plus a one-line total (`N wiki notes swept, M
atomic notes created, K updated`). "Nothing to sweep" is a valid result.

### 3. Flags
Every Pass 1 filename collision (both paths named, neither altered) and any offending
folder left behind because of one; every Pass 2 wiki note swept despite missing
`atomic-summary` (predates the template); any idea that was close to an existing atomic
note but ambiguous enough to need the user's judgment instead of an automatic merge.

### 4. Log
Write an `_core/_logs/<YYYY-MM-DD-HHMM>.md` note per `_core/_templates/TSchemaLogs.md`
(scope `vault`), and add its line to `_core/_logs/__INDEX.md`. Confirm the path.

## Style

Terminal markdown, no preamble. Never overwrite a file to resolve a Pass 1 collision —
flag it instead. Never delete a folder that still has files in it. Never fabricate an
idea, a topic match, or new information a wiki note doesn't actually contain. Never touch
`_data/raw/`. Respect `_schema/_atomic.md`'s one-idea-per-note and word-cap rules even
when updating an existing note, not just when creating one.
