---
name: process
description: >-
  When the user wants a full vault housekeeping pass. Runs every _core/_skills/process-*
  skill in turn and reports one combined result. Use for "process the vault", "/process",
  "run all the housekeeping skills", "do a full sweep".
---

# Process

Run every schema-processing skill in one pass. This is the orchestrator, not a
sixth check of its own — it has no rules beyond "run the others and combine their
output."

## Purpose

Each `_core/_skills/process-<schema>.md` skill enforces one schema's mechanical
invariants (`_data`, `_tasks`, `_code`, `_atomic`, `_skills`, and any added later).
Running them one at a time is fine, but a single "do all of it" command is worth having.
This skill is that command — it discovers the others, so adding a new `process-*` skill
automatically extends `/process`; nothing here needs updating when that happens.

## Inputs to read first

Every `_core/_skills/process-*.md` file **except this one** (`process.md`) — currently:

- [`process-data.md`](process-data.md)
- [`process-tasks.md`](process-tasks.md)
- [`process-code.md`](process-code.md)
- [`process-atomic.md`](process-atomic.md)
- [`process-skills.md`](process-skills.md)

Discover the list by globbing `_core/_skills/process-*.md` rather than hard-coding it —
the list above is current as of this file's writing, not a fixed set.

**Fixed dependency: `process-data` before `process-atomic`.** `process-atomic`'s `_data/`
sweep pass reads `_data/wiki/` notes and their `atomic-summary` field, which only exists
(or is current) once `process-data` has run. This isn't incidental list order — if
discovery ever changes (alphabetical, parallel, etc.), `process-data` must still be
sequenced before `process-atomic` in the run.

## Arguments

- None. If the user names one schema ("just process data"), run that one skill directly
  instead of the full sweep — this skill is for "all of them."

## Steps

1. Run each discovered skill's own **Steps** in full, in the order listed above.
2. Collect each one's **Output format** sections, but skip each sub-skill's own **Log**
   step — this skill writes one combined log at the end instead of one per sub-skill.
3. If one sub-skill's steps fail or find nothing to do, note that and continue to the
   next — one skill having nothing to do doesn't stop the sweep.

## Output format

One section per sub-skill, in the order run, each headed with the skill's name (e.g.
`### process-data`) and containing that skill's own output sections verbatim (minus its
Log step). Close with:

### Summary
One line per sub-skill: what it found / changed, or "nothing to do."

### Log
Write **one** `_core/_logs/<YYYY-MM-DD-HHMM>.md` note (scope `vault`) per
`_core/_templates/TSchemaLogs.md` covering the whole run — `## Changed` lists every file every sub-skill
touched, grouped by which skill touched it. Add its line to `_core/_logs/__INDEX.md`.
Confirm the path.

## Style

Terminal markdown, no preamble. This skill makes no decisions of its own — every rule
about what's safe to change lives in the sub-skill being run.
