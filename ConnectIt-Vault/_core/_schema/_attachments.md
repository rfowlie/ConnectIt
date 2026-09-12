# _schema/_attachments.md

An `_attachments/` folder is the dumping ground for **non-note files** — images, PDFs,
audio, video, spreadsheets, exports, any binary. Notes elsewhere link into it.

## Structure

```
_attachments/
  <file kept under its original name>
  <subfolders by source or kind are fine, at the user's discretion>
```

No fixed layout — it's storage, not a note collection.

## Rules

- **Immutable — treat it like `_data/raw/`.** Claude never edits, renames, converts,
  moves, compresses, or deletes anything in `_attachments/`. Only the user does.
- **Link, don't blind-embed.** Reference an attachment from a note; don't paste a large
  binary inline or transcribe it into a note without a reason.
- **Keep the source's own filename.** The user renames only for disambiguation.
- Not for text sources — a transcript or article goes to `_data/raw/`. `_attachments/` is
  the binary counterpart, held to the same "never altered" standard.
- Created on first use.

## Mounts

Either: `_core/_attachments/` for vault-level files, `<domain>/_attachments/` for a
domain's own.

## Template

No template — `_attachments/` defines a storage folder, not notes of a fixed shape.

## Deviations

Add `_attachments/__README.md` starting `Extends _schema/_attachments.md. Differences:` if a
domain needs a naming scheme or a retention rule.
