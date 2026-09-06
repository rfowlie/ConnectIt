# _schema/code.md

A `code/` folder holds **per-type help-wiki pages** for a section's source code — one page
per public class, struct, enum, or interface a developer works with. It is the drill-down
layer under the section's `high-level.md`.

Unlike [`knowledge.md`](knowledge.md), a `code/` folder has **no `raw/`**. The source
file is already under version control and changes often; a frozen copy would just go
stale. Instead every page is *derived* and carries a **provenance anchor** — the source
paths it covers and the commit it was last reconciled against — so drift is detected
against git, not against a stored copy. `systems.md` and `recipes.md` follow this same
provenance model.

## Division of labour

If the code ships its own reference docs (for Unreal plugins, a `Docs/README.md` +
`Docs/Systems.md`), **those stay authoritative** for exhaustive per-member reference and
narrative. A `code/` page adds only the co-developer overlay:

- the few entry points you actually call or override (not a full member dump)
- who it collaborates with, and through which delegates
- gotchas — authority/replication rules, ordering constraints, footguns
- cross-impact — "if you change this, also update …"
- a freshness anchor

For code with no in-repo docs, `code/` pages may be fuller.

## Layout

```
code/
  index.md              inventory: type | kind | role | source | reconciled@commit | status
  <TypeName>.md          one page per primary public type (e.g. UTurnBasedAction.md)
  <FileStem>.md          one page for a file's cluster of small types (e.g. Enums.md, Structs.md)
```

Flat — no source-tree mirroring. `index.md` lists **every** public type, including ones
with no page yet (`status: stub`).

## Page frontmatter

```yaml
---
schema: code
kind: UCLASS | USTRUCT | UENUM | UINTERFACE | class
role: primary | internal
source:
  - <repo-relative path to .h>
  - <repo-relative path to .cpp>          # omit if header-only
reconciled: <YYYY-MM-DD>
commit: <short SHA the page was checked against>
---
```

## Page body (fixed order, terse)

1. **What it is** — 1–2 sentences.
2. **When you touch this** — the situations that bring a developer to this type.
3. **Entry points** — the handful of members you call or override. Not a full list.
4. **Collaborators** — what it talks to; delegates it fires / consumes; `[[wikilinks]]`
   to other `code/` pages and `systems/` flows.
5. **Gotchas** — authority/replication, ordering, safe-vs-raw delegate surfaces, other
   footguns.
6. **Cross-impact** — concrete "change this → also update <files/types>".
7. **See also** — in-repo `Docs/` anchor, related `systems/` / `recipes/` pages.

Keep a page to about one screen. If it grows past that, the overflow belongs in
`systems/` or the in-repo reference, not here.

## Rules

- **No `raw/`.** Every page names its `source:` files and the `commit:` it was reconciled
  against. That is the provenance anchor.
- **Refresh, don't diff.** To update: `git log <source paths> <commit>..HEAD`. If anything
  changed, re-read the source, rewrite the affected sections, bump `commit:` and
  `reconciled:`.
- **One page per primary type.** Grouped small types (all enums in a header, small PODs)
  get one page named for the file stem.
- **Don't restate the in-repo reference.** See *Division of labour*.
- `index.md` carries a row for every public type; its `status` is `current` (page exists
  and is reconciled), `stale` (page exists, drift detected), or `stub` (no page yet).

## Deviations

A section's `code/` adds a `code/README.md` starting
`Extends _schema/code.md. Differences:` when it needs section-specific rules (e.g. a
plugin with no in-repo docs that wants fuller pages, or a different page-grouping rule).
