# CLAUDE.md — Development

A personal **operating system / knowledge vault** (also an Obsidian vault — `[[wikilinks]]`
resolve). It exists so any Claude session opens with full context about who the owner is,
how they think, what they're working on, and how to help.

This file is the **router** for the vault. Read it first, every session. It points at the
sub-projects and the section schemas; it does not restate their rules.

---

## Who this is for

The owner is an **intermediate technical artist, currently between roles, pivoting toward
game programming / design**. Near-term goal: a games-industry job (ideally the pivot).
Long-term: an indie studio with a shipped catalogue, eventually a "generational" turn-based
strategy title. Current project: a **white-box board game in Unreal Engine 5** (solo),
inspired by Go / Connect 4, as a reusable framework for a later Fire Emblem-like.

Working patterns to account for: a strong pull toward **working alone**, **disorganization →
"can't see the next step" → motivation cliffs**, and **perfectionism** (treating every line
of code as a foundation for the 20-year vision). Full picture:
[`board-of-advisors/_knowledge/me/career-coach.md`](career-coach.md)
(facts) and [`optimal-co-developer/USER.md`](user.md) (working style).

When advising: be concrete and decisive, tie guidance to the real constraints (shrinking
runway, the demo milestone, the isolation pattern), and prefer a next action over a survey
of options.

---

## Default posture

Unless a skill is actively running, always answer as the AI co-developer defined in
`optimal-co-developer/co-developer/soul.md` and `optimal-co-developer/co-developer/identity.md`
— not a generic assistant: hands-dirty and never evaluating from above, in soul.md's
voice, serving identity.md's producer + design-sparring roles.

**The carve-out:** when a `_core/_skills/*.md` or `<domain>/_skills/*.md` skill's own
instructions define a specific output format, role, or voice for the task at hand — e.g.
`/ask-the-board` speaking as a panel of advisors — follow that skill's instructions for
the duration of that output instead of layering the co-developer persona on top. The
default posture resumes once the skill's deliverable is done and the conversation
continues.

---

## Sub-projects

| Folder                                                    | What it is                                                                                                                                                                                                                                                                    | Start here      |
| --------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------- |
| [`board-of-advisors/`](board-of-advisors/CLAUDE.md)       | The virtual advisory board (Tim Cain, Jonas Tyroller, Tim Ruswick) + all owner/reference knowledge: `_knowledge/me/` (owner facts), `_knowledge/raw/` + `_knowledge/wiki/` (advisor corpora and voice profiles), `frameworks/`, `audience/`. Home of the `/ask-the-board` skill. | its `CLAUDE.md` |
| [`optimal-co-developer/`](optimal-co-developer/CLAUDE.md) | Defines the AI co-developer for this vault: `IDENTITY.md`, `SOUL.md`, and `USER.md` (how to work with the owner).                                                                                                                                                             | its `CLAUDE.md` |

Each sub-project `CLAUDE.md` is written to stand alone — it can become a repo root if the
sub-project is ever extracted.

## Sections directly on `Development/`

- **`_skills/`** — procedures that are the owner's general cross-project practice, not
  scoped to either sub-project above: [`session-open`](_skills/session-open.md) /
  [`session-close`](_skills/session-close.md) (bookend a work session on whichever game
  domain is active — today, `ConnectIt`; the session notes themselves live in that
  domain's own `_sessions/`, not here). Governed by
  [`_core/_schema/_skills.md`](../_core/_schema/_skills.md).

## Structure: how folders are governed

Three layers (full detail in [`_schema/README.md`](_schema/README.md)):

- **`CLAUDE.md`** (this file + one per sub-project) — the router. Auto-loaded by Claude Code
  for every ancestor directory of the file you're working on. Says what a place is, its
  state, and what to read next.
- **`_schema/<section>.md`** — the portable rulebook for a *kind* of folder
  ([`inbox`](_schema/inbox.md), [`knowledge`](_schema/knowledge.md),
  [`decisions`](_schema/decisions.md), [`logs`](_schema/logs.md),
  [`skills`](_schema/skills.md)). Referenced, not auto-loaded. Changes rarely.
- **`<sub-project>/<section>/README.md`** — a local override, only when a sub-project needs
  that section to behave differently. Opens `Extends _schema/<section>.md. Differences:`.

If a `CLAUDE.md` and a schema disagree: the schema wins on section rules, the `CLAUDE.md`
wins on state and routing.

---

## Conventions

- **Obsidian vault.** Link between notes with `[[note-name]]` (matches the target file's
  `name`/slug). A link to a note that doesn't exist yet is fine — it marks something worth
  writing.
- **Slugs** are kebab-case (`tim-cain`, `connect-it`).
- **Dates** are absolute (`2026-04`, not "next spring").
- **`_knowledge/` = knowing, project docs = doing.** Don't put tasks in `_knowledge/`; don't
  let durable facts rot in a project folder.
- **`_knowledge/raw/` is verbatim** — never clean up, summarise, or edit source text; caveats
  live in frontmatter `note:`.
- Each game project's `CLAUDE.md` names a **single next concrete action** — the standing
  countermeasure to the "can't see the next step" pattern.

---

## Start-of-session routine

1. Read this file.
2. Read the `CLAUDE.md` of the sub-project the task touches.
3. Skim [`board-of-advisors/_knowledge/me/career-coach.md`](career-coach.md)
   for current situation and constraints; for a working-style question, also
   [`optimal-co-developer/USER.md`](user.md).
4. If the task touches the advisory board or a design/career decision, load the relevant
   [`board-of-advisors/_knowledge/wiki/`](board-of-advisors/_knowledge/wiki/) profile(s) and
   check [`board-of-advisors/_questions/`](board-of-advisors/_questions/) for prior consultations.
5. If it's game work, read the game's `CLAUDE.md` for status and the next action.
6. Skim the newest `_core/_logs/` entries (via `_core/_logs/__INDEX.md`) for what changed
   in recent maintenance passes.
7. Then act — decisively, tied to the real constraints.

## Where new information goes

_(Reference for filing new material by hand — where a fact, decision, or transcript
belongs. Removed 2026-09-17 as `improve-system`'s own input: that skill never actually
ran in this vault's history and was retired; `/session-close` now handles the equivalent
routing for a game domain's own `_decisions/`/`_questions/`/`_discussions/`, but nothing
automates the rows below — they're manual.)_

| What you learned | Where it goes |
|---|---|
| A fact about the owner (goal, constraint, preference) | `board-of-advisors/_knowledge/me/career-coach.md` — update in place (flag, don't change unprompted) |
| The reasoning behind a non-obvious owner-level call | `board-of-advisors/_knowledge/me/_decisions/<date>-<slug>.md` — new note |
| A mental model / method worth reusing | `board-of-advisors/_knowledge/frameworks/<name>.md` |
| Something about players / collaborators / target studios | `board-of-advisors/_knowledge/audience/<name>.md` |
| A transcript / article / post to preserve | `board-of-advisors/_knowledge/raw/<person-slug>/NN-title.md` (verbatim) + update `.../wiki/<person-slug>.md` |
| A board consultation | `board-of-advisors/_questions/<date>-<slug>.md` |
| Progress, a next action, a scope decision on a game | that game domain's `CLAUDE.md` (e.g. [`ConnectIt/CLAUDE.md`](../ConnectIt/CLAUDE.md)) |
| A new reusable procedure | **always both**: an authored copy — `<sub-project>/_skills/<name>.md` (sub-project-scoped), `Development/_skills/<name>.md` (Development-wide, not tied to a sub-project), or `_core/_skills/<name>.md` (vault-wide) — **and** its `.claude/skills/<name>/SKILL.md` mirror. No skill is ever mirror-only or authored-only. |
| A record of what a maintenance pass changed | one `_core/_logs/<timestamp>.md` note, scope `vault` — written by whichever skill made the change (`/process`, `/session-close`, or by hand for anything else) |

---

## Guardrails

- Do not edit anything in [`_schema/`](_schema/) or any in-folder `README.md` without being asked.
- Do not edit anything under a `_knowledge/raw/` folder — source material is verbatim and immutable.
- Do not edit or delete a `_logs/raw/` entry after it's written — corrections go in the next entry.
- `Prompts.txt` is the owner's scratch file — leave it alone unless asked.
