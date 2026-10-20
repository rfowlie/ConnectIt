# CLAUDE.md — board-of-advisors

The **virtual advisory board** and the vault's store of owner/reference knowledge.

Part of `GameDeveloperVault/`. Written to stand alone: if this folder is extracted into its
own repo, this file becomes the root `CLAUDE.md` (copy `_schema/` in alongside it — see
[`../_schema/README.md`](../_schema/README.md)).

---

## What's here

| Path | What it is |
|---|---|
| [`board-of-advisors.md`](board-of-advisors.md) | The roster — Tim Cain, Jonas Tyroller, Tim Ruswick — why each fits, 5 content picks each, and how to run a board. |
| [`board-of-advisors-links.md`](board-of-advisors-links.md) | Verified channel / playlist / video links for the picks. |
| [`_knowledge/me/career-coach.md`](career-coach.md) | **Canonical owner facts** — status, goals, blockers, constraints, the 30/90-day action framework. |
| `_knowledge/me/_decisions/` | One dated note per non-obvious owner-level call → [`../_schema/decisions.md`](../_schema/decisions.md). |
| `_knowledge/raw/<slug>/` | Advisor source material, verbatim. Local rules: [`_knowledge/raw/__README.md`](_knowledge/raw/__README.md). |
| `_knowledge/wiki/<slug>.md` | One synthesis (voice profile) per advisor — what `/ask-the-board` reads. Local rules: [`_knowledge/wiki/__README.md`](_knowledge/wiki/__README.md). |
| `_knowledge/frameworks/` | Named mental models to reason with. Created on first use. |
| `_knowledge/audience/` | Players, collaborators, target studios. Created on first use. |
| [`_questions/`](_questions/) | One file per board consultation. Local rules: [`_questions/__README.md`](_questions/__README.md). |
| `_skills/ask-the-board.md` | Authored copy of the `/ask-the-board` skill (live copy at `.claude/skills/ask-the-board/SKILL.md`). |
| `_logs/` | Maintenance history for this sub-project → [`../_schema/logs.md`](../_schema/logs.md). Created on first use — still empty; `/improve-system`, the skill that would have written here, was retired 2026-09-17 (never actually ran). |

Sections follow the shared schemas in [`../_schema/`](../_schema/). Local overrides:
`_knowledge/raw/`, `_knowledge/wiki/`, and `_questions/` each have a `__README.md` that extends
its schema.

## State

- Corpora ingested: **tim-cain**, **jonas-tyroller**, **tim-ruswick** (all three board members).
- `_questions/`, `_knowledge/frameworks/`, `_knowledge/audience/`, `_logs/` are empty — created on first use.
- Bench advisors (not yet ingested): Chris Zukowski, Mark Brown. See `board-of-advisors.md`.

## Working with owner facts

`_knowledge/me/career-coach.md` is canonical for **what is true** about the owner.
[`../optimal-co-developer/USER.md`](user.md) is canonical for **how
to work with them**. On overlap, career-coach.md wins on facts, USER.md wins on approach.
Never change owner facts unprompted — flag the proposed edit instead.
