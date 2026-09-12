# ABOUT.md — ConnectIt Vault

The **reasoning** behind this vault's structure — the *why*, the decision history, and
what's still unsettled. The mechanical *how* lives in
[`_core/_schema/__README.md`](_core/_schema/__README.md); this file is what you read to
understand the shape rather than operate it.

## What this vault is

The knowledge base for AI co-development of **ConnectIt**, a turn-based tile-based
strategy game in Unreal Engine 5 (C++). It is the high-altitude, evolving layer over the
code: short "what is this / why does it exist / what changed and why it matters" notes an
AI co-developer loads quickly. It is **not** the exhaustive API reference — the code and
its own comments are authoritative; a vault note that disagrees with the code is wrong.

The vault runs on **`_core/`** — a portable co-development operating system (spec,
templates, vault-wide section instances, skills). `_core/` is built to be lifted into any
vault; see [`_core/CLAUDE.md`](_core/CLAUDE.md).

## The model

- **`_core/`** is the one non-domain folder — everything structural.
- Every other top-level folder is a **domain**: one area of work, its own stand-alone
  `CLAUDE.md` router, opting into the sections it uses. The 7 `Unreal*` plugins and
  `ConnectIt` (the game module) are **flat sibling domains** — each is independently
  reusable / extractable, so none nests under another.
- A **section** is a bare-named subfolder (`code/`, `logs/`, `decisions/`) governed by
  `_core/_schema/_<section>.md`. Sections mount **either** vault-wide (`_core/_<section>/`)
  or per-domain (`<domain>/<section>/`); a section inside a domain scopes to that domain.
- The old `_section.md` marker system was retired 2026-09-10 — a domain is now simply "a
  top-level folder with a `CLAUDE.md`".

## Section table

| Section | Purpose | Instances in this vault |
|---|---|---|
| `_core` | Spec, templates, vault-wide section instances | `_core/` (fixed) |
| `code` | Per-type pages + `systems/` flows + `recipes/` tasks + an index/map; git-anchored provenance, no `raw/`; `## Changes` history per page | every domain (8) |
| `logs` | Append-only machine-written maintenance history, one dated note per run | `_core/_logs/` (vault) · `ConnectIt/_logs/` |
| `decisions` | One dated note per non-obvious settled call | `ConnectIt/_decisions/` · `optimal-co-developer/_decisions/` |
| `skills` | Reusable procedures, authored here, mirrored to `.claude/skills/` | `_core/_skills/` (vault) · `board-of-advisors/_skills/` · `optimal-co-developer/_skills/` |
| `atomic` | Single-idea Zettelkasten pool — **single instance, `_core/_atomic/` only** | `_core/_atomic/` |
| `clippings` | Unprocessed capture dump; empty is the goal | `_core/_clippings/` |
| `tasks` | Status tables (`active`/`suspended`/`complete`) | `_core/_tasks/` (vault) · `optimal-co-developer/_tasks/` |
| `questions` | One note per open, unresolved question | `_core/_questions/` (vault) · `optimal-co-developer/_questions/` |
| `meetings` | One dated note per meeting or session | `optimal-co-developer/_meetings/` |
| `daily` | One free-form note per day | `optimal-co-developer/_daily/` |
| `ask-vault` `attachments` | Vault-level instances | `_core/_*/` |
| `data` `user` `people` `maps` | Available, not yet used | — |

Full rulebook per section: `_core/_schema/_<section>.md`.

## Conventions

- Context files are always `CLAUDE.md` at every level (auto-loading; extract-to-repo-root
  with no rename). The root `CLAUDE.md` opens with `@_core/CLAUDE.md` so the operating
  rules load every session.
- **`_` prefix ⇒ schema-governed** — every section folder governed by a
  `_schema/_<section>.md` rulebook is `_`-prefixed, domain-level or vault-wide (the prefix
  means "governed by a rulebook," not "core-only"). Reversed 2026-09-11 — it used to mean
  the opposite, that only `_core/`/`_schema/` and their contents got the prefix and a
  domain's own live sections were bare. `_code`, `_atomic`, `_maps`, and `_user` are not
  part of that reversal (see Open questions).
- **Log every structural change** to `_core/_logs/` (vault) or `<domain>/_logs/`, and keep
  this file's Changelog current — the standing rule from `_core/CLAUDE.md`.
- Dated event notes (`logs`, `decisions`, `meetings`, `daily`) are append-only; a past
  note is never rewritten.
- `code/` pages carry `commit:` (last commit to touch the page's `source:` paths) +
  `reconciled:` instead of `created:` / `source:`.
- **Links are standard Markdown, `[text](path)` — not Obsidian `[[wikilinks]]`** (decided
  2026-09-11, see `_core/CLAUDE.md` → Link format). Chosen once the vault's structure
  stabilized enough that files stop moving often: a Markdown link is directly actionable
  by an AI co-developer reading the file tree, at the cost that nothing rewrites its path
  automatically on a move — the `fix-links` skill (`_core/_skills/`) is the mechanical
  upkeep for that trade-off. Reasoning:
  `_core/_atomic/wikilink-aliases-separate-target-from-display.md` and
  `_core/_atomic/link-format-choice-depends-on-maintainer-and-consumer.md`.

## Open questions

- **Section mount point.** `_core/_schema/__README.md` marks most sections "mounts at:
  either" as provisional — whether the shared sections live at the vault root, per-domain,
  or both is unsettled. Current practice: `logs` and `decisions` split by scope
  (vault-wide vs `ConnectIt`); everything else vault-wide in `_core/`.
- **Schema near-duplicates / gaps.** Raised in
  [`_core/_ask-vault/2026-09-09-schema-similarity-and-gaps.md`](_core/_ask-vault/2026-09-09-schema-similarity-and-gaps.md)
  — needs a direct schema-audit pass, not `/ask-vault` (which reads content only).
- **`ideas.md`** (vault root) — a raw design-thinking pile from early Sept 2026; partly
  captured into `ConnectIt/` notes, partly not. Needs routing into `_core/_clippings/`
  or the relevant domain, or deletion.
- **Known-issues follow-through.** `ConnectIt/code/__INDEX.md` `## Known issues` now records
  the structural-overlap register and two confirmed correctness bugs extracted from the
  in-repo docs — recorded, not triaged. Each needs an owner decision or a C++ change.
- **Legacy MVVM pipeline still in the tree.** Documented as dead
  (`ConnectIt/decisions/2026-09-08-retire-legacy-mvvm-pipeline`); the actual deletion is a
  pending C++ change once non-C++ references are ruled out.
- **`Development/CLAUDE.md` has several Markdown links with no real target** —
  `_schema/`, `game-projects/`, and two single-word paths (`career-coach.md`, `user.md`)
  whose own display text names a fuller path than the link actually points to. Found
  2026-09-11 while explaining the link-format switch; not yet fixed. A candidate first
  real run for the new `fix-links` skill.
- **Repo-root `.claude/skills/` still holds a separate, older skill set**
  (`ask-the-board`, `improve-system`, `session-close`, `session-open`, plus stale
  `ask-vault` / `reconcile-tasks` mirrors predating the `process-*` rename) that doesn't
  match this vault's `_core/_skills/` catalog or its `ConnectIt-Vault/.claude/skills/`
  mirrors. Likely belongs to `Development/`'s own skill set; not reconciled.
- **`_code`/`_atomic`/`_maps`/`_user` still bare.** The 2026-09-11 `_`-prefix reversal
  (see Conventions) covers every other schema-governed section folder, domain or
  vault-wide; these four kept their old bare naming on purpose — a separate future pass
  if full consistency across every section kind is wanted.

## Changelog

- **2026-09-11** — Reversed the section-folder naming rule: every schema-governed section
  folder is now `_`-prefixed, domain-level or vault-wide, not just `_core/`/`_schema/`
  (the pre-existing `optimal-co-developer/_skills/` was the tell). `git mv`'d the 5 bare
  instances that existed (`board-of-advisors/{questions,skills,knowledge}/` →
  `_questions/`/`_skills/`/`_knowledge/`, `ConnectIt/{decisions,logs}/` →
  `_decisions/`/`_logs/`), flipped the rule text and every mount/location line in
  `_core/_schema/__README.md` + 12 section rulebooks, and fixed every reference across
  `_core/_templates/`, `_core/_skills/`, `_core/_ask-vault|_attachments|_clippings|
  _questions|_tasks|_logs/`, `Development/board-of-advisors/` (`CLAUDE.md`,
  `board-of-advisors.md`, `_skills/ask-the-board.md`, 3 advisor wiki notes), and
  `ConnectIt/CLAUDE.md`. Retargeted all 34 `[[wikilinks]]` under `ConnectIt/` that pointed
  at the old `decisions/`/`logs/` paths. Built out `optimal-co-developer`'s schema
  folders — `_meetings/`, `_questions/`, `_decisions/`, `_daily/` (each a seeded
  `__README.md`) and `_tasks/` (`__README.md` + `active`/`suspended`/`complete.md`) — and
  added the `/ask-partner` skill (`optimal-co-developer/_skills/ask-partner.md`): a
  single-voice design-sparring consultation in the co-developer's own voice, counterpart
  to `/ask-the-board`, logging each consultation to `_questions/`. Fixed
  `optimal-co-developer/CLAUDE.md`'s file-table links (`co-developer/` vs `me/` split)
  and its stale extraction caveat, and added a new "Default posture" section to
  `Development/CLAUDE.md`: always answer as the co-developer defined in `soul.md` /
  `identity.md` unless a skill's own instructions define a different voice for that
  output. Extended `/process-skills` to also mirror `<domain>/_skills/*.md` skills
  (Pass 2, with domain-scoped logging and name-collision flagging), then ran it by hand:
  created the long-missing `.claude/skills/ask-the-board/SKILL.md` mirror and the new
  `.claude/skills/ask-partner/SKILL.md`, and refreshed the 5 vault-wide mirrors whose
  authored copies changed in this pass (`ask-vault`, `fix-links`, `process-data`,
  `process-skills`, `process-tasks`). Log: `_core/_logs/2026-09-11-1500.md`.
- **2026-09-11** — Set the vault's link-format standard: standard Markdown links
  (`[text](path)`), not Obsidian `[[wikilinks]]`, decided once `_core/` stabilized enough
  that a Markdown link's lack of auto-rewrite-on-move stopped being the deciding cost.
  Documented in `_core/CLAUDE.md` → Link format, with two new `_core/_atomic/` notes
  (`wikilink-aliases-separate-target-from-display`,
  `link-format-choice-depends-on-maintainer-and-consumer`) carrying the reasoning.
  Converted every wikilink in the 9 `CLAUDE.md` files (root, `_core/`, `ConnectIt`, the 7
  plugins) to Markdown links. Added the `fix-links` skill (`_core/_skills/` +
  `ConnectIt-Vault/.claude/skills/` mirror) as the mechanical upkeep for links that go
  stale on a file move. `Development/CLAUDE.md` was left as-is (already Markdown-link
  style) — its existing broken links are flagged above, not fixed here.
- **2026-09-11** — Ran `/process`'s full sweep for the first time: 0 drift across 35
  `code/` pages checked against git history, nothing to move in `tasks/`, no `data/raw/`
  or rogue `atomic/` folders to reconcile. Bootstrapped `.claude/skills/` (didn't exist
  before) with all 7 vault-wide skills, so they're now actually invocable as
  `/ask-vault`, `/process`, and the five `/process-<schema>` commands. Log:
  `_core/_logs/2026-09-11-0112.md`.
- **2026-09-11** — Split the folder-explainer note kind into two, mirroring the
  canonical `_core` change: `__README.md` (static explainer) and `__INDEX.md` (a living
  content list), templated by `_core/_templates/TReadMe.md` / `TIndex.md`.
  `_core/_skills/__README.md` split into a trimmed `__README.md` (rules) + new
  `__INDEX.md` (live-skills catalog); `_core/_logs/__README.md` →
  `_core/_logs/__INDEX.md` and `_core/_skills/drafts/__README.md` →
  `_core/_skills/drafts/__INDEX.md` (both keep their own content, only the filename +
  framing changed). Renamed every domain `code/index.md`, `code/recipes/index.md`,
  `code/systems/index.md`, and `ConnectIt/decisions/index.md` to `__INDEX.md`;
  `ConnectIt/logs/README.md` → `ConnectIt/logs/__INDEX.md` (it was index-shaped despite
  the old name); `Development/board-of-advisors/knowledge/raw/README.md`,
  `knowledge/wiki/README.md`, and `questions/README.md` → `__README.md` (missing the
  vault's own `__` prefix convention). Updated every wikilink/reference to a renamed
  file. `_core/_templates/TSchema.md` and `TSchemaTasks.md` (already locally customized)
  were left untouched, as were this vault's own `_core/_logs/` dated entries.
- **2026-09-10** — migrated the whole vault onto `_core/`: retired all `_section.md` and
  the vault-root `_schema/`; gave each of the 8 domains a `CLAUDE.md` router; created
  this file; re-ingested every domain's `code/` to the consolidated `_code` schema
  (`systems/`/`recipes/` nested under `code/`, `updates/` folded into per-page
  `## Changes`, em-dash mojibake + broken `See also` links repaired); folded
  `architecture/` into `ConnectIt/code/index.md`'s Map; deleted `devlog/` (no real
  entries); created `ConnectIt/decisions/` (5 dated notes) and `ConnectIt/code/runtime-state.md`
  from the extracted `old/` docs, then removed `old/`; mirrored the two vault-wide skills
  to `.claude/skills/`; removed stale `extras/templates/`. Logs:
  `_core/_logs/2026-09-10-0900…1200`.
- **2026-09-09** — `_core/` module built out: `_schema/` moved inside `_core/` with
  `_`-prefixed rulebooks; `_code` consolidation; `_frameworks`/`_systems`/`_recipes`/
  `_updates` dropped; `_tasks`/`_questions`/`_attachments`/`_atomic` added; `_inbox` →
  `_clippings`; `/ask-vault` skill + `_core/_ask-vault/`; log-on-change rule added;
  `_extras/` → `_core/` rename; root `CLAUDE.md` → thin stub + `@import`.
- **2026-09-06** — first vault build: per-plugin sections, the `code`/`systems`/`recipes`
  ingestion family, `architecture/` + `devlog/`.
