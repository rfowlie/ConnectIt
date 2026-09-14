---
name: improve-system
description: >-
  Use at the end of a work session, or whenever the user says "improve the system",
  "/improve-system", "update the wikis/skills", or "tidy up the vault". Reviews what
  happened this session and maintains this operating system: refreshes the wikis and
  skills the session touched, creates new knowledge folders / wikis / framework notes
  where the session produced material that needs a home, flags anything that feels
  stale, duplicated, or contradictory (never silently deletes), and writes a dated
  full entry + summary to board-of-advisors/logs/raw/improve-system/ and
  board-of-advisors/logs/wiki/improve-system/.
---

# Improve System

A maintenance pass over the vault. Reconstruct what just happened, fold it into the
right folders, surface rot, and log it. The goal is that the next session opens with
an operating system that reflects everything learned in this one.

Paths below are relative to the vault root (`GameDeveloperVault/`).

## Purpose

Keep the `CLAUDE.md` files, `_schema/`, `board-of-advisors/knowledge/`, the skills, and the
game-project `CLAUDE.md`s accurate and non-redundant as work accumulates. This skill makes
the **safe, mechanical updates** itself (parity fixes, link fixes, index tables, obvious
wiki refreshes) and **flags everything judgment-heavy** (deletions, moves, restructuring,
changes to the owner's stated facts) for the user to approve.

## Inputs to read first

1. **The current session transcript** — the primary input. Identify: what the user
   asked for, what files were created or changed, what was learned about the owner /
   an advisor / an audience / a project, and any procedure that got repeated.
2. [`CLAUDE.md`](../../../CLAUDE.md) — the vault router: the sub-project index, conventions,
   and the "where new information goes" table. Everything this skill does must conform to
   it. Also read the `CLAUDE.md` of any sub-project the session touched.
3. [`_schema/skill-template.md`](../../../_schema/skill-template.md) — the skill template, when
   editing or creating a skill; [`_schema/skills.md`](../../../_schema/skills.md) for the
   two-location rules.
4. The specific files the session touched — read them before editing.
5. `board-of-advisors/logs/wiki/improve-system/README.md` (the run index) and the most
   recent entry or two under `board-of-advisors/logs/wiki/improve-system/` — for continuity
   with prior passes. Create the `logs/` structure if absent (see "The log" below).

If an expected input is missing, say so; don't guess what the session contained.

## Arguments

- Optional focus hint after `/improve-system` (e.g. `wikis only`, `check for stale/dupe`,
  `just log it`). With a hint, scope the pass to that; still always write the log.
- With no argument: run the full pass on the current session.
- Restate the resolved scope in one line at the top of the output.

## The pass

### 1. Reconstruct the session
2–4 sentences: what was requested, what was produced, what was learned.

### 2. Route the session's output to the right homes
For each thing the session produced or changed, check it landed where `CLAUDE.md`'s
"where new information goes" table says it should, and fix or flag if not:

| Session produced… | Should live in… | This skill's move |
|---|---|---|
| New raw transcript/article/post | `board-of-advisors/knowledge/raw/<person-slug>/NN-*.md` (verbatim, frontmatter) | verify it exists & is verbatim; **ensure the matching `board-of-advisors/knowledge/wiki/<slug>.md` is created/updated** |
| A new source person | new `board-of-advisors/knowledge/raw/<slug>/` **and** new `.../wiki/<slug>.md`; consider adding to `board-of-advisors/board-of-advisors.md` | create the wiki if missing; flag the board question |
| A fact about the owner (goal, constraint, preference) | `board-of-advisors/knowledge/me/career-coach.md` | **flag** with the exact edit proposed — don't change owner facts unprompted |
| The reasoning behind a non-obvious owner-level call made this session | `board-of-advisors/knowledge/me/decisions/<date>-<slug>.md` | create the note (what / when / why / what would change it) |
| A mental model used more than once | `board-of-advisors/knowledge/frameworks/<name>.md` | create the note (short: what / when / how / source), link `[[wiki-slug]]` |
| Info about players, collaborators, target studios, community | `board-of-advisors/knowledge/audience/<name>.md` | create or update |
| A board consultation | `board-of-advisors/questions/<date>-<slug>.md` | verify `/ask-the-board` wrote it; create it if a board was run manually |
| Progress, a next action, a scope decision on a game | that game's `CLAUDE.md` (e.g. `game-projects/connect-it/CLAUDE.md`) | update the status / next-action / not-doing lines; create the game folder + `CLAUDE.md` if the work is now real |
| A new reusable procedure | sub-project skill → `<sub-project>/skills/<name>.md` **and** `.claude/skills/<name>/SKILL.md`; vault-wide → `.claude/skills/<name>/SKILL.md` only | create the copies that apply, add to the indexes (step 4) |

### 3. Consistency checks
- **wiki ↔ raw parity** — every file in `board-of-advisors/knowledge/raw/<slug>/` is reflected in `board-of-advisors/knowledge/wiki/<slug>.md`; no wiki claim lacks raw support; guest-speaker / hosted-conversation caveats are carried into the wiki.
- **skill sync** — every sub-project skill's `<sub-project>/skills/<name>.md` matches its `.claude/skills/<name>/SKILL.md`. Report diffs; align them (prefer the `.claude/` copy if the session edited that one). Vault-wide skills have no authored mirror — don't flag their absence.
- **log parity** — every `board-of-advisors/logs/raw/<source>/X.md` has a matching `.../wiki/<source>/X.md`, and `.../wiki/<source>/README.md` has a line for it.
- **`CLAUDE.md` accuracy** — the vault router's sub-project index, "where new info goes" table, guardrails, and start-of-session routine still match reality; each sub-project `CLAUDE.md`'s state section is current.
- **`_schema/skills.md` + `board-of-advisors/board-of-advisors.md`** — deployed-skills table current; advisor roster ↔ wikis present.
- **Links** — `[[wikilinks]]` and relative links resolve, or are intentional stubs. No link points into the deleted `README/` folder.

### 4. Update the indexes (safe, do directly)
When a skill/wiki/framework/game was added or renamed, update: `CLAUDE.md` (sub-project
index + "where new info goes" if a new category) and `_schema/skills.md` (deployed-skills
table). Update the touched sub-project's `CLAUDE.md` state section.

### 5. Flag — don't fix
Anything needing judgment goes in a **Flags** table, each row: *item · issue · recommendation*.
Includes: stale content (superseded, out-of-date), duplication (same idea in two places —
name both and propose which is canonical), contradictions (two files disagree),
orphans (a file nothing links to / references a deleted thing), and any owner-fact edit.
**Never delete or move files, restructure folders, edit `_schema/`, or edit
`board-of-advisors/knowledge/me/career-coach.md` facts without explicit approval in this
session.** (`logs/raw/` entries are also never edited after writing — corrections go in the
next entry.)

### 6. Write the log
Write the two files + update the index described in "The log" below.

## Output format

Terminal markdown, start at section 1 — no preamble.

1. **Scope** — one line.
2. **Session summary** — 2–4 sentences.
3. **Changes made** — bullets, each with the file path. "None" is a valid answer.
4. **Flags** — the table (or "None").
5. **Suggested next steps** — short, optional.
6. Confirm the log was written: `board-of-advisors/logs/raw/improve-system/<timestamp>.md`,
   `board-of-advisors/logs/wiki/improve-system/<timestamp>.md`, and the index line in
   `board-of-advisors/logs/wiki/improve-system/README.md`.

## The log — `board-of-advisors/logs/`

Every run writes **two files with the same name**, plus one index update:

| File | Content |
|---|---|
| `board-of-advisors/logs/raw/improve-system/<timestamp>.md` | The **full entry** — session topic, summary, every changed path, every flag with its reasoning, all follow-ups. **Immutable once written.** |
| `board-of-advisors/logs/wiki/improve-system/<timestamp>.md` | The **summary** — 4–6 lines for quick context: what happened, headline changes, open flags, link to the full entry. |
| `board-of-advisors/logs/wiki/improve-system/README.md` | The **rolling index** — prepend one line for this run (newest first). |

`<timestamp>` = `YYYY-MM-DD-HHMM` (local, 24h; e.g. `2026-09-14-0930`). Date-only is
acceptable if the time is unknown. On a same-minute collision, append `-2`, `-3`.

Create `board-of-advisors/logs/raw/improve-system/` and `.../wiki/improve-system/` if
absent. Never edit or delete a `logs/raw/` entry after writing it.

**Full entry** — `board-of-advisors/logs/raw/improve-system/<timestamp>.md`:

```markdown
# improve-system — <timestamp>

**Session topic:** <short>

**Summary:** 2–4 sentences on what the session did.

**Changes:**
- `path` — what changed and why

**Flags raised:**
- <item> — <issue> — <recommendation>   (or "None")

**Follow-ups:**
- <anything deferred>   (or "None")
```

**Summary** — `board-of-advisors/logs/wiki/improve-system/<timestamp>.md`:

```markdown
# improve-system — <timestamp>

**What happened:** 1–2 sentences.

**Headline changes:**
- <the 2–4 that matter>

**Open flags:** <the ones still needing a decision, or "None">

**Full entry:** [`../../raw/improve-system/<timestamp>.md`](../../raw/improve-system/<timestamp>.md)
```

**Index line** — prepend under the header of
`board-of-advisors/logs/wiki/improve-system/README.md`:

```markdown
- **<date>** — <one-line description> — [summary](<timestamp>.md) · [full](../../raw/improve-system/<timestamp>.md)
```

## Style

- Conservative. Make only the safe, mechanical edits directly; everything else is a flag.
- Concrete file paths, not vague references.
- Don't invent session content you can't see in the transcript.
- Keep the log tight — it's a changelog, not a narrative.

## Sync note

This is a **vault-wide** skill: it lives only at `.claude/skills/improve-system/SKILL.md`,
with no authored mirror. Edit it here.
