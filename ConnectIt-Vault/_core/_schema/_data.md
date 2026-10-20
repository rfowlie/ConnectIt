# _schema/_data.md

A `_data/` folder is **reference** — stable facts and understanding, not a to-do list.

## Subfolders

| Subfolder | Holds                                                                                                                                                                                         |
| --------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `raw/`    | Source material — transcripts, articles, posts, talks — ingested **verbatim**. One subfolder per source by kebab-case slug, files numbered `NN-` in ingest order, each with YAML frontmatter. |
| `wiki/`   | One synthesis note per source, built from everything in the matching `raw/<slug>/`.                                                                                                           |

Owner facts live in `_user`, not here. Mental models are `_atomic` notes tagged
`mental model`, not here. Binary source files (PDFs, audio, images) live in
`_attachments`, not `_data/raw/` — `raw/` is text sources only.

## Rules

- **`raw/` is verbatim.** Never clean up, summarize, or edit source text — its value is
  fidelity. Caveats (garbled captions, a guest speaker, a hosted conversation) go in the
  frontmatter `note:`, not inline.
- **Every `raw/<slug>/` file is reflected in `wiki/<slug>.md`.** A new raw file means a
  wiki update. No wiki claim should lack raw support. `/process-data` sweeps every
  `_data/` folder and writes the wiki notes missing for existing raw sources.
- **Paraphrase in the source's voice in `wiki/`;** reserve quotation marks for phrases
  actually attested in `raw/`.
- **One canonical file per subject.** If a domain has a single reference doc that is the
  source of truth for some set of facts, update it in place when facts change; don't
  scatter the same facts across other files.
- **Every `wiki/` note carries `atomic-summary`** (`true`/`false`), defaulting `false` on
  creation. `/process-data` never flips it beyond that initial `false` — `/process-atomic`
  is what sweeps a wiki note for atomic-note-worthy ideas and sets it `true` once done.

## Template

`raw/` still has no per-note template — files keep the source's own shape plus
frontmatter. `wiki/<slug>.md` now follows [[TSchemaWiki]].

## Deviations

A domain's `_data/` (or a subfolder of it) adds a local `__README.md` starting
`Extends _schema/_data.md. Differences:` when it needs domain-specific rules — e.g. a
per-source corpus convention.
