# _schema/_decisions.md

A `decisions/` folder holds **one dated note per non-obvious decision**, so a settled
call doesn't get silently re-argued later. It's the "why we decided X" trail, not the
place for the decision's full context — that lives in the area's main reference file.

## Filename

`<YYYY-MM-DD>-<short-slug>.md` — e.g. `2026-09-05-stay-solo-for-now.md`

## Template

[[TSchemaDecisions]]

## Rules

- Follows the **dated event note** convention (see `_schema/README.md`): dated filename,
  append-only, never rewritten.
- A superseded note is marked `Status: superseded by [[<newer-slug>]]` — not deleted.
- One decision per file. If a session settles three things, that's three notes.

## Deviations

Add `decisions/README.md` starting `Extends _schema/_decisions.md. Differences:` if a
domain needs extra frontmatter or a different slug scheme.
