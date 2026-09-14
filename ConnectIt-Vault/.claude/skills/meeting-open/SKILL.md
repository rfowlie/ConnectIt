---
name: meeting-open
description: >-
  Opens a structured check-in conversation about the core project — triggers on
  "/meeting-open", "start a meeting", "let's check in", "status meeting", or the owner
  wanting a deliberate progress/blockers/morale conversation rather than routine coding
  work. Reads the active game domain's CLAUDE.md, its open tasks, the last
  optimal-co-developer/_meetings/ note, and career-coach.md / user.md for what to probe.
  Asks a small set of open-ended questions — progress, roadblocks, decisions, morale,
  priorities — one group at a time, each free to be elaborated or explored, in the
  Producer voice from identity.md. Doesn't save anything itself. Paired with
  /meeting-closed, which saves the resulting transcript and a summary.
---

# Meeting Open

The opening half of a deliberate check-in — not the routine [`session-open`](../../_skills/session-open.md)
/ [`session-close`](../../_skills/session-close.md) bookend around one sitting of coding
work, but a standing-back conversation about how the project **and the owner** are doing:
progress, what's stuck, what needs a call, morale, and whether the scope still makes
sense. Run it whenever the owner asks for a check-in, or when several sessions have
passed without one.

This is the co-developer's **Producer** role
([`identity.md`](../co-developer/identity.md)) — the person who runs the standup so the
owner doesn't have to run it on himself. Voice is [`soul.md`](../co-developer/soul.md):
hands-dirty, asking real questions, not administering a form.

## Inputs to read first

1. **The active game domain's `CLAUDE.md`** — today,
   [`ConnectIt/CLAUDE.md`](../../../ConnectIt/CLAUDE.md): current state, single next
   action, anything marked not-doing-right-now. Anchors the progress question in what was
   actually planned.
2. **That domain's `_tasks/active.md`** (e.g. [`ConnectIt/_tasks/active.md`](../../../ConnectIt/_tasks/active.md))
   — the open task list, if the file exists yet.
3. **The most recent note in [`optimal-co-developer/_meetings/`](../_meetings/__README.md)**
   — what was flagged, decided, or left open last time. Carry it forward instead of
   re-asking from zero. If the folder is empty, say so — this is the first meeting.
4. **[`co-developer/identity.md`](../co-developer/identity.md)** +
   **[`co-developer/soul.md`](../co-developer/soul.md)** — the posture for this session.
5. **[`me/career-coach.md`](../me/career-coach.md)** + **[`me/user.md`](../me/user.md)** —
   the owner's real constraints and patterns (isolation, perfectionism, the
   motivation-cliff loop, the couch/engine split). Use these to make the morale question
   specific, never a generic wellness check.

If any of these is missing or thin, say so plainly rather than skipping the question it
would have informed.

## Arguments

- Optional focus after `/meeting-open` (e.g. `/meeting-open roadblocks`) narrows to one
  question group. With none, run the full check-in.
- State which domain and which prior meeting note (or "first meeting") this is building
  from, in one line, before the first question.

## The questions

Six groups. Ask **one group at a time**, in order, and wait for a real answer before
moving on — this is a conversation, not a survey dumped in one message. After each answer,
offer one genuine follow-up if there's an obvious thread to pull (a blocker that sounds
bigger than stated, a task marked done that contradicts the last meeting note); otherwise
move on. Skip a group only if the owner explicitly says to.

1. **Progress** — What actually got done since [the last meeting / the last session note /
   the domain's current state], concretely? Anchor against input 1–3 rather than taking
   "a lot" at face value — name the specific systems or tasks if they're known, and ask
   which of them are true.
2. **Roadblocks** — What's stuck right now, and what's already been tried on it? Distinct
   from a task that's merely not-yet-started.
3. **Decisions** — Is there anything unresolved that needs an actual call before work can
   continue — scope, architecture, or otherwise? (A "yes" here is a candidate for a
   `_decisions/` note at close.)
4. **Morale & energy** — How's motivation and focus right now — grounded in a specific
   pattern from `career-coach.md` / `user.md` (isolation, perfectionism, the
   motivation-cliff loop) rather than a generic "how are you feeling." Ask about the
   pattern that's actually live, not all of them at once.
5. **Priorities & scope** — Does the current scope still hold? Anything worth cutting,
   adding, or re-sequencing against the milestone / runway.
6. **Anything else** — open floor. Explicitly invite whatever doesn't fit the first five.

## Closing the conversation

After group 6, say plainly that the check-in is complete and that running
[`/meeting-closed`](meeting-closed.md) will save this conversation as a dated
`_meetings/` note — transcript plus summary. Don't write anything to disk yourself; this
skill only runs the conversation.

## Style

- Terminal markdown. No preamble beyond the one-line continuity statement required above.
- Real questions, not form fields — restate what's already known (input 1–3) so the owner
  isn't asked to repeat it.
- Never render a verdict on the owner mid-conversation (`soul.md`) — surface a pattern,
  don't diagnose it.
- If an input is missing, name the gap and ask the question anyway rather than skipping it
  silently.

## See also

Paired with [`meeting-closed`](meeting-closed.md).
