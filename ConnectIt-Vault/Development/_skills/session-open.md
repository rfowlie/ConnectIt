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
   (the vault's only game domain; ask which if a second one ever exists) — for structure
   and routing only. **It deliberately carries no "current state" or single-next-action
   section** (removed 2026-09-14 as a stale artifact that always drifted from reality) —
   don't expect one and don't reintroduce one; current state is reconstructed live from
   inputs 2–3 below instead. If it names a demo checklist, note it.
2. **`_tasks/active.md`** in that domain (e.g. [`ConnectIt/_tasks/active.md`](../../ConnectIt/_tasks/active.md))
   — the actual open-work list. This, not `CLAUDE.md`, is the source for "what's in play"
   and the candidate next actions.
3. **The last session note** — the newest file in that domain's `_sessions/` (e.g.
   `ConnectIt/_sessions/`). Read its `## Close` reckoning. If it has no reckoning filled
   in, the last session didn't close — say so and treat its next action as unverified.
4. **The previous 2–3 session notes** — skim only the `Ruled out` and `Re-entry point`
   lines, so the map can list what's currently off the table.
5. **The most recent note in `_meetings/`** (e.g.
   [`Development/optimal-co-developer/_meetings/`](../optimal-co-developer/_meetings/__README.md))
   and any `_decisions/` notes dated since the last session note — a check-in's `## Next`
   and a decision's explicit deferrals ("captured as next step after X ships") are real
   sources for "not touching" and "the one next thing," on top of what `_tasks/active.md`
   itself says.
6. [`Development/optimal-co-developer/co-developer/identity.md`](../optimal-co-developer/co-developer/identity.md)
   + [`soul.md`](../optimal-co-developer/co-developer/soul.md) — the producer posture, if
   not already in context.

If the domain has no `_sessions/` folder yet, this is the first session: say so, build
the map from `_tasks/active.md` plus any `_meetings/`/`_decisions/` notes, and create the
folder at the end.

## Arguments

- Optional game name after `/session-open`. With none, resolve per input 1.
- Restate which game and which prior session note you're building from, in one line.

## Output — the map

Terminal markdown, no preamble. Five blocks, **each a bullet list, not prose** — one
bullet per fact, no scene-setting or connective sentences around them. A bullet states a
fact or names an item; it doesn't narrate why the block exists.

1. **Where we are** — 2–4 bullets on the build's current position, from `_tasks/active.md`,
   the most recent `_meetings/` note, and the last session reckoning.
2. **Playable** — bullets on what a stranger could do in the current build. If the last
   reckoning said nothing became playable, one bullet states that plainly as a fact about
   the build.
3. **Off the table** — one bullet per recently ruled-out item (input 4) plus any explicit
   deferrals from recent `_decisions/` notes: what, and why it was killed or deferred, in
   one line each. "Nothing recent" is a single bullet, not a block skipped.
4. **The one next thing** — one bullet naming the single next action, derived from
   `_tasks/active.md` and the last `_meetings/` note's `## Next` (`CLAUDE.md` never states
   one). If neither is concrete, one bullet says so, then 2–3 bullets each naming a
   candidate next action from the open task rows.
5. **Not touching** — one bullet per item `_tasks/active.md` or a recent
   `_decisions/`/`_meetings/` note explicitly sequences for later, trimmed to what's live
   this session.

Then stop. Don't start the work or expand the plan unless asked.

## Then create the session note

Create `<domain>/_sessions/<YYYY-MM-DD-HHMM>.md` per
[`_core/_schema/_sessions.md`](../../_core/_schema/_sessions.md) /
[`TSchemaSessions`](../../_core/_templates/TSchemaSessions.md), the map as the `## Open`
half:

```markdown
# session — <timestamp>

## Open — the map

**Where we are:**
- …

**Playable:**
- …

**Off the table:**
- …

**Next thing:**
- …

**Not touching:**
- …

## Close — the reckoning
_(written by /session-close)_
```

`<timestamp>` = `YYYY-MM-DD-HHMM` local, 24h. On a same-minute collision append `-2`.

## Style

- Five blocks, nothing more. This runs every session — keep it to what re-orients in
  30 seconds.
- **Bullets, not paragraphs, inside every block** (owner preference, set 2026-09-16) —
  direct statements of fact, one per bullet. No connective prose stitching bullets
  together, no restating the block's own label in sentence form.
- No preamble, no encouragement, no assessment of the person.
- If an input is missing, name the gap — don't paper over it.

## See also

Paired with [`session-close`](session-close.md).
