---
name: improve-system
description: >-
  Use at the end of a work session, or whenever the user says "improve the system",
  "/improve-system", "update the wikis/skills", or "tidy up the vault". Runs the
  vault-wide `/process` sweep first (every `_core/_skills/process-*` skill), then does
  everything that sweep doesn't cover: reconstructs what the session actually produced,
  routes it to the right home per the owning domain's CLAUDE.md, refreshes
  CLAUDE.md/schema accuracy, and flags anything stale, duplicated, or contradictory
  (never silently deletes). Writes one combined log.
---

# Improve System

A maintenance pass over the vault. Run the mechanical sweep, then reconstruct what just
happened and fold it into the right folders, surface rot, and log it all in one place. The
goal is that the next session opens with an operating system that reflects everything
learned in this one.

## Purpose

Keep the vault's mechanical invariants (what `/process` already checks) *and* its
higher-judgment accuracy (CLAUDE.md files, the skills, each game domain's own state) both
current as work accumulates. This skill makes the **safe, mechanical updates** itself
(parity fixes, link fixes, index tables, obvious refreshes) and **flags everything
judgment-heavy** (deletions, moves, restructuring, changes to the owner's stated facts)
for the user to approve.

## Inputs to read first

1. **The current session transcript** — the primary input for the unique-work half.
   Identify: what the user asked for, what files were created or changed, what was
   learned about the owner / an advisor / an audience / a project, and any procedure that
   got repeated.
2. Every `_core/_skills/process-*.md` file — needed to run step 1. Discover by globbing
   `_core/_skills/process-*.md` rather than hard-coding the list.
3. [`ConnectIt-Vault/CLAUDE.md`](../../CLAUDE.md) — the vault router: the domain index and
   `_core/CLAUDE.md`'s standing rules. Also read the `CLAUDE.md` of any domain the session
   touched, and — for owner/career context — [`Development/CLAUDE.md`](../../Development/CLAUDE.md)'s
   "Where new information goes" table.
4. [`_core/_templates/TSchemaSkills.md`](../_templates/TSchemaSkills.md) — the skill
   template, when editing or creating a skill; [`_core/_schema/_skills.md`](../_schema/_skills.md)
   for the two-location rule (every skill keeps an authored copy *and* a `.claude/skills/`
   mirror — no exceptions).
5. The specific files the session touched — read them before editing.

If an expected input is missing, say so; don't guess what the session contained.

## Arguments

- Optional focus hint after `/improve-system` (e.g. `wikis only`, `check for stale/dupe`,
  `just log it`). With a hint, still run the `/process` sweep, but scope the unique-work
  half to that hint.
- With no argument: run the full pass on the current session.
- Restate the resolved scope in one line at the top of the output.

## The pass

### 1. Run `/process`

Run every discovered `_core/_skills/process-*.md` skill's own **Steps** in full, in
`/process`'s own discovery order — collect each one's **Output format** sections but skip
each sub-skill's own **Log** step (this skill writes one combined log at the end covering
everything, per Step 6 below). This is the mechanical half: `_data/` wiki↔raw parity,
`_tasks/` filing, `_code/` drift, stray `_atomic/` folders, and `_skills/` mirror sync are
all handled here — nothing below repeats them.

### 2. Reconstruct the session

2–4 sentences: what was requested, what was produced, what was learned. (This is the
context the rest of the pass routes and checks against.)

### 3. Route the session's output to the right homes

For anything the session produced or changed that step 1 doesn't already cover, check it
landed where the owning domain's `CLAUDE.md` says it should (`Development/CLAUDE.md`'s
"Where new information goes" table is the reference one), and fix or flag if not:

| Session produced… | Should live in… | This skill's move |
|---|---|---|
| A fact about the owner (goal, constraint, preference) | `Development/board-of-advisors/_knowledge/me/career-coach.md` | **flag** with the exact edit proposed — don't change owner facts unprompted |
| The reasoning behind a non-obvious owner-level call made this session | `Development/board-of-advisors/_knowledge/me/_decisions/<date>-<slug>.md` | create the note (what / when / why / what would change it) |
| A mental model used more than once | `Development/board-of-advisors/_knowledge/frameworks/<name>.md` | create the note (short: what / when / how / source) |
| Info about players, collaborators, target studios, community | `Development/board-of-advisors/_knowledge/audience/<name>.md` | create or update |
| A board consultation | `Development/board-of-advisors/_questions/<date>-<slug>.md` | verify `/ask-the-board` wrote it; create it if a board was run manually |
| Progress, a next action, a scope decision on a game | that game domain's `CLAUDE.md` (e.g. `ConnectIt/CLAUDE.md`) | update the status / next-action / not-doing lines |
| A new reusable procedure | authored copy — `<domain>/_skills/<name>.md` (domain-scoped) or `_core/_skills/<name>.md` (vault-wide) — **and always** its `.claude/skills/<name>/SKILL.md` mirror; no skill is ever mirror-only or authored-only | create both, add to the relevant index |

### 4. Consistency checks (beyond what `/process` covers)

- **`CLAUDE.md` accuracy** — every domain router's state section, the domain index, and
  (for `Development/CLAUDE.md`) the "where new info goes" table still match reality.
- **`_core/_schema/_skills.md` + any advisor roster** — deployed-skills expectations
  current; advisor roster ↔ knowledge/wiki notes present.
- **Links** — Markdown links (`[text](path)`, this vault's standard — not `[[wikilinks]]`,
  see `_core/CLAUDE.md` → Link format) resolve, or are intentional stubs.

### 5. Update the indexes (safe, do directly)

When a skill/domain/schema was added or renamed, update: the relevant `CLAUDE.md`
(domain index, sections list) and `_core/_schema/__README.md`'s section catalogue.
Update the touched domain's `CLAUDE.md` state section. Update `ABOUT.md` (section table +
Changelog) if vault *structure* changed — that's `_core/CLAUDE.md`'s standing rule, not
optional.

### 6. Flag — don't fix

Anything needing judgment goes in a **Flags** table, each row: *item · issue ·
recommendation*. Includes: stale content (superseded, out-of-date), duplication (same idea
in two places — name both and propose which is canonical), contradictions (two files
disagree), orphans (a file nothing links to / references a deleted thing), and any
owner-fact edit. **Never delete or move files, restructure folders, edit `_core/_schema/`,
or edit `Development/board-of-advisors/_knowledge/me/career-coach.md` facts without
explicit approval in this session.** (`_knowledge/raw/` entries are never edited after
writing — corrections go in the next entry.)

### 7. Write the log

One `_core/_logs/<YYYY-MM-DD-HHMM>.md` note per [`_core/_schema/_logs.md`](../_schema/_logs.md) /
[`TSchemaLogs`](../_templates/TSchemaLogs.md), scope `vault` — covering the *whole* run,
step 1's sweep included. `## Created`/`## Changed` list every file touched, grouped by
which step touched it (the `/process` sweep vs. this skill's own unique work). Add its
line to `_core/_logs/__INDEX.md`.

## Output format

Terminal markdown, start at section 1 — no preamble.

1. **Scope** — one line.
2. **`/process` sweep** — one section per sub-skill it ran, in order, each headed with the
   skill's name and its own output verbatim (minus its Log step); close with `/process`'s
   own one-line-per-sub-skill summary.
3. **Session summary** — 2–4 sentences (step 2 above).
4. **Changes made** — bullets, each with the file path, for everything from steps 3 and 5.
   "None" is a valid answer.
5. **Flags** — the table from step 6 (or "None").
6. **Suggested next steps** — short, optional.
7. Confirm the log path: `_core/_logs/<timestamp>.md`.

## Style

- Conservative. Make only the safe, mechanical edits directly; everything else is a flag.
- Concrete file paths, not vague references.
- Don't invent session content you can't see in the transcript.
- Keep the log tight — it's a changelog, not a narrative.

## See also

[`_core/_skills/process.md`](process.md) — the sweep this skill runs first; read that
file's own docs for exactly which sub-skills it discovers and in what order.
