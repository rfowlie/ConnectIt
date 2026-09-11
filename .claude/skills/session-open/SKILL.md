---
name: session-open
description: >-
  Run at the very start of a work session on a game project. Hands back the map:
  where the project is, what's playable, what was recently ruled out, the single
  next action, and what not to touch. Reads the last session log and the game's
  CLAUDE.md so the owner re-enters with a map, not a blank engine. Use when the
  user says "session open", "/session-open", "where were we", or starts working
  without stating a plan.
---

# Session Open

The opening bookend. The owner externalizes decisions but not state
(`optimal-co-developer/me/user.md`), and the broken between-session loop is the core
vulnerability. This skill closes that loop at the start: reconstruct where we are from the
written record and hand it back in five short blocks.

This is a **map hand-off, not a status report on the person.** No "you didn't finish X."
State the build's position; don't render a verdict.

Paths are relative to the vault root (`GameDeveloperVault/`).

## Inputs to read first

1. **The active game's `CLAUDE.md`** — `game-projects/<game>/CLAUDE.md`: `Current status`,
   `Single next action`, `Not doing right now`, and the demo checklist. If the user didn't
   name a game and only one game folder exists, use it; otherwise ask which.
2. **The last session log** — the newest file in `game-projects/<game>/sessions/`. Read its
   `## Close` reckoning. If it has no reckoning, the last session didn't close — say so and
   treat its next action as unverified.
3. **The previous 2–3 session logs** — skim only the `Ruled out` and `Re-entry point` lines,
   so the map can list what's currently off the table.
4. **`optimal-co-developer/co-developer/identity.md` + `soul.md`** — the producer posture, if
   not already in context.

If the game has no `sessions/` folder yet, this is the first session: say so, build the map
from `CLAUDE.md` alone, and create the folder at the end.

## Arguments

- Optional game name after `/session-open`. With none, resolve per input 1.
- Restate which game and which prior session log you're building from, in one line.

## Output — the map

Terminal markdown, no preamble. Five blocks:

1. **Where we are** — 1–2 sentences on the build's current position, from `Current status`
   and the last reckoning.
2. **Playable** — what a stranger could do in the current build. If the last reckoning said
   nothing became playable, state that plainly as a fact about the build.
3. **Off the table** — recently ruled-out items (input 3), one line each: what, and why it
   was killed. "Nothing recent" is valid.
4. **The one next thing** — the single next action, verbatim from `CLAUDE.md` if it's
   concrete. If it's blank or vague, say so — the first task is to make it concrete
   (planning, not motivation); offer 2–3 candidate next actions from the demo checklist.
5. **Not touching** — the `Not doing right now` list, trimmed to what's live this session.

Then stop. Don't start the work or expand the plan unless asked.

## Then create the session log

Create `game-projects/<game>/sessions/<YYYY-MM-DD-HHMM>.md` with the map as the top half:

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

## Sync note

Vault-wide skill: lives only at `.claude/skills/session-open/SKILL.md`, no authored mirror.
Paired with `/session-close`.
