# _schema/

The **mechanical spec** for this vault's structure. One rulebook per *kind* of section
folder, plus the rules for how sections, domains, and routers fit together.

> This file is the spec. The **reasoning**, decision history, and open questions live in
> the vault's `ABOUT.md`. If you want to know *why* the structure is shaped this way,
> read that. If you want to know *how it works*, read this.

`_schema/` is not auto-loaded. A `CLAUDE.md` points here; it never restates these rules.

> **`_schema/` in these files is shorthand for `_core/_schema/`** — the folder lives
> inside `_core/`. Rulebook H1s keep the short `# _schema/_<section>.md` form as a
> stable label.

---

## The three layers

| Layer | File | Auto-loaded? | Scope | Answers |
|---|---|---|---|---|
| **Router / context** | `CLAUDE.md` | yes — every ancestor dir of the file you're editing | one domain, or the vault | where am I, what is this, current state, what to read next |
| **Schema / spec** | `_schema/_<section>.md` | no (referenced) | any folder of that kind, anywhere | what a `<section>/` *is*, its invariants, naming rule, what must never happen there |
| **Local override** | `<domain>/<section>/__README.md` | no (referenced) | one specific folder | how *this* section differs from the schema |

On conflict: the **schema wins** on section rules; the `CLAUDE.md` wins on state and routing.

## Domains

A **domain** is a top-level folder for one area of work or life — a business, a job, a
project, personal. Each domain:

- has its own `CLAUDE.md` router, written to stand alone (it could be lifted into its own
  repo with zero renaming);
- **opts into a subset of sections** — it creates only the ones it uses;
- creates each section **on first use**. Nothing is pre-scaffolded (git doesn't track
  empty directories anyway).

An **active project is a domain** — a root-level folder with its own `CLAUDE.md` — not a
section type. A domain that only groups child domains carries a `__INDEX.md` (a list of
its children) instead of a full router.

## The section catalogue

| Section | Purpose | Template | Mounts at |
|---|---|---|---|
| `_clippings` | Unprocessed capture dump (Web Clipper output); empty is the goal state | — (folder layout) | either |
| `_data` | Reference: `raw/` (verbatim text sources) + `wiki/` (synthesis) | — (folder layout) | either |
| `_attachments` | Non-note files (images, PDFs, audio) — immutable, like `raw/` | — (folder layout) | either |
| `_user` | Everything about the owner | `[[TSchemaUser]]` | either |
| `_decisions` | One dated note per non-obvious settled call | `[[TSchemaDecisions]]` | either |
| `_questions` | One note per open, unresolved question | `[[TSchemaQuestions]]` | either |
| `_tasks` | Work tracked as status tables (`active` / `suspended` / `complete`) | `[[TSchemaTasks]]` | either |
| `_skills` | Reusable procedures Claude can run on demand | `[[TSchemaSkills]]` | either |
| `_logs` | Append-only record of automated maintenance passes | `[[TSchemaLogs]]` | either |
| `_people` | One note per person — a lightweight CRM | `[[TSchemaPeople]]` | either |
| `_meetings` | One dated note per meeting or call | `[[TSchemaMeetings]]` | either |
| `_daily` | One free-form note per day — the human's capture surface | `[[TSchemaDaily]]` | either |
| `_maps` | Hand-curated Maps of Content — deliberate entry points | `[[TSchemaMaps]]` | either |
| `_atomic` | One self-contained note per idea — a flat Zettelkasten pool | `[[TSchemaAtomic]]` | `_core/_atomic/` only |
| `_code` | A domain's source-code docs — per-type pages, `systems/`, `recipes/`, an index+map | `[[TSchemaCode]]` · `[[TCodeSystem]]` · `[[TCodeRecipe]]` | domain |
| `_core` | The vault's non-domain folder — spec, templates, vault-wide section instances | — (container) | root (fixed) |

"Mounts at: either" is provisional — whether the shared sections live at the vault root,
inside each domain, or both, is an open question in `ABOUT.md`. `_atomic` is the
exception: **single-instance**, only ever `_core/_atomic/` (a second `atomic/` anywhere
is an error).

`_clippings`, `_data`, `_attachments`, and `_core` have no `TSchema*` file — they
define a folder layout, not a note of fixed shape.

`_code` is **code-derived**: its pages are generated from source under version control
and carry a `commit:` provenance anchor (the last commit to touch the page's `source:`
paths) instead of the baseline `created:` / `source:` frontmatter. It absorbs what used
to be three sections — per-type pages, `systems/` per-flow narratives, `recipes/`
per-task procedures — plus a `## Changes` history on each type page. Mounts only inside a
project domain with source code.

## The non-domain folder (`_core/`)

A vault has two kinds of root-level citizen: the **domains** (one `CLAUDE.md`-routed
folder per area of work or life), and **`_core/`** — everything that isn't a domain.
`_core/` holds the spec (`_schema/`), the templates (`_templates/`), and the
**vault-wide instance of every section**: `_logs/`, `_clippings/`, `_skills/`,
`_ask-vault/`, `_atomic/` (the only instance), `_tasks/`, `_questions/`, `_attachments/`.
A domain gets `<domain>/<section>/`; the vault gets `_core/_<section>/`. `_core/` is
fixed, single, root-only, and does *not* travel with an extracted domain. See
[`_schema/_core.md`](_core.md) for the spec, and `_core/CLAUDE.md` for the standing
operating rules (log every change, keep `ABOUT.md` current) and the steps to drop
`_core/` into another vault.

## Naming conventions

- Schema folder is `_core/_schema/`. Each rulebook is `_<section>.md`; its H1 keeps the
  short label `# _schema/_<section>.md`.
- **Live section folders a domain creates are bare** — `data/`, `people/`, `decisions/`.
  The `_` prefix is only for `_schema/`, `_core/`, and their contents.
- Context files are always named `CLAUDE.md` at every level — required for auto-loading,
  and it lets any folder be extracted to a repo root with no renaming.
- Templates live in `_core/_templates/`: `TSchema<Section>.md` for a section's note
  shape (referenced from the schema as `[[TSchema<Section>]]`), `TCode*` for the code-doc
  note kinds, `TSchema.md` for authoring a new schema file, `TReadMe.md` / `TIndex.md`
  for the two folder-level note kinds above.
- **Two kinds of folder-level note, both `__`-prefixed and in caps** so they sort above
  every ordinary note in that folder and read as machinery, not content, at a glance:
  - **`__README.md`** — a static explainer: what the folder is, its rules, its note
    shape. Doesn't grow. Template: [[TReadMe]].
  - **`__INDEX.md`** — a living list of the folder's actual contents (dated, curated, or
    an inventory table), meant to grow as content is added. Template: [[TIndex]].
  A folder carries whichever actually describes it — including both, side by side, when
  it genuinely needs a rules explainer *and* a growing content list (e.g. `_skills/`:
  `__README.md` for the rules, `__INDEX.md` for the live-skills catalog). Don't force a
  static folder to carry an index it doesn't need, and don't let an index's list double
  as the folder's only explanation when real rules exist too. Applies everywhere the
  pattern appears: the spec index (this file), every `_core/_<section>/`, and every
  domain-level instance.
  - A domain-level **deviation note** (see "How a domain deviates from a schema" below)
    is always `__README.md` — it's explaining a difference, not listing content.

## What earns a schema

A schema governs a **file structure** — a folder layout, or a separation that's valuable
to keep domain-specific and consistent (like `_logs/`). If a would-be schema is only a
*note shape* with no structure to enforce, it belongs in `_core/_templates/` as a
`T*` template, not in the catalogue. Every schema **opens with a structure map** of the
folder it governs, so the rulebook doubles as the folder's MOC (this is why there's no
separate `_glossary` — a schema self-describes its own contents).

## Baseline frontmatter

Every note carries, at minimum:

- `created: <YYYY-MM-DD>` — and `updated:` on notes that are revised in place.
- `tags:` — at least the section tag (`person`, `meeting`, `framework`, …).
- `source:` — where the content came from, when there's an external origin.

`_code` notes (including `code/systems/` and `code/recipes/`) replace `created:` /
`source:` with a `commit:` provenance anchor — see [`_schema/_code.md`](_code.md).

## Dated event notes

`_decisions`, `_meetings`, `_logs`, and `_daily` are **dated, append-only, one note per
event**. Filename `<YYYY-MM-DD>[-<slug>].md` (`_logs` adds `-HHMM`); the date may live in
frontmatter as `date:` / `Date:` instead of `created:`. A past note is never rewritten —
you add a new one (`_decisions` marks the old one `superseded by [[…]]`; `_logs`
corrections go in the next entry). `_daily` is the one that's edited through its own day.

## How a domain deviates from a schema

Add a `__README.md` **inside that section folder**, opening:

```
Extends _schema/_<section>.md. Differences:
```

…then list only the deltas. The domain's `CLAUDE.md` gets one line noting the override
and linking to it — but the rule text lives in the folder it governs, so it travels if
the domain is extracted.

Only create a **new named section** (with its own `_schema/_<name>.md`) when the
behaviour is a genuinely different *kind* of thing with a file structure to enforce — not
a one-off tweak, and not just a note shape (that's a template). Follow
`_core/_templates/TSchema.md` and its checklist.

## Extracting a domain into its own repo

1. `git subtree split` (or copy) the domain folder.
2. Copy `_core/` in alongside it — the spec and templates are shared. The
   `_core/_<section>/` **vault-wide instances do not travel**; the domain keeps its own
   `<domain>/<section>/` folders.
3. Promote the domain's `CLAUDE.md` to the new repo root; drop any "part of <vault>"
   framing lines.
4. Re-point or copy in anything the domain linked to outside itself.
