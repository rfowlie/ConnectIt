---
name: process-skills
description: >-
  When the user wants every authored skill actually mirrored so it's invocable. Checks
  every _core/_skills/*.md and every <domain>/_skills/*.md against its
  .claude/skills/<name>/SKILL.md mirror in this vault, creates any mirror that's missing,
  and refreshes one that's drifted. Use for "process skills", "/process-skills", "mirror
  my skills", "make sure skills are synced".
---

# Process Skills

Keep `.claude/skills/` an exact, invocable mirror of every authored skill in the vault —
vault-wide skills in `_core/_skills/` and domain skills in `<domain>/_skills/`. Per
`_schema/_skills.md`: the authored copy is canonical, the mirror is generated and never
hand-edited.

## Purpose

A skill only becomes a runnable `/<name>` once it exists at
`.claude/skills/<name>/SKILL.md`. This skill is the check-and-fix step: find every
authored skill — vault-wide or domain — missing its mirror (or drifted from it) and
correct it, scoped to this vault only — no cross-vault sync.

## Inputs to read first

1. Every `_core/_skills/*.md` file, **excluding** `__README.md`, `__INDEX.md`, and
   everything under `drafts/` (unconverted, not live skills).
2. Each file's frontmatter `name:` — that's the mirror's folder name.
3. `.claude/skills/<name>/SKILL.md` for each, if it exists, to compare content.
4. Every `<domain>/_skills/*.md` file vault-wide — glob `**/_skills/*.md` and exclude any
   `_core/_skills/` hits (already covered by item 1) — with the same exclusions applied
   per domain: `__README.md`, `__INDEX.md`, and everything under a `drafts/` subfolder.

## Arguments

- None. Always checks every live authored skill, vault-wide and domain.

## Steps

### Pass 1 — vault-wide skills

For each authored skill (`_core/_skills/<name>.md`):

1. If `.claude/skills/<name>/SKILL.md` doesn't exist: create it (and the folder), with
   the authored file's content verbatim.
2. If it exists and its content differs from the authored copy: overwrite it to match —
   the mirror is never the source of truth.
3. If it exists and already matches: leave it, and count it as in sync.

### Pass 2 — domain skills

For each authored skill found at `<domain>/_skills/<name>.md`:

1. Same create / overwrite / already-in-sync logic as Pass 1, against
   `.claude/skills/<name>/SKILL.md`.
2. **Name collisions are flagged, never silently resolved.** The mirror namespace
   (`.claude/skills/<name>/`) is flat and shared across every vault-wide and domain skill.
   If a domain skill's `name:` collides with a vault-wide skill's, another domain's skill,
   or an existing mirror whose authored source can't be identified, stop for that name:
   list it under Flags instead of guessing which authored copy is the real one.

Never edit an authored `_core/_skills/*.md` or `<domain>/_skills/*.md` file from this
skill — drift only ever gets fixed by copying authored → mirror, not the other way round.

## Output format

### 1. Mirrored
A table: `skill | scope (vault-wide / domain) | action` (`created` / `refreshed` /
`already in sync`).

### 2. Flags
Any authored skill file with no `name:` frontmatter (can't be mirrored until fixed), a
`.claude/skills/` entry with no matching authored file (an orphan mirror — flag, don't
delete without being asked), or a `name:` collision found in Pass 2.

### 3. Log
- Vault-wide changes (Pass 1): write an `_core/_logs/<YYYY-MM-DD-HHMM>.md` note per
  `_core/_templates/TSchemaLogs.md` (scope `vault`), and add its line to
  `_core/_logs/__INDEX.md`.
- Domain-skill changes (Pass 2): additionally group by domain and write each domain's own
  `<domain>/_logs/<YYYY-MM-DD-HHMM>.md` note (scope `<domain>`) — create that domain's
  `_logs/` on first use, like any other domain section — and add its line to
  `<domain>/_logs/__INDEX.md`.

Confirm every path written.

## Style

Terminal markdown, no preamble. Authored files are read-only from this skill's
perspective; only `.claude/skills/` is written.
