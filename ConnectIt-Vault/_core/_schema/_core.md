# _schema/_core.md

`_core/` is **the vault's non-domain folder** — everything that isn't a domain and
isn't a loose root file. It holds the spec, the templates, and the **vault-wide instance
of every section**. One per vault, at the vault root, part of the fixed skeleton.

`_core/CLAUDE.md` is the module's router — start-of-session pointers, the standing rules
(log every change; keep `ABOUT.md` current), and how to drop `_core/` into another
vault. This rulebook is the spec; that file points, it doesn't restate.

## Where it mounts

Vault root only, exactly one. Never inside a domain — a domain keeps its own
`<domain>/<section>/` folders and its `CLAUDE.md`.

## Contents

| Folder | Holds | Governed by |
|---|---|---|
| `_core/_schema/` | The mechanical spec — one rulebook per section kind, plus `README.md`. | itself |
| `_core/_templates/` | Every note template (`TSchema*`, `TCode*`). Obsidian's Templates plugin points here. | this file |
| `_core/_logs/` | The vault-wide maintenance log. | [`_schema/_logs.md`](_logs.md) |
| `_core/_clippings/` | The vault-wide unprocessed capture dump. | [`_schema/_clippings.md`](_clippings.md) |
| `_core/_skills/` | Authored home for vault-wide skills; mirrored to `.claude/skills/`. | [`_schema/_skills.md`](_skills.md) |
| `_core/_ask-vault/` | Q&A records from the `/ask-vault` skill. | this file + the `ask-vault` skill |
| `_core/_atomic/` | The vault's **single** atomic-notes pool (only ever here). | [`_schema/_atomic.md`](_atomic.md) |
| `_core/_tasks/` | Vault-level task tables. | [`_schema/_tasks.md`](_tasks.md) |
| `_core/_questions/` | Vault-level open questions. | [`_schema/_questions.md`](_questions.md) |
| `_core/_attachments/` | Vault-level non-note files (immutable). | [`_schema/_attachments.md`](_attachments.md) |

## Rules

- **Non-domain only.** `_core/` holds vault-scoped instances and the spec — no
  domain-scoped content. A domain's own `logs/` / `tasks/` / `clippings/` / etc. live in
  that domain, not here.
- **Named children are `_`-prefixed** — same convention as `_schema/`.
- **Fixed skeleton.** `_core/` and its named children exist from the start; they are
  not opt-in-on-first-use like the folders a domain creates.
- Each named child is governed by its section schema (see the table) or specified here
  (`_templates/`).
- **`_core/_atomic/` is the one exception to "either" mounting** — the atomic pool is
  single-instance and only ever lives here.
- **Loose files at `_core/`'s top level are unsorted design / meta material** —
  reconcile each into the vault or delete it. A to-do pile, not an archive.
- **Stays with the vault.** When a domain is extracted, it takes its own `<section>/`
  folders; `_core/` (spec, templates, vault-wide instances) does not travel with it.

## Template

No note template — `_core/` defines a fixed folder layout. Its children carry their own
templates where their governing schema defines one.

## Deviations

Add `_core/README.md` starting `Extends _schema/_core.md. Differences:` if a vault
adds or omits a non-domain folder.
