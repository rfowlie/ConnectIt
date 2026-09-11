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
| `logs` | Append-only machine-written maintenance history, one dated note per run | `_core/_logs/` (vault) · `ConnectIt/logs/` |
| `decisions` | One dated note per non-obvious settled call | `ConnectIt/decisions/` |
| `skills` | Reusable procedures, authored here, mirrored to `.claude/skills/` | `_core/_skills/` |
| `atomic` | Single-idea Zettelkasten pool — **single instance, `_core/_atomic/` only** | `_core/_atomic/` |
| `clippings` | Unprocessed capture dump; empty is the goal | `_core/_clippings/` |
| `tasks` `questions` `ask-vault` `attachments` | Vault-level instances | `_core/_*/` |
| `data` `user` `people` `meetings` `daily` `maps` | Available, not yet used | — |

Full rulebook per section: `_core/_schema/_<section>.md`.

## Conventions

- Context files are always `CLAUDE.md` at every level (auto-loading; extract-to-repo-root
  with no rename). The root `CLAUDE.md` opens with `@_core/CLAUDE.md` so the operating
  rules load every session.
- `_` prefix ⇒ structural (`_core/`, `_schema/`, and their contents). Live domain
  sections are bare.
- **Log every structural change** to `_core/_logs/` (vault) or `<domain>/logs/`, and keep
  this file's Changelog current — the standing rule from `_core/CLAUDE.md`.
- Dated event notes (`logs`, `decisions`, `meetings`, `daily`) are append-only; a past
  note is never rewritten.
- `code/` pages carry `commit:` (last commit to touch the page's `source:` paths) +
  `reconciled:` instead of `created:` / `source:`.

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

## Changelog

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
