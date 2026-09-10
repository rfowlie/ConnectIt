# _schema/_logs.md

A `logs/` folder is an **append-only record of automated maintenance passes** over a
domain — or, at `_core/_logs/`, over the whole vault. **One note per run**, flat in the
folder. This is machine-written history; the human's own day-to-day notes go in `_daily`,
not here.

## Where it mounts

- **`_core/_logs/`** — the vault-wide log: passes that touch the whole vault or cross
  domains.
- **`<domain>/logs/`** — a single domain's log: passes scoped to that domain.

## Filename

`logs/<YYYY-MM-DD-HHMM>.md` — local, 24-hour. Date-only (`<YYYY-MM-DD>.md`) is fine when
the time isn't known; add `-2`, `-3` for same-timestamp collisions.

## What a note holds

Frontmatter:

- `source:` — the skill or process that produced the run.
- `run:` — the `YYYY-MM-DD-HHMM` timestamp.
- `scope:` — `vault`, or the domain name.
- `tags:` — at least `log`.

Body (see [[TSchemaLogs]]): **Request** (one line — what was asked for) · **Overview**
(what was actually done, and why) · **Created** (`[[wikilinks]]` to new notes/files) ·
**Changed** (`[[wikilinks]]` to changed notes, each with a short what-changed) ·
**Flags & Follow-ups** (assumptions, gaps, offered-but-not-done, decisions, anything to
revisit).

## Rules

- **Immutable once written** (the strict form of the **dated event note** convention, see
  `_schema/README.md`) — never edit or delete an entry after the run; corrections go in
  the next note.
- **One note per run.** No `raw/` / `wiki/` split, no per-source subfolders — just dated
  notes in the folder.
- Created on first use — the writing process makes the folder if it's absent. Don't
  pre-scaffold.
- Optional: a `logs/README.md` may carry a newest-first one-line index for quick
  start-of-session context.

## Template

[[TSchemaLogs]]

## Deviations

Add `logs/README.md` starting `Extends _schema/_logs.md. Differences:` if a domain needs
a different retention rule or extra frontmatter. (A plain rolling index in that same file
is not a deviation.)
