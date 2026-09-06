# _schema/skills.md

A `skills/` folder holds **reusable procedures** a section defines — the "programs" it can
run on demand.

## The two locations

| Location | Role |
|---|---|
| `<section>/skills/<name>.md` | Where the skill is **authored and kept** — portable, tool-agnostic, part of the folder you'd copy anywhere. |
| `.claude/skills/<name>/SKILL.md` | Where Claude Code **loads it from**. Only skills here are invocable as `/<name>`. |

- **Section skills** keep both copies in sync. If you edit one, mirror the other and note
  it in the section's log (or its lead doc if it has no `logs/`).
- **Vault-wide skills** — ones that operate on the whole vault rather than a single
  section — keep **only** the `.claude/skills/` copy; it's canonical, with no authored
  mirror.
- New skills start from [`skill-template.md`](skill-template.md).

## Conventions

- `name` in frontmatter is kebab-case and matches the filename / folder.
- `description` leads with the trigger condition — it's what Claude matches on.
- A skill reads from the section's `knowledge/` and lead doc; it shouldn't hard-code
  facts that belong in a reference file.

## Deviations

Add `skills/README.md` starting `Extends _schema/skills.md. Differences:` if a section
needs a different authoring or sync convention.
