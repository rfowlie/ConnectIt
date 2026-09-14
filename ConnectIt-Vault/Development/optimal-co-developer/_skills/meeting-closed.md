---
name: meeting-closed
description: >-
  Closes a check-in opened by /meeting-open — triggers on "/meeting-closed", "close the
  meeting", "end the check-in", "wrap up this meeting". Reads this conversation's
  transcript back to where /meeting-open ran, writes it verbatim plus a structured summary
  (overview, key points, insights, decisions, tasks, next step) to a dated
  optimal-co-developer/_meetings/ note per _schema/_meetings.md, surfaces any settled
  non-obvious call as a candidate _decisions/ note, and lists any real tasks discussed as
  candidates for the domain's _tasks/active.md — never adding task rows itself. Paired
  with /meeting-open.
---

# Meeting Closed

The closing half of the check-in opened by [`meeting-open`](meeting-open.md). Same
principle as [`session-close`](../../_skills/session-close.md): the owner externalizes
decisions but not state (`me/user.md`), so the conversation gets written down rather than
carried in memory. Produces one artifact of record — the verbatim conversation plus a
synthesis — filed under `_meetings/`.

## Inputs to read first

1. **This meeting's transcript** — the conversation since `/meeting-open` ran this
   session. If `/meeting-open` never ran this session (the owner jumped straight to
   `/meeting-closed`), say so, use the check-in conversation that did happen, and flag the
   scope assumption in the output.
2. **[`_core/_schema/_meetings.md`](../../../_core/_schema/_meetings.md)** +
   **[`_core/_templates/TSchemaMeetings.md`](../../../_core/_templates/TSchemaMeetings.md)**
   — the base note shape.
3. **[`_meetings/__README.md`](../_meetings/__README.md)** — this domain's extension of
   the base shape (expanded summary sections + verbatim transcript; see below). If it
   hasn't been extended yet, extend it per "Deviation to maintain" before writing the
   first note under the new shape.
4. **The domain the meeting was actually about** — usually
   [`ConnectIt/_tasks/active.md`](../../../ConnectIt/_tasks/active.md) and
   [`ConnectIt/_decisions/`](../../../ConnectIt/_decisions/__README.md), but use
   `optimal-co-developer/_tasks/` / `optimal-co-developer/_decisions/` instead if the
   check-in was about the collaboration itself rather than the game. If it's genuinely
   mixed, say so and split candidates by domain in the output.

## Deviation to maintain

`_meetings/__README.md` should read:

> Extends `_schema/_meetings.md`. Differences: a `meeting-open`/`meeting-closed` note adds
> `## Overview`, `## Key Points`, `## Insights`, and a verbatim `## Transcript` section
> (never edited after writing, same rule as `_discussions/`'s transcript section) on top of
> the base template's `## Actions` / `## Decisions` / `## Next`. `attendees:` for these
> notes is always `Owner` and `Co-developer` as plain strings, not `_people/` links — this
> is an internal check-in, not a call with a third party the `_people/` CRM is for.

If the file doesn't already say this, update it as part of this run (it's a `__README.md`
deviation note, not a guarded `_schema/` file).

## Steps

1. Derive today's date and a short kebab-case slug (e.g. `check-in`, or the dominant topic
   if one stands out); check `optimal-co-developer/_meetings/` for a same-day collision
   and add `-2` if needed.
2. Extract, from the transcript alone — never invent or pad:
   - **Overview** — 2–4 sentence plain-language recap.
   - **Key Points** — grouped by the six question categories from `/meeting-open`
     (Progress, Roadblocks, Decisions, Morale, Priorities, Other); omit any group that
     genuinely didn't come up.
   - **Insights** — anything non-obvious that surfaced (a pattern connecting two answers, a
     contradiction with the last meeting note). Omit if there's honestly nothing beyond
     the key points.
   - **Decisions** — calls actually settled during the conversation, as bullets.
   - **Tasks** — concrete work items raised, as checkboxes with owner if stated.
   - **Next** — the follow-up step and date, from the "anything else" / priorities
     discussion.
3. **For each settled decision that's non-obvious** (would otherwise get silently
   re-argued later): write it as its own dated note in the relevant domain's
   `_decisions/` per `_core/_schema/_decisions.md` / `TSchemaDecisions`, and link it from
   this meeting note's `## Decisions`. A decision that's just a restatement of something
   already settled doesn't need a new note — link the existing one instead.
4. **For each real task raised, list it as a candidate — do not add it to any
   `_tasks/active.md` yourself.** `_core/_schema/_tasks.md` is explicit: the owner enters
   and prioritises tasks by hand. Present the candidates in the output (below) so the
   owner can add what they want, where they want it.
5. Write `## Transcript` with the full conversation since `/meeting-open`, completely
   verbatim — no cleanup, no paraphrase, no trimming.
6. Save `optimal-co-developer/_meetings/<YYYY-MM-DD>-<slug>.md`.
7. Log the run per the vault's standing rule: `_core/_logs/<YYYY-MM-DD-HHMM>.md` per
   `TSchemaLogs`, scope `vault` (or the domain if everything about this run was
   domain-local), and add its line to `_core/_logs/__INDEX.md`.

## Output format

### 1. Filed
The saved meeting note's path, and any new `_decisions/` note path(s).

### 2. Summary
The Overview, verbatim from the note.

### 3. Task candidates
Each task raised, one line, with which `_tasks/active.md` it'd belong in — explicitly
framed as "add these yourself if you want them tracked," not as already filed.

### 4. Flags
Anything thin, ambiguous, or assumed (e.g. no `/meeting-open` this session, a domain split
between game and collaboration topics) — and the log entry's path.

## Style

- Terminal markdown, no preamble — start with section 1.
- Never fabricate a decision, task, or insight that isn't actually in the transcript.
- `## Transcript` is sacred, same as `_discussions/`'s — verbatim, never summarized.
- Don't pad a thin section; a short meeting produces a short note.

## See also

Paired with [`meeting-open`](meeting-open.md).
