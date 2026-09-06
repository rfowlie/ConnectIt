# Skill Title

One line on the job this skill does. If it produces a deliverable, say what shape it is.

## Purpose

2–4 sentences. What outcome the skill exists to produce, and the standard it's held to
(e.g. "a decision, not a survey"; "grounded in the real source, not invented").

## Inputs to read first

List the files/sources this skill must load before doing anything, in order, and why:

1. `path/to/lead-doc.md` — what this section is and its current state.
2. `path/to/context.md` — the reference context every answer must fit.
3. `knowledge/wiki/<slug>.md` — synthesised source profiles, with a fallback if one is
   missing.

State what to do when an expected input is absent (say so; don't invent it).

## Arguments

- What the user passes after `/skill-name`.
- What to do if they pass nothing (ask one specific question with a sensible default, then
  wait).
- Restate the resolved request in one line at the top of the output.

## Output format

Number the sections the skill should produce. For each: what it contains, how long, and
any hard rules (e.g. "first person", "≤ 200 words", "end with a one-line bottom line").
End with a synthesis/decision section that actually commits to an answer rather than
listing options.

## Style

- Terminal markdown. No preamble ("Great question…") — start with section 1.
- Don't pad; a thin section is fine if there's little to say.
- Never fabricate quotes or sources; flag low-confidence areas.
- Close by noting what was thin or omitted, and offer the obvious next step.

---

## How to make this skill runnable

Authored skills live in `<section>/skills/` as portable `.md` files. To make one
invocable as `/skill-name` inside Claude Code, copy its body to:

```
.claude/skills/<skill-name>/SKILL.md
```

Claude Code only discovers skills under `.claude/skills/`. Keep the two in sync. A
vault-wide skill keeps only the `.claude/skills/` copy — see [`skills.md`](skills.md).
