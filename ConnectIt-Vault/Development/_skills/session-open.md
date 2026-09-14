---
name: session-open
description: >-
  Run at the very start of a work session on a game project. Hands back the map:
  where the project is, what's playable, what was recently ruled out, the single
  next action, and what not to touch. Reads the last session note and the game's
  CLAUDE.md so the owner re-enters with a map, not a blank engine. Use when the
  user says "session open", "/session-open", "where were we", or starts working
  without stating a plan.
---

# Session Open

The opening bookend. The owner externalizes decisions but not state
(`Development/optimal-co-developer/me/user.md`), and the broken between-session loop is
the core vulnerability. This skill closes that loop at the start: reconstruct where we
are from the written record and hand it back in five short blocks.

This is a **map hand-off, not a status report on the person.** No "you didn't finish X."
State the build's position; don't render a verdict.

## Inputs to read first

1. **The active game domain's `CLAUDE.md`** — today, [`ConnectIt/CLAUDE.md`](../../ConnectIt/CLAUDE.md)
   (the vault's only game domain; ask which if a second one ever exists): its current
   state, single next action, anything marked not-doing-right-now, and the demo
   checklist if one exists.
2. **The last session note** — the newest file in that domain's `_sessions/` (e.g.
   `ConnectIt/_sessions/`). Read its `## Close` reckoning. If it has no reckoning filled
   in, the last session didn't close — say so and treat its next action as unverified.
3. **The previous 2–3 session notes** — skim only the `Ruled out` and `Re-entry point`
   lines, so the map can list what's currently off the table.
4. [`Development/optimal-co-developer/co-developer/identity.md`](../optimal-co-developer/co-developer/identity.md)
   + [`soul.md`](../optimal-co-developer/co-developer/soul.md) — the producer posture, if
   not already in context.

If the domain has no `_sessions/` folder yet, this is the first session: say so, build
the map from its `CLAUDE.md` alone, and create the folder at the end.

## Arguments

- Optional game name after `/session-open`. With none, resolve per input 1.
- Restate which game and which prior session note you're building from, in one line.

## Output — the map

Terminal markdown, no preamble. Five blocks:

1. **Where we are** — 1–2 sentences on the build's current position, from the domain's
   `CLAUDE.md` and the last reckoning.
2. **Playable** — what a stranger could do in the current build. If the last reckoning
   said nothing became playable, state that plainly as a fact about the build.
3. **Off the table** — recently ruled-out items (input 3), one line each: what, and why
   it was killed. "Nothing recent" is valid.
4. **The one next thing** — the single next action, verbatim from the domain's `CLAUDE.md`
   if it's concrete. If it's blank or vague, say so — the first task is to make it
   concrete (planning, not motivation); offer 2–3 candidate next actions.
5. **Not touching** — anything marked not-doing-right-now, trimmed to what's live this
   session.

Then stop. Don't start the work or expand the plan unless asked.

## Then create the session note

Create `<domain>/_sessions/<YYYY-MM-DD-HHMM>.md` per
[`_core/_schema/_sessions.md`](../../_core/_schema/_sessions.md) /
[`TSchemaSessions`](../../_core/_templates/TSchemaSessions.md), the map as the `## Open`
half:

```markdown
# session — <timestamp>

## Open — the map
**Where we are:** …
**Playable:** …
**Off the table:** …
**Next thing:** …
**Not touching:** …

## Close — the reckoning
_(written by /session-close)_
```

`<timestamp>` = `YYYY-MM-DD-HHMM` local, 24h. On a same-minute collision append `-2`.

## Style

- Five blocks, nothing more. This runs every session — keep it to what re-orients in
  30 seconds.
- No preamble, no encouragement, no assessment of the person.
- If an input is missing, name the gap — don't paper over it.

## See also

Paired with [`session-close`](session-close.md).
