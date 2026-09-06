# _schema/inbox.md

An `inbox/` is the **unprocessed drop zone** for its section. Raw material lands here
before it's read, sorted, and moved to where it belongs.

## Rules

- **Everything in `inbox/` is unprocessed.** Presence in `inbox/` means "not yet dealt
  with." An empty `inbox/` is the goal state.
- **Processing an `inbox/` only writes inside the same section.** Move each item to the
  correct place — `knowledge/`, `decisions/`, the section's lead doc, and so on. Don't
  reach into another section or the vault root while clearing an inbox.
- **The normal section rules still apply** wherever an item lands — verbatim rules for
  `knowledge/raw/`, dated-note format for `decisions/`, and so on. The inbox is a staging
  area, not an exemption.
- **Remove an item from `inbox/` once it's routed** (or record why it was discarded). The
  inbox is not an archive.

## File naming

No fixed convention — keep whatever name the source arrived with. If it needs a note
about where it came from, add a short `NN-source.md` sidecar or frontmatter rather than
renaming.

## Deviations

A section whose inbox is filled automatically, or needs a different retention rule, adds
`inbox/README.md` starting `Extends _schema/inbox.md. Differences:` and lists only the
changes.
