# _core/_attachments/

The **vault-level** store for non-note files — images, PDFs, audio, video, binaries —
that aren't scoped to a single domain.

Governed by [`_schema/_attachments.md`](../_schema/_attachments.md). A domain's own files
live in `<domain>/attachments/`.

**Immutable.** Like `data/raw/`, nothing here is edited, renamed, or deleted by Claude —
only by the user. Notes link in; they don't rewrite what's here.
