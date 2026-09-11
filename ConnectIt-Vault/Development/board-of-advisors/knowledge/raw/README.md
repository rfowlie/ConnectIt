# board-of-advisors/knowledge/raw/

Extends [`../../../_schema/knowledge.md`](../../../_schema/knowledge.md). Differences:

- **Corpora are per advisor.** One subfolder per source person by kebab-case slug:
  `raw/<person-slug>/NN-title.md`. Current: `tim-cain/`, `jonas-tyroller/`, `tim-ruswick/`.
- **Frontmatter fields:** `person`, `slug`, `title`, `type`, `channel`/source, `ingested`,
  and a `note:` for caveats (garbled auto-captions, a guest speaker, a conversation the
  person *hosted* rather than authored).
- **Flag hosted/guest content** in the `note:` so that person's `wiki/` synthesis doesn't
  absorb someone else's opinions.
- Number files `01-`, `02-` in ingest order.
- Every raw file must be reflected in that person's synthesis at
  [`../wiki/<person-slug>.md`](../wiki/).
