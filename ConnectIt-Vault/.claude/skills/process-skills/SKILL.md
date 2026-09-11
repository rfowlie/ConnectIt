---
name: process-skills
description: >-
  When the user wants every authored skill actually mirrored so it's invocable. Checks
  every _core/_skills/*.md against its .claude/skills/<name>/SKILL.md mirror in this
  vault, creates any mirror that's missing, and refreshes one that's drifted. Use for
  "process skills", "/process-skills", "mirror my skills", "make sure skills are synced".
---

# Process Skills

Keep `.claude/skills/` an exact, invocable mirror of every authored skill in
`_core/_skills/`, in **this vault**. Per `_schema/_skills.md`: the authored copy is
canonical, the mirror is generated and never hand-edited.

## Purpose

A skill only becomes a runnable `/<name>` once it exists at
`.claude/skills/<name>/SKILL.md`. This skill is the check-and-fix step: find every
authored skill missing its mirror (or drifted from it) and correct it, scoped to this
vault only — no cross-vault sync.

## Inputs to read first

1. Every `_core/_skills/*.md` file, **excluding** `__README.md`, `__INDEX.md`, and
   everything under `drafts/` (unconverted, not live skills).
2. Each file's frontmatter `name:` — that's the mirror's folder name.
3. `.claude/skills/<name>/SKILL.md` for each, if it exists, to compare content.

## Arguments

- None. Always checks every live authored skill.

## Steps

For each authored skill (`_core/_skills/<name>.md`):

1. If `.claude/skills/<name>/SKILL.md` doesn't exist: create it (and the folder), with
   the authored file's content verbatim.
2. If it exists and its content differs from the authored copy: overwrite it to match —
   the mirror is never the source of truth.
3. If it exists and already matches: leave it, and count it as in sync.

Never edit an authored `_core/_skills/*.md` file from this skill — drift only ever gets
fixed by copying authored → mirror, not the other way round.

## Output format

### 1. Mirrored
A table: `skill | action` (`created` / `refreshed` / `already in sync`).

### 2. Flags
Any `_core/_skills/*.md` with no `name:` frontmatter (can't be mirrored until fixed), or
a `.claude/skills/` entry with no matching authored file (an orphan mirror — flag, don't
delete without being asked).

### 3. Log
Write an `_core/_logs/<YYYY-MM-DD-HHMM>.md` note per `[[TSchemaLogs]]` (scope `vault`),
and add its line to `_core/_logs/__INDEX.md`. Confirm the path.

## Style

Terminal markdown, no preamble. Authored files are read-only from this skill's
perspective; only `.claude/skills/` is written.
