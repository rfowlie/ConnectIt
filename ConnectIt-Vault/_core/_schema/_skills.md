# _schema/_skills.md

Reusable procedures Claude can run on demand — the "programs" of this vault.

## The locations

| Location | Role |
|---|---|
| `<domain>/skills/<name>.md` | Authored home for a **domain** skill — portable, tool-agnostic, part of the folder you would copy anywhere. |
| `_core/_skills/<name>.md` | Authored home for a **vault-wide** skill — one that operates on the whole vault rather than a single domain. |
| `.claude/skills/<name>/SKILL.md` | Where Claude Code **loads** any skill from. Only skills here are invocable as `/<name>`. |

- Every skill keeps its **authored copy** and the **`.claude/skills/` mirror** in sync:
  edit the authored copy, mirror it, and note the change in the relevant log —
  `<domain>/logs/` for a domain skill, `_core/_logs/` for a vault-wide one.
- The authored copy is canonical; `.claude/skills/` is a generated mirror, not
  hand-edited. `/process-skills` checks every vault-wide skill's mirror exists and
  matches, and fixes it if not — scoped to this vault, not other vaults holding a copy.

## Template

[[TSchemaSkills]]

## Standards

Every skill in this vault is held to the same bar:

- **Synthesis, not a survey.** End with a section that commits to an answer, not a list
  of options.
- **No preamble.** Start with the first real section.
- **Never fabricate** quotes or sources; flag low-confidence areas.
- **Missing input is stated, not invented.** If an expected file is absent, say so.
- **Close** by noting what was thin or omitted, and offer the obvious next step.

## Conventions

- `name` in frontmatter is kebab-case and matches the filename / folder.
- `description` leads with the trigger condition — it is what Claude matches on.
- A skill reads from `data/wiki/` and the domain's reference docs; it shouldn't
  hard-code facts that belong in a reference file.

## Deviations

Add `skills/__README.md` starting `Extends _schema/_skills.md. Differences:` if a domain
authors skills differently.
