# _schema/_meetings.md

A `meetings/` folder holds **one dated note per meeting or call** — what was discussed,
what was agreed, what happens next.

## Filename

`meetings/<YYYY-MM-DD>-<short-slug>.md` — e.g. `meetings/2026-09-05-publisher-intro.md`.
Add `-2` for a second meeting the same day with the same slug.

## What a note holds

- Date, attendees (each linked to their `people/` note), and the domain/project it
  belongs to.
- Notes taken during the meeting.
- Action items, with owners.
- Next step / follow-up date.

## Rules

- Follows the **dated event note** convention (see `_schema/__README.md`): dated filename,
  append-only, one per file.
- **A meeting that settles a non-obvious call also gets a `decisions/` note** — the
  meeting note records the discussion, the decision note records the "why".
- Attendees are always links, never bare names.

## Template

[[TSchemaMeetings]]

## Deviations

Add `meetings/__README.md` starting `Extends _schema/_meetings.md. Differences:` if a
domain needs a different slug scheme or extra frontmatter.
