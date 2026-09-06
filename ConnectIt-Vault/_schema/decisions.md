# _schema/decisions.md

A `decisions/` folder holds **one dated note per non-obvious decision**, so a settled
call doesn't get silently re-argued later. It's the "why we decided X" trail, not the
place for the decision's full context — that lives in the section's lead doc.

## Filename

`<YYYY-MM-DD>-<short-slug>.md` — e.g. `2026-01-15-drop-legacy-path.md`

## Template

```markdown
# <decision, stated plainly>

**Date:** <YYYY-MM-DD>
**Status:** active | superseded by [[<newer-slug>]]

**Decision:** what was chosen.

**Why:** the reasoning, and the alternatives weighed against it.

**What would change it:** the condition or evidence that should trigger a revisit.
```

## Rules

- **Append-only in spirit.** Don't rewrite an old note — write a new one and mark the old
  one `Status: superseded by [[<newer-slug>]]`.
- **Keep it short.** Detail that belongs in the main picture for this area goes in the
  section's canonical file. This folder is only the decision trail.
- One decision per file. If a session settles three things, that's three notes.

## Deviations

Add `decisions/README.md` starting `Extends _schema/decisions.md. Differences:` if a
section needs extra frontmatter or a different slug scheme.
