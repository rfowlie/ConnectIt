---
name: process-discussion
description: >-
  When the user has a transcript — pasted text, or a path to a text file holding one — from
  a call, brainstorm, or recorded conversation (e.g. a transcribed voice memo) and wants it
  filed as a _discussions/ note. Extracts a summary plus Tasks/Deliverables/Questions/
  Updates/Workflows, links each topic discussed back to the most recent prior
  _discussions/ note that covered it, and preserves the transcript verbatim. Use for
  "process this transcript", "/process-discussion", "file this conversation", "make a
  discussion note from this".
---

# Discussion Process

Turn a transcript into a dated `_discussions/` note per `_schema/_discussions.md` —
summarized at the top, topic-linked to prior discussions, transcript preserved verbatim at
the bottom.

## Purpose

`_schema/_discussions.md` defines a note shape but doesn't populate itself — this skill is
the mechanical step that takes one transcript and turns it into a compliant note: derive
its topics, resolve each one against the folder's existing notes to find the most recent
prior mention, extract the actionable content, and never touch the source text itself.
Never fabricates a task, question, or topic that isn't actually in the transcript.

## Inputs to read first

1. [`_core/_schema/_discussions.md`](../_schema/_discussions.md) — the section's rules and
   note shape.
2. [`_core/_templates/TSchemaDiscussions.md`](../_templates/TSchemaDiscussions.md) — the
   exact template to fill.
3. The target `_discussions/` folder's existing notes (if any) — read each one's `topics:`
   frontmatter, needed to resolve this note's `## Topics` backward links.

## Arguments

- The transcript itself: pasted text, or a path to a file holding it (read the file in
  full — never summarize from a partial read).
- Which domain's `_discussions/` to file into. If not stated and it isn't obvious from
  context (e.g. the conversation was clearly about one specific domain), ask which domain,
  or whether it belongs at the vault-wide `_core/_discussions/`.

If the transcript is missing entirely, say so and stop — don't invent one.

## Steps

1. Derive a date (from the transcript's own content if it states one, else today) and a
   short kebab-case slug for the filename; check the target `_discussions/` folder for a
   same-day collision and add `-2` etc. if needed.
2. Identify the distinct topics discussed.
3. For each topic, scan the target `_discussions/` folder's existing notes (most recent
   filename date first) for one whose `topics:` frontmatter lists the same topic. Link to
   the first match found; if none, mark it "first time this came up."
4. Fill `## Summary`, `## Tasks` (checkboxes, owner if the transcript states one),
   `## Deliverables`, `## Questions` (genuinely open/unresolved), `## Updates`
   (status/progress that isn't a task or question), and `## Workflows` (a process
   described, adopted, or changed) from the transcript's actual content. Omit any section
   with nothing real to put in it — don't pad.
5. Write `## Transcript` with the full input text, completely verbatim — no edits,
   paraphrasing, or trimming.
6. Save `<domain-or-core>/_discussions/<date>-<slug>.md`. Update the folder's
   `__INDEX.md` if one exists.
7. Log the run per the vault's standing rule: write an `_core/_logs/<YYYY-MM-DD-HHMM>.md`
   note (scope `vault`, or the domain if the note landed at domain-level) per
   `_core/_templates/TSchemaLogs.md`, and add its line to `_core/_logs/__INDEX.md`.
   Confirm the path.

## Output format

### 1. Filed
The saved note's path.

### 2. Topics resolved
A short list: each topic named, and what it linked to (the prior note's path, or "first
time this came up").

### 3. Flags
Anything thin, ambiguous, or omitted from the transcript (e.g. an unclear owner on a task,
a topic that might overlap an existing one under a different name) — and the log entry's
path.

## Style

Terminal markdown, no preamble — start with the first real section. Never fabricate a
task, question, topic, or quote that isn't actually in the transcript; flag low-confidence
extractions rather than guessing. The `## Transcript` section is sacred — verbatim, never
summarized or cleaned up.
