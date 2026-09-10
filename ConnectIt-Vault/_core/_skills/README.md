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
- **[`reconcile-tasks.md`](reconcile-tasks.md)** — sweep every `tasks/` folder; move rows
  marked suspended / complete / dropped out of `active.md` into their archive file.
