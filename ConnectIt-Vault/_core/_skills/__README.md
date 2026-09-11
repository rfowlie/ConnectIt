# _core/_skills/

Authored home for **vault-wide** skills — ones that operate on the whole vault rather
than a single domain. Each is mirrored to `.claude/skills/<name>/SKILL.md`, which is
where Claude Code loads it from.

Governed by [`_schema/_skills.md`](_skills.md). A domain's own skills live
in `<domain>/skills/`, not here. `drafts/` holds unconverted raw material — not live
skills.

Live skills:

- **[`ask-vault.md`](ask-vault.md)** — answer a question using only the vault's own
  content notes; report gaps when the material isn't there. Logs each Q&A to
  `_core/_ask-vault/`.

**`process-*` family** — one mechanical health-check-and-fix per schema, named
`/process-<schema>` for consistency. [`process.md`](process.md) runs all of them in one
pass.

- **[`process-data.md`](process-data.md)** — sweep every `data/` folder; write the
  `wiki/` note for any `raw/` source that doesn't have one yet.
- **[`process-tasks.md`](process-tasks.md)** — sweep every `tasks/` folder; move rows
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
