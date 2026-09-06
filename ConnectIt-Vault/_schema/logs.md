# _schema/logs.md

A `logs/` folder is an **append-only record of automated maintenance passes** over its
section or the vault. Split two ways:

- **`logs/raw/`** — the full, immutable entry for every run. The real history.
- **`logs/wiki/`** — a short summary of each run, plus a rolling index, for quick context
  at the start of a session.

## Layout

```
logs/
  raw/<source>/<YYYY-MM-DD-HHMM>.md    full entry, one per run, never edited after writing
  wiki/<source>/<YYYY-MM-DD-HHMM>.md   summary of that same run (same filename)
  wiki/<source>/README.md              rolling index for that source, newest first
```

`<source>` is the tool or skill that produced the run. One subfolder per source.

## Rules

- **`logs/raw/` is immutable.** Never edit or delete an entry after it's written —
  corrections go in the next entry.
- Every `logs/raw/<source>/X.md` has a matching `logs/wiki/<source>/X.md`.
- `logs/wiki/<source>/README.md` has one line for every run.
- `<timestamp>` = `YYYY-MM-DD-HHMM` (local, 24h). Date-only is fine if the time is
  unknown; add `-2`, `-3` for same-minute collisions.
- Created on first use — the producing tool makes the folders if they're absent. Don't
  pre-scaffold.

## Deviations

Add `logs/README.md` starting `Extends _schema/logs.md. Differences:` if a section logs a
second source or needs a different retention rule.
