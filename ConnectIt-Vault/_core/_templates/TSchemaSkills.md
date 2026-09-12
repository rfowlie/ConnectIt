---
name: <kebab-case-name>
description: <trigger condition first — what Claude matches on — then what it produces>
---

# <Skill Title>

One line on the job this skill does. If it produces a deliverable, name its shape.

## Purpose

2–4 sentences: the outcome this skill exists to produce, and the standard it is held to
(e.g. "a decision, not a survey"; "in the person's real voice, not a caricature").

## Inputs to read first

The files this skill loads before doing anything, in order, and why:

1. `path/to/file.md` — what it provides.
2. `<domain>/user/<file>.md` — the personal context every answer must fit.
3. `<domain>/_data/wiki/<slug>.md` — voice / framework profile; note the fallback if one is missing.

If an expected input is absent: say so, do not invent it.

## Arguments

- What the user passes after `/<name>`.
- If they pass nothing: ask one specific question with a sensible default, then wait.
- Restate the resolved request in one line at the top of the output.

## Output format

Numbered sections the skill produces. For each: what it contains, how long, and any hard
rules (e.g. "first person", "≤ 200 words", "end with a one-line bottom line"). End with a
synthesis section that commits to an answer rather than listing options.

## Style

Terminal markdown. No preamble — start with the first real section. Don't pad; a thin
section is fine. Never fabricate quotes or sources; flag low-confidence areas. Close by
noting what was thin or omitted, and offer the obvious next step.
