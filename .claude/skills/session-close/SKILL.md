---
name: session-close
description: >-
  Run at the end of a work session on a game project — the two-minute reckoning.
  Captures what changed, what's playable now, what was ruled out, and the single
  next action, then updates the game's CLAUDE.md and the session log so state
  stops living only in the owner's head. Use when the user says "session close",
  "/session-close", "wrap up", "done for today", or is about to stop.
---

# Session Close

The closing bookend. Force the reckoning so state is written down, not carried
(`optimal-co-developer/me/user.md`: the owner externalizes decisions, not state — and that
gap is the core vulnerability).

This is a **reckoning, not a performance review.** Report the build's position. "Nothing
became playable" is a fact about the build, never a verdict on the owner. Every flag comes
with the fix in the same motion (`optimal-co-developer/co-developer/soul.md`).

Paths are relative to the vault root (`GameDeveloperVault/`).

## Inputs to read first

1. **This session's transcript** — what was actually done, decided, tried, abandoned.
2. **The open session log** — newest file in `game-projects/<game>/sessions/`, the one whose
   `## Close` section is still a placeholder. If there's no open log (the session didn't run
   `/session-open`), create one now and fill both halves.
3. **The game's `CLAUDE.md`** — `Current status`, `Single next action`, `Not doing right
   now`, the demo checklist. You'll propose new wording for the first two.

## Arguments

- Optional game name after `/session-close`. Restate which game and which session log you're
  closing, in one line.

## The reckoning

Walk these with the owner, confirm or correct each, then write:

1. **What changed** — 2–4 concrete bullets: files, systems, decisions. Not effort, not hours.
2. **Playable now** — what a stranger could do that they couldn't at open. If unchanged, say
   so plainly. If three closes in a row say "unchanged", name it once — with the next demo
   checklist item as the proposed fix, not as a complaint. (Tell from `user.md`: active
   daily, nothing becomes playable.)
3. **Ruled out this session** — anything tried and killed, and why. This is the record that
   stops a discarded idea being re-suggested next session.
4. **Re-entry point** — the single next action, stated concretely. If a couch problem is
   open (a scout briefing not yet acted on, a decision not yet made), that is the re-entry
   point — name it as such.
5. **Deadline check** — is the demo date / runway still in view, and has it gone unmentioned
   for a while? If so, surface it once, plainly. No lecture.

## Then write three things

1. **Fill the session log's `## Close` section:**

```markdown
## Close — the reckoning
**What changed:** …
**Playable now:** …
**Ruled out:** …
**Re-entry point:** …
**Deadline check:** …
```

2. **Update the game's `CLAUDE.md`** — rewrite `Current status` and `Single next action`
   from the reckoning. Show the two new lines, then write them. (A safe, do-directly update
   per the vault's "where new information goes" table — but the wording is the owner's
   reality, so confirm it before saving.)

3. **Foundation-vs-avoidance note** — *only* if a tell actually fired this session:
   polishing something that already works, revisiting a settled decision, an unmentioned
   deadline, or daily activity with nothing playable. One line, hands-dirty: name the tell
   and point at the next checklist item. If nothing fired, omit this entirely — no fishing.

## Style

- The reckoning is a checklist, not a narrative. Terminal markdown, no preamble.
- Never rate the session or the person. Position, not praise, not blame.
- If the transcript is thin, write what you can verify and mark the rest unconfirmed.

## Sync note

Vault-wide skill: lives only at `.claude/skills/session-close/SKILL.md`, no authored mirror.
Paired with `/session-open`.
