# _schema/_discussions.md

A `_discussions/` folder holds **one dated note per discussion** — a call, brainstorm, or
transcribed conversation. It's organized by **topic-threading**, not subfolders: each note
names the topics it covers and links each one back to the most recent prior
`_discussions/` note that also covered it, so the folder can stay flat without losing the
thread of any one topic.

## Structure

```
_discussions/
  __INDEX.md               (optional — newest-first table, per TIndex)
  <YYYY-MM-DD>-<slug>.md    (one per discussion; -2, -3 for same-day collisions)
```

## Where it mounts

Either — root (`_core/_discussions/`) or per-domain (`<domain>/_discussions/`), like most
sections.

## What a note holds

Frontmatter:

- `date:` — the discussion's date.
- `source:` — where it came from, when there's an external origin (an audio file, a call
  platform). Omit if the discussion has no such source.
- `topics:` — a flat list of topic slugs discussed, used to resolve the next note's
  backward links.
- `tags:` — at least `discussion`.

Body (see [[TSchemaDiscussions]]): **Summary** (a short plain-language recap) · **Topics**
(each topic named, linked to the most recent prior `_discussions/` note that covered it, or
marked as the first time it came up) · **Tasks** (checkboxes, owner if stated) ·
**Deliverables** · **Questions** (open, unresolved) · **Updates** (status/progress that
isn't a task or question) · **Workflows** (a process described, adopted, or changed) ·
**Transcript** (optional — the full verbatim source text, when one exists).

## Rules

- Follows the **dated event note** convention (see `_schema/__README.md`): dated filename,
  append-only, never rewritten. A correction goes in a new note, not an edit to an old one.
- **Flat folder, no subfolders.** Topic continuity comes from the `## Topics` links, not
  from filing notes into per-topic folders — this is what keeps the folder from needing
  organization decisions up front.
- Every topic named in a note's `## Topics` section must link to the most recent earlier
  `_discussions/` note whose `topics:` frontmatter also lists it (resolved by scanning the
  folder's existing notes, most recent filename date first) — or say "first time this came
  up" if no earlier note covered it.
- `## Transcript`, when present, is verbatim — never edited, paraphrased, or trimmed after
  writing.
- **Vs. `_meetings`:** `_meetings` is for attendee-tracked calls with formal action items.
  `_discussions` is looser — any recorded or ad hoc conversation, organized by
  topic-threading rather than attendee-tracking. When a discussion is really a
  formal, attendee-tracked meeting, prefer `_meetings` instead.
- Created on first use; don't pre-scaffold.
- Optional: a `_discussions/__INDEX.md` (per [[TIndex]]) may carry a newest-first one-line
  index for quick start-of-session context.

## Template

[[TSchemaDiscussions]]

## Deviations

Add `_discussions/__README.md` starting `Extends _schema/_discussions.md. Differences:` if
a domain needs a different slug scheme or extra frontmatter.
