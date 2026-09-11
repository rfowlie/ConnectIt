# _schema/_clippings.md

A `clippings/` folder is an **unprocessed capture dump**. The Obsidian Web Clipper is the
main feeder — saved articles, videos, and pages land here before they're read, sorted,
and moved to where they belong. Other quick captures (a pasted note, an emailed link)
belong here too.

## Where it mounts

- **Vault-wide `_core/_clippings/`** — the catch-all. Capture now, decide later.
  Processing it **triages each item into the right domain** (and the right section within
  that domain).
- **Domain `clippings/`** — scoped to one domain. Processing it routes items **within
  that domain only**; never reach into another domain or the vault root.

## Rules

- **Everything in `clippings/` is unprocessed.** Presence means "not yet dealt with." An
  empty `clippings/` is the goal state.
- Nothing in `clippings/` feeds any process while it sits there.
- **Processing a clip = moving it to its real home.** Source material goes to the target
  `data/raw/` (verbatim, with its origin metadata — URL, author, capture date — intact);
  a decision goes to `decisions/`, and so on. Normal section rules apply wherever an item
  lands; `clippings/` is a staging area, not an exemption.
- **Delete an item once it's routed** (or record why it was discarded). `clippings/` is
  not an archive.
- **No `/process-*` skill touches this folder.** Deciding where a clip belongs is a
  judgment call every time — routing stays manual, on purpose.

## File naming

No fixed convention — keep whatever name the clipper or source produced. If it needs a
note about where it came from beyond its frontmatter, add a short `NN-source.md` sidecar
rather than renaming.

## Template

No note template — a clippings dump holds arbitrary captured material, not notes of a
fixed shape.

## Deviations

A domain whose `clippings/` is filled by a different tool, or needs a different retention
rule, adds `clippings/__README.md` starting `Extends _schema/_clippings.md. Differences:`
and lists only the changes.
