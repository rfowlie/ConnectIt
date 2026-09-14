# CLAUDE.md — `_core/`

`_core/` is this vault's **non-domain module** — its co-development operating system.
Everything structural lives here: the spec (`_schema/`), the note templates
(`_templates/`), and the vault-wide instance of every section (`_logs/`, `_clippings/`,
`_skills/`, `_ask-vault/`, `_atomic/`, `_tasks/`, `_questions/`, `_attachments/`). A
domain gets `<domain>/<section>/`; the vault gets `_core/_<section>/`.

Full rulebook: [`_schema/_core.md`](_schema/_core.md). This file points; it does not
restate the spec.

## Link format

**Vault-wide standard: standard Markdown links, `[text](path)` — not Obsidian
`[[wikilinks]]`.** Chosen 2026-09-11 once the `_core/` structure stabilized enough that
files stop moving often: a Markdown link's path is directly actionable by an AI
co-developer working from the file tree (no vault-wide name search to resolve a target),
at the cost of needing a human or the [`fix-links`](_skills/fix-links.md) skill to keep
paths correct across a move — an acceptable trade now that restructuring is rare. Full
reasoning: [`wikilink-aliases-separate-target-from-display`](_atomic/wikilink-aliases-separate-target-from-display.md)
and [`link-format-choice-depends-on-maintainer-and-consumer`](_atomic/link-format-choice-depends-on-maintainer-and-consumer.md).
Run `/fix-links` after any file move or rename to catch what a Markdown link's lack of
auto-rewrite would otherwise leave stale.

## Start of session

When a session will touch vault structure, read first:

1. [`_schema/__README.md`](_schema/__README.md) — how the vault works (the three layers, the
   section catalogue, naming, extraction).
2. [`../ABOUT.md`](../ABOUT.md) — why it's shaped this way, and the decision history.

## Standing rules

**Log every change.** After any change to the vault — its structure, a `_schema/`
rulebook, `_core/`, a skill, or the reasoning behind the setup — before the change is
done:

1. Write one note to `_logs/<YYYY-MM-DD-HHMM>.md` per
   [`_schema/_logs.md`](_schema/_logs.md) / [`TSchemaLogs`](_templates/TSchemaLogs.md) —
   one per working session / change-set, append-only, never edited after writing. Add its
   line to [`_logs/__INDEX.md`](_logs/__INDEX.md) (newest first).
2. Update [`../ABOUT.md`](../ABOUT.md) when the structure or its rationale shifted — the
   section table, Conventions, Open questions, and a Changelog line.

(A `.claude/` hook would enforce step 1 more strictly in a live vault; this instruction
is the portable form.)

## Using this module in another vault

`_core/` is built to be lifted into any vault:

1. Copy the whole `_core/` folder to the **host vault's root** (→ `host/_core/`).
2. In the host's root `CLAUDE.md`, add `@_core/CLAUDE.md` (or make that line the whole
   file) so these rules load at the start of every session — a nested `CLAUDE.md`
   otherwise only loads once a file inside it is touched.
3. The host's existing top-level folders become **domains** — each gets its own
   `CLAUDE.md` and opts into the sections it uses (`_schema/__README.md` → "Domains").
4. `_core/_<section>/` are the vault-wide instances; a domain keeps its own
   `<domain>/<section>/`. `_core/` does not travel into an *extracted* domain.
