# _schema/_sessions.md

A `_sessions/` folder holds **one dated note per work session** on the domain it mounts
in — bookended by `/session-open` (the map) and `/session-close` (the reckoning), so
state stops living only in the owner's head between sessions.

## Structure

```
_sessions/
  <YYYY-MM-DD-HHMM>.md    one per work session, Open half written first, Close half after
```

## Where it mounts

Per-domain, on whichever domain the session's work belongs to (e.g. `ConnectIt/_sessions/`)
— never vault-wide, since a session is always work on one specific project. The
`/session-open` / `/session-close` skills that produce these notes are authored generically
at `Development/_skills/` (the owner's cross-project operating practice) but write into the
target domain's own `_sessions/`, the same cross-domain-write shape `/process-discussion`
already uses for `_discussions/`.

## What a note holds

Frontmatter: `date:`, `tags:` (at least `session`).

Body, two halves:

- **`## Open — the map`** — written by `/session-open`: where we are, what's playable,
  what's off the table, the single next thing, what not to touch.
- **`## Close — the reckoning`** — written by `/session-close`, filled in later the same
  note: what changed, what's playable now, what was ruled out, the re-entry point, a
  deadline check. A placeholder (`_(written by /session-close)_`) until then.

## Rules

- Follows the **dated event note** convention (see `_schema/__README.md`): dated filename,
  append-only, one per file, never rewritten after both halves are filled.
- **One note per session** — if `/session-open` runs and finds the newest note's `## Close`
  already filled in, that session closed; start a new note rather than reopening it.
- A session note is a **reckoning, not a performance review** — position, not praise or
  blame (see `Development/_skills/session-close.md`).
- Created on first use; don't pre-scaffold.

## Template

[[TSchemaSessions]]

## Deviations

Add `_sessions/__README.md` starting `Extends _schema/_sessions.md. Differences:` if a
domain needs a different cadence or extra frontmatter.
