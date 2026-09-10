# _schema/_user.md

A `user/` folder holds everything about the owner: role, history, status, constraints,
goals, strengths, blockers, working style.

## Canonical file

`user/profile.md` — the structured self-portrait, updated in place. Covers who the owner
is, what they're working toward, how they work best, and what trips them up.

## Decisions

`user/decisions/` holds one dated note per non-obvious owner-level call (see
[`_schema/_decisions.md`](_decisions.md)).

## Splitting the profile

If "what is true about the owner" and "how to work with the owner" grow large enough to
separate, split them into two files (e.g. `user/profile.md` and `user/working-style.md`).
When they overlap: **facts win on what is true; working-style wins on how to act on it.**
Each file carries a one-line pointer to the other.

## Template

[[TSchemaUser]]

## Deviations

Add `user/README.md` starting `Extends _schema/_user.md. Differences:` if a domain needs
extra structure or fields.
