# _core/_skills/

The live catalog of every vault-wide skill actually present. Rules for this folder are
in [`__README.md`](__README.md); governed by [`_schema/_skills.md`](../_schema/_skills.md).

Live skills:

- **[`ask-vault.md`](ask-vault.md)** — answer a question using only the vault's own
  content notes; report gaps when the material isn't there. Logs each Q&A to
  `_core/_ask-vault/`.
- **[`fix-links.md`](fix-links.md)** — vault-wide Markdown link health check: find every
  broken `[text](path)` link, propose a fix for it, apply only what the user confirms.
  Markdown links don't self-heal on a file move the way `[[wikilinks]]` do (see
  `_core/CLAUDE.md` → Link format), so this is the mechanical upkeep for that trade-off.
- **[`process-discussion.md`](process-discussion.md)** — despite the name, **not** part of
  the sweep family below: it's input-driven, not a folder sweep. Takes one transcript
  (pasted, or a file path) and files it as a `_discussions/` note per
  `_schema/_discussions.md` — summary + Tasks/Deliverables/Questions/Updates/Workflows up
  top, each topic linked back to its most recent prior mention, transcript verbatim at the
  bottom. Not run by `/process` (nothing to sweep without a transcript in hand).

**`process-*` family** — one mechanical health-check-and-fix per schema, named
`/process-<schema>` for consistency. [`process.md`](process.md) runs all of them in one
pass.

- **[`process-data.md`](process-data.md)** — sweep every `_data/` folder; write the
  `wiki/` note for any `raw/` source that doesn't have one yet.
- **[`process-tasks.md`](process-tasks.md)** — sweep every `_tasks/` folder; move rows
  marked suspended / complete / dropped out of `active.md` into their archive file.
  One-directional; reviving a task stays manual (see `ABOUT.md` → Open questions).
- **[`process-code.md`](process-code.md)** — check every `code/` page's `commit:`
  provenance against git history; re-read and rewrite anything that's drifted.
- **[`process-atomic.md`](process-atomic.md)** — find any `atomic/` / `_atomic/` folder
  that isn't `_core/_atomic/`, merge its notes in, delete the invalid folder.
- **[`process-skills.md`](process-skills.md)** — make sure every authored skill here has
  a matching, up-to-date `.claude/skills/` mirror in this vault.
- **[`process.md`](process.md)** — runs every `process-*` skill above and reports one
  combined result.
