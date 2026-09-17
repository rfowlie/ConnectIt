---
name: session-close
description: >-
  Run at the end of a work session on a game project — the two-minute reckoning.
  Captures what changed, what's playable now, what was ruled out, and the single
  next action, writes the session note's `## Close` half so state stops living
  only in the owner's head, and routes anything the session settled, left open,
  or discussed at length into that domain's `_decisions/`, `_questions/`, and
  `_discussions/` — without this step those don't get populated on their own.
  Use when the user says "session close", "/session-close", "wrap up", "done
  for today", or is about to stop.
---

# Session Close

The closing bookend. Force the reckoning so state is written down, not carried
(`Development/optimal-co-developer/me/user.md`: the owner externalizes decisions, not
state — and that gap is the core vulnerability).

This is a **reckoning, not a performance review.** Report the build's position. "Nothing
became playable" is a fact about the build, never a verdict on the owner. Every flag comes
with the fix in the same motion (`Development/optimal-co-developer/co-developer/soul.md`).

## Inputs to read first

1. **This session's transcript** — what was actually done, decided, tried, abandoned.
2. **The open session note** — the newest file in the active game domain's `_sessions/`
   (e.g. `ConnectIt/_sessions/`), the one whose `## Close` half is still a placeholder. If
   there's no open note (the session didn't run `/session-open`), create one now and fill
   both halves.
3. **The game domain's `CLAUDE.md`** — for structure/routing and its demo checklist, if
   it names one. It deliberately carries no "current state" or single-next-action section
   (removed 2026-09-14, and never reintroduced by this skill) — the reckoning below is
   what actually carries that information forward, in the session note and, where a real
   task was raised, as a candidate for `_tasks/active.md` per that section's own "the
   owner enters tasks by hand" rule.

## Arguments

- Optional game name after `/session-close`. Restate which game and which session note
  you're closing, in one line.

## Also read, for the routing step below

- [`_core/_schema/_decisions.md`](../../_core/_schema/_decisions.md) /
  [`TSchemaDecisions`](../../_core/_templates/TSchemaDecisions.md)
- [`_core/_schema/_discussions.md`](../../_core/_schema/_discussions.md) /
  [`TSchemaDiscussions`](../../_core/_templates/TSchemaDiscussions.md)
- [`_core/_schema/_questions.md`](../../_core/_schema/_questions.md) /
  [`TSchemaQuestions`](../../_core/_templates/TSchemaQuestions.md)

Nothing about the reckoning above needs these — they're for the routing step, so a
session's real output doesn't only exist as ad hoc notes created mid-conversation because
someone happened to ask or happened to notice. **Without this step, `_decisions/`,
`_questions/`, and `_discussions/` do not get populated on their own** — closing the
session bookends `_sessions/` correctly, nothing more, unless this step actually runs.

## The reckoning

Walk these with the owner, confirm or correct each, then write:

1. **What changed** — 2–4 concrete bullets: files, systems, decisions. Not effort, not
   hours.
2. **Playable now** — what a stranger could do that they couldn't at open. If unchanged,
   say so plainly. If three closes in a row say "unchanged", name it once — with the next
   demo checklist item as the proposed fix, not as a complaint. (Tell from `user.md`:
   active daily, nothing becomes playable.)
3. **Ruled out this session** — anything tried and killed, and why. This is the record
   that stops a discarded idea being re-suggested next session.
4. **Re-entry point** — the single next action, stated concretely. If a couch problem is
   open (a scout briefing not yet acted on, a decision not yet made), that is the re-entry
   point — name it as such.
5. **Deadline check** — is the demo date / runway still in view, and has it gone
   unmentioned for a while? If so, surface it once, plainly. No lecture.

## Route decisions, questions, and discussions

Run this every close, on whichever domain the session's work belongs to (the same domain
as the session note itself) — this is the step that keeps `_decisions/`, `_questions/`,
and `_discussions/` from only ever growing when someone happens to remember mid-session.
Walk the transcript once, specifically for these three shapes (independent of the
reckoning above — a session can have zero, one, or several of each):

1. **Non-obvious settled decisions** — an architecture/design call actually *made* this
   session, not just discussed, that would otherwise get silently re-argued later. For
   each: write it as its own dated note in that domain's `_decisions/` per
   `_core/_schema/_decisions.md` / `TSchemaDecisions`, same bar and shape
   `/meeting-closed` already uses. A decision that's just a restatement of something
   already settled doesn't need a new note — link the existing one in the reckoning
   instead. Write these directly; don't ask first — same as `/meeting-closed`, this is
   low-risk (append-only, dated, never silently overwrites anything).
2. **Genuinely open, unresolved threads** worth tracking past this session — not
   everything discussed qualifies, only something that would otherwise just get
   forgotten and rediscovered later. List these as **candidates** in the closing output
   (name each, one line, with why it seems open) — do **not** create the `_questions/`
   note yourself. Judging "is this actually worth a persistent note" is exactly the kind
   of call that's easy to get wrong in either direction (missing a real one, or creating
   clutter for something that was never really unresolved) — the owner decides.
3. **A real extended Q&A or technical exchange** worth preserving close to verbatim (the
   shape `_discussions/` holds) — not routine back-and-forth, something with actual
   reasoning worth keeping. If one exists, **offer** to file it via `/process-discussion`
   (compiling the relevant portion of the transcript) rather than doing it unasked —
   picking which exchanges actually matter enough to preserve is a judgment call worth a
   quick confirm, and `/process-discussion` already does the real work once pointed at
   the text.

If none of the three fired this session, say so plainly in the closing output — an empty
result here is a real, valid finding, not a skipped step.

## Then write three things

1. **Fill the session note's `## Close` section:**

```markdown
## Close — the reckoning
**What changed:** …
**Playable now:** …
**Ruled out:** …
**Re-entry point:** …
**Deadline check:** …
```

This session note — not `CLAUDE.md` — is what the next `/session-open` reads to rebuild
the map, alongside `_tasks/active.md` and recent `_meetings/`/`_decisions/` notes. Do not
write a "Current status" or next-action section into the game domain's `CLAUDE.md`; that
file deliberately carries none. If the reckoning surfaced a real new task, list it as a
candidate for `_tasks/active.md` in your closing message rather than adding the row
yourself. Link any `_decisions/` note(s) just filed from the routing step above into
**What changed**.

2. **The routing step's own output** — any `_decisions/` note(s) filed (path + one line
   each), `_questions/` candidates (owner decides), and any `_discussions/` filing offered
   — as its own short section in the closing message, separate from the reckoning.

3. **Foundation-vs-avoidance note** — *only* if a tell actually fired this session:
   polishing something that already works, revisiting a settled decision, an unmentioned
   deadline, or daily activity with nothing playable. One line, hands-dirty: name the tell
   and point at the next checklist item. If nothing fired, omit this entirely — no fishing.

## Style

- The reckoning is a checklist, not a narrative. Terminal markdown, no preamble.
- Never rate the session or the person. Position, not praise, not blame.
- If the transcript is thin, write what you can verify and mark the rest unconfirmed.

## See also

Paired with [`session-open`](session-open.md).
