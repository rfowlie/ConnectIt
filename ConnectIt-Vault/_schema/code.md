# _schema/code.md

A `code/` folder holds **per-type help-wiki pages** for a section's source code — one page
per public class, struct, enum, or interface a developer works with. It is the drill-down
layer under the section's `high-level.md`.

Unlike [`knowledge.md`](knowledge.md), a `code/` folder has **no `raw/`**. The source
file is already under version control and changes often; a frozen copy would just go
stale. Instead every page is *derived* and carries a **provenance anchor** — the source
paths it covers and a `commit:` — so drift is detected against git, not against a stored
copy. `systems.md` and `recipes.md` follow this same provenance model.

**`commit:` is the last commit that touched the page's `source:` paths**, not HEAD:

```
git log -1 --format=%h -- <source path> [<source path> ...]
```

This makes the anchor point at the *source's actual state*. A page is fresh as long as
that command still returns the recorded `commit:` — i.e. as long as
`git log <source paths> <commit>..HEAD` is empty. `reconciled:` is the date a human last
read the page against the code; the two are independent.

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
  index.md              inventory: type | kind | role | source | status  (current / stub / stale)
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
reconciled: <YYYY-MM-DD>                  # date a human last checked the page vs the code
commit: <short SHA — last commit to touch the source: paths (git log -1 --format=%h -- …)>
---
```

`index.md` uses the same two fields; its `source:` is the section's whole source root, so
its `commit:` is the last commit to touch the section at all. Per-row `status` in the
index is just `current` / `stub` / `stale` — each page's own frontmatter is the single
place the anchor lives.

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

- **No `raw/`.** Every page names its `source:` files and a `commit:` = the last commit
  that touched them (`git log -1 --format=%h -- <paths>`). That is the provenance anchor.
- **Refresh, don't diff.** To check freshness: `git log <source paths> <commit>..HEAD` —
  empty means current. If anything changed, re-read the source, rewrite the affected
  sections, and reset `commit:` to the new `git log -1 --format=%h -- <paths>` and
  `reconciled:` to today.
- **One page per primary type.** Grouped small types (all enums in a header, small PODs)
  get one page named for the file stem.
- **Don't restate the in-repo reference.** See *Division of labour*.
- `index.md` carries a row for every public type; its `status` is `current` (page exists
  and is reconciled), `stale` (page exists, drift detected), or `stub` (no page yet).

## Deviations

A section's `code/` adds a `code/README.md` starting
`Extends _schema/code.md. Differences:` when it needs section-specific rules (e.g. a
plugin with no in-repo docs that wants fuller pages, or a different page-grouping rule).
