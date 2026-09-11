---
name: process-tasks
description: >-
  When the user has changed task rows in a tasks/active.md to Status suspended / complete
  / dropped and wants them filed. Vault-wide: finds every tasks/ folder, relocates each
  retired row to its archive file with a Moved date, and reports what moved. Use for
  "process tasks", "/process-tasks", "file my done tasks", "tidy the task lists".
---

# Process Tasks

Keep every `tasks/active.md` showing only active work by moving retired rows to their
archive files. The deliverable is a **precise relocation** — nothing is rephrased,
reprioritised, or deleted.

**One-directional by design.** This skill only moves rows *out of* `active.md`. Reviving
a `suspended` task is deliberately out of scope here — it's planned as part of a future
sprint-planning workflow skill (surface suspended tasks as available to resume; see
`ABOUT.md` → Open questions). Move a task back by hand until that exists.

## Purpose

The user tracks work in `tasks/` status tables and marks a row done by editing its
`Status` cell. This skill is the mechanical step that acts on that edit: cut the row from
`active.md`, append it to `suspended.md` / `complete.md` / `dropped.md`, stamp a `Moved`
date. It never decides what a task's status *should* be.

## Inputs to read first

1. Every `tasks/active.md` in the vault — `_core/_tasks/active.md` and each
   `<domain>/tasks/active.md`. Use a glob for `**/tasks/active.md`.
2. For each, its sibling archive files (`suspended.md`, `complete.md`, `dropped.md`) —
   read the header row so appended rows match the columns.

If a `tasks/` folder has no `active.md`, skip it. If an archive file is missing, create
it from the `active.md` header plus a trailing `Moved` column.

## Arguments

- None. If the user names one folder ("just the connect-it tasks"), limit to that folder.

## Steps

For each `active.md`:

1. Parse the markdown table. Identify the `Status` column.
2. For every row where `Status` is `suspended`, `complete`, or `dropped` (case-insensitive,
   trimmed): remove it from `active.md`, and append it to the matching archive file with
   a `Moved` cell set to today's date (`YYYY-MM-DD`).
3. Rows with `Status` `active` — leave untouched.
4. Rows with an unrecognised `Status` — leave in `active.md`, and list them under Flags.
5. Preserve every other cell verbatim. Preserve row order within each file (append to the
   end of the archive).

## Output format

### 1. Moved
A table per `tasks/` folder that changed: `Folder | Task | From → To`. If nothing moved,
say so.

### 2. Flags
Unrecognised statuses, malformed rows, archive files created, anything skipped.

### 3. Log
Write an `_core/_logs/<YYYY-MM-DD-HHMM>.md` note per `[[TSchemaLogs]]` (scope `vault`),
and add its line to `_core/_logs/__README.md`. Confirm the path.

## Style

Terminal markdown, no preamble. Do not touch task text, `Status` values, dates, or
priorities. Never delete a row — only relocate it.
