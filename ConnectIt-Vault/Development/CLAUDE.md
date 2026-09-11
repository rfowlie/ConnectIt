# CLAUDE.md — GameDeveloperVault

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
[`board-of-advisors/knowledge/me/career-coach.md`](career-coach.md)
(facts) and [`optimal-co-developer/USER.md`](user.md) (working style).

When advising: be concrete and decisive, tie guidance to the real constraints (shrinking
runway, the demo milestone, the isolation pattern), and prefer a next action over a survey
of options.

---

## Sub-projects

| Folder | What it is | Start here |
|---|---|---|
| [`board-of-advisors/`](board-of-advisors/CLAUDE.md) | The virtual advisory board (Tim Cain, Jonas Tyroller, Tim Ruswick) + all owner/reference knowledge: `knowledge/me/` (owner facts), `knowledge/raw/` + `knowledge/wiki/` (advisor corpora and voice profiles), `frameworks/`, `audience/`. Home of the `/ask-the-board` skill. | its `CLAUDE.md` |
| [`game-projects/`](game-projects/README.md) | Group folder. Each child is a game with its own `CLAUDE.md`. Active: [`connect-it/`](game-projects/connect-it/CLAUDE.md) — the Connect-4/Go white-box framework. | the child's `CLAUDE.md` |
| [`optimal-co-developer/`](optimal-co-developer/CLAUDE.md) | Defines the AI co-developer for this vault: `IDENTITY.md`, `SOUL.md`, and `USER.md` (how to work with the owner). | its `CLAUDE.md` |

Each sub-project `CLAUDE.md` is written to stand alone — it can become a repo root if the
sub-project is ever extracted.

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
- **`knowledge/` = knowing, project docs = doing.** Don't put tasks in `knowledge/`; don't
  let durable facts rot in a project folder.
- **`knowledge/raw/` is verbatim** — never clean up, summarise, or edit source text; caveats
  live in frontmatter `note:`.
- Each game project's `CLAUDE.md` names a **single next concrete action** — the standing
  countermeasure to the "can't see the next step" pattern.

---

## Start-of-session routine

1. Read this file.
2. Read the `CLAUDE.md` of the sub-project the task touches.
3. Skim [`board-of-advisors/knowledge/me/career-coach.md`](career-coach.md)
   for current situation and constraints; for a working-style question, also
   [`optimal-co-developer/USER.md`](user.md).
4. If the task touches the advisory board or a design/career decision, load the relevant
   [`board-of-advisors/knowledge/wiki/`](board-of-advisors/knowledge/wiki/) profile(s) and
   check [`board-of-advisors/questions/`](board-of-advisors/questions/) for prior consultations.
5. If it's game work, read the game's `CLAUDE.md` for status and the next action.
6. Skim the latest entry in `board-of-advisors/logs/wiki/improve-system/` (via its
   `README.md` index) for what changed in recent maintenance passes — once the first
   `/improve-system` run has created it.
7. Then act — decisively, tied to the real constraints.

## Where new information goes

_(Used by the `improve-system` skill to route a session's output.)_

| What you learned | Where it goes |
|---|---|
| A fact about the owner (goal, constraint, preference) | `board-of-advisors/knowledge/me/career-coach.md` — update in place (flag, don't change unprompted) |
| The reasoning behind a non-obvious owner-level call | `board-of-advisors/knowledge/me/decisions/<date>-<slug>.md` — new note |
| A mental model / method worth reusing | `board-of-advisors/knowledge/frameworks/<name>.md` |
| Something about players / collaborators / target studios | `board-of-advisors/knowledge/audience/<name>.md` |
| A transcript / article / post to preserve | `board-of-advisors/knowledge/raw/<person-slug>/NN-title.md` (verbatim) + update `.../wiki/<person-slug>.md` |
| A board consultation | `board-of-advisors/questions/<date>-<slug>.md` |
| Progress, a next action, a scope decision on a game | that game's `CLAUDE.md` (e.g. `game-projects/connect-it/CLAUDE.md`) |
| A new reusable procedure | sub-project skill → `<sub-project>/skills/<name>.md` + `.claude/skills/<name>/SKILL.md`; vault-wide → `.claude/skills/<name>/SKILL.md` only |
| A record of what a maintenance pass changed | `board-of-advisors/logs/raw/improve-system/<timestamp>.md` + summary in `.../wiki/…` (written by `/improve-system`, not by hand) |

---

## Guardrails

- Do not edit anything in [`_schema/`](_schema/) or any in-folder `README.md` without being asked.
- Do not edit anything under a `knowledge/raw/` folder — source material is verbatim and immutable.
- Do not edit or delete a `logs/raw/` entry after it's written — corrections go in the next entry.
- `Prompts.txt` is the owner's scratch file — leave it alone unless asked.
