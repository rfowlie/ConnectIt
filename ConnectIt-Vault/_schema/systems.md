# _schema/systems.md

A `systems/` folder holds **per-flow narrative pages** — how several types collaborate to
do one thing (a turn-end sequence, a request-routing path, a state-machine lifecycle).
One page per flow. Where `code/` is per-type, `systems/` is per-flow.

Its job is to add what a per-class reference and a prose narrative can't: **a diagram**,
an ordered walkthrough that names every hop, and an explicit cross-impact list.

Follows the provenance model in [`code.md`](code.md) — no `raw/`, anchor to a commit,
refresh by re-reading the source.

## Layout

```
systems/
  index.md              one row per flow: flow | what it does | status
  <flow-slug>.md         one page per flow (e.g. turn-end-tag-gate.md)
```

## Page frontmatter

```yaml
---
schema: systems
spans:                   # the source paths / types this flow crosses
  - <repo-relative path>
reconciled: <YYYY-MM-DD>
commit: <short SHA>
---
```

## Page body (fixed order)

1. **What happens** — the flow in 3–6 sentences.
2. **Diagram** — a Mermaid diagram (`sequence`, `stateDiagram`, or `flowchart` as fits).
   **Required.**
3. **Steps** — the ordered walkthrough; each hop names the type + member, `[[wikilinked]]`
   to its `code/` page.
4. **Gotchas** — failure modes, timing / authority constraints.
5. **Cross-impact** — what else must change if this flow changes.
6. **See also** — in-repo `Docs/`, related `code/` / `recipes/` pages.

## Rules

- **Link to `code/` pages; don't re-describe types.** A `systems/` page is about the
  wiring between them.
- **The diagram is mandatory.** If a flow can't be drawn, it's probably a single-type
  concern that belongs on a `code/` page.
- Provenance + refresh per [`code.md`](code.md).

## Deviations

Add `systems/README.md` starting `Extends _schema/systems.md. Differences:` for
section-specific rules.
