# _schema/knowledge.md

A `knowledge/` folder is **reference** — stable facts and understanding, not a to-do
list. If it's a task or a next action, it belongs in the section's lead doc or a
`decisions/` note, not here.

## Subfolders

| Subfolder | Holds |
|---|---|
| `raw/` | Source material — transcripts, articles, posts, talks, specs — ingested **verbatim**. One subfolder per source, by kebab-case slug; files numbered `NN-` in ingest order; each with YAML frontmatter recording where it came from. |
| `wiki/` | One synthesis per source, built from everything in the matching `raw/<slug>/`. This is the version other notes and skills read. |

A section may add its own named subfolders under `knowledge/` (e.g. `frameworks/` for
named mental models, `glossary/` for terms). Document any such addition in the section's
`_section.md` or a local `knowledge/README.md`. Create subfolders on first use.

## Rules

- **`raw/` is verbatim.** Never clean up, summarize, or edit source text — its value is
  fidelity. Caveats (garbled captions, a second speaker, a hosted conversation) go in the
  frontmatter `note:`, not inline.
- **Every `raw/<slug>/` file is reflected in `wiki/<slug>.md`.** A new raw file means a
  wiki update. No wiki claim should lack raw support.
- **Paraphrase in the source's voice in `wiki/`;** reserve quotation marks for phrases
  actually attested in `raw/`.
- **One canonical file per subject.** If the section has a single reference doc that is
  the source of truth for some set of facts, update it in place when facts change; don't
  scatter the same facts across other files.

## Deviations

A section's `knowledge/` (or a subfolder of it) adds a local `README.md` starting
`Extends _schema/knowledge.md. Differences:` when it needs section-specific rules — e.g.
a per-source corpus convention.
