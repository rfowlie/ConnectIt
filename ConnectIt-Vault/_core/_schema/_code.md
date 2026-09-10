# _schema/_code.md

A `code/` folder is the drill-down documentation layer for a domain's source code — one
folder holding several typed notes (per-type pages, per-flow narratives, per-task
recipes) plus an index that maps the codebase. Like `_atomic/` is one folder of one note
kind, `code/` is one folder of a few code-note kinds. Mounts only in a project domain
that contains source code.

## Structure

```
code/
  index.md               inventory of every public type + a MAP of the codebase's core
                         directories / plugins / major systems (where to start)
  <TypeName>.md           one page per primary public class / struct / enum / interface
  <FileStem>.md           one page for a file's cluster of small types (Enums.md, …)
  systems/<flow-slug>.md  per-flow narrative pages — how several types collaborate
  recipes/<task-slug>.md  per-task step-by-step procedures for a concrete change
```

Flat within each level — no source-tree mirroring.

## Provenance model

A `code/` folder has **no `raw/`**. The source is already under version control and
changes often; a frozen copy would just go stale. Every page is *derived* and carries a
**provenance anchor** — the `source:` paths it covers and a `commit:` — so drift is
checked against git, not a stored copy.

**`commit:` is the last commit that touched the page's `source:` paths**, not HEAD:

```
git log -1 --format=%h -- <source path> [<source path> ...]
```

A page is fresh while `git log <source paths> <commit>..HEAD` is empty. `reconciled:` is
the date a human last read the page against the code; the two are independent. To
refresh: re-read the source, rewrite the affected sections, reset `commit:` and
`reconciled:`. This replaces the baseline `created:` / `source:` frontmatter for every
note in `code/`.

## `index.md`

Two jobs in one file:

1. **Inventory** — a row for **every** public type: `type | kind | role | source |
   status` (`current` / `stub` / `stale`), including types with no page yet.
2. **Map** — a short guided tour of the codebase: the core directories, the plugins /
   modules, the major systems, and where a new developer should start reading. This is
   the architecture MOC (it absorbs what a separate "frameworks" note used to do).

## `<TypeName>.md` — per-type page

Frontmatter: `schema: code`, `kind:`, `role: primary | internal`, `source:` (`.h` [+
`.cpp`]), `reconciled:`, `commit:`. Body, fixed order, terse (about one screen):

1. **What it is** — 1–2 sentences.
2. **When you touch this** — the situations that bring a developer here.
3. **Entry points** — the handful of members you call or override. Not a full list.
4. **Collaborators** — what it talks to; delegates it fires / consumes; `[[wikilinks]]`
   to other `code/` pages and `systems/` flows.
5. **Gotchas** — authority / replication, ordering, footguns.
6. **Cross-impact** — concrete "change this → also update <files/types>".
7. **Changes** — dated bullets, newest first: what changed and why it matters. Append
   only; this is the type's change history (it replaces a separate `updates/` folder).
8. **See also** — in-repo `Docs/` anchor, related `systems/` / `recipes/` pages.

If the code ships its own reference docs, those stay authoritative for the exhaustive
per-member reference; a `code/` page adds only the co-developer overlay.

## `systems/<flow-slug>.md` — per-flow page

Frontmatter: `schema: systems`, `spans:` (paths/types the flow crosses), `reconciled:`,
`commit:`. Body, fixed order:

1. **What happens** — the flow in 3–6 sentences.
2. **Diagram** — a Mermaid diagram (`sequence`, `stateDiagram`, or `flowchart`).
   **Required.** If a flow can't be drawn it's probably a single-type concern for a
   `code/` page.
3. **Steps** — the ordered walkthrough; each hop names the type + member, `[[wikilinked]]`
   to its `code/` page.
4. **Gotchas** — failure modes, timing / authority constraints.
5. **Cross-impact** — what else must change if this flow changes.
6. **See also** — in-repo `Docs/`, related `code/` / `recipes/` pages.

Link to `code/` pages; don't re-describe types. `systems/index.md` is one row per flow:
`flow | what it does | status`.

## `recipes/<task-slug>.md` — per-task page

Frontmatter: `schema: recipes`, `task:` (one-line goal), `touches:` (files/types a correct
implementation edits — also the cross-impact list), `reconciled:`, `commit:`. Body, fixed
order:

1. **Goal** — what you'll have when done.
2. **Prerequisites** — what must already exist.
3. **Steps** — numbered; each names the concrete file / type / member and the change.
   Snippets minimal — link to the `code/` page for detail.
4. **Verify** — how to confirm it works.
5. **Pitfalls** — the mistakes people make on this task.
6. **See also** — the `code/` / `systems/` pages behind the steps.

Executable without leaving the vault + the named source files. `recipes/index.md` is one
row per recipe: `task | when you'd do it | status`.

## Rules

- **No `raw/`** anywhere in `code/`. Provenance is the `commit:` anchor.
- **Refresh, don't diff** — per the provenance model above, for every page kind.
- **One page per primary type**; grouped small types share a file-stem page.
- **`index.md` carries a row for every public type** and the codebase map.
- **The `systems/` diagram is mandatory.**
- **`## Changes` is append-only** — never rewrite a past entry.
- Created on first use, once the domain's source has been ingested. Don't pre-scaffold.

## Templates

`[[TSchemaCode]]` (per-type, incl. `## Changes`) · `[[TCodeSystem]]` (flow page) ·
`[[TCodeRecipe]]` (task page). `index.md` has no template — it's an inventory + map.

## Deviations

Add `code/README.md` starting `Extends _schema/_code.md. Differences:` when a domain needs
section-specific rules (e.g. a plugin with no in-repo docs that wants fuller pages, a
different page-grouping rule, or a domain that treats each plugin as its own sub-domain).
