# _schema/_tasks.md

A `tasks/` folder tracks the open and closed work that has entered a domain (or the
vault). It is a set of **status tables**, not one note per task — every task is a row.

## Structure

```
tasks/
  active.md       the working list — tasks currently in play
  suspended.md    parked tasks, kept for when they resume
  complete.md     finished tasks (archive)
  dropped.md      abandoned tasks (created only if used)
```

Each file is a single markdown table with the same columns.

## What a row holds

| Column | Meaning |
|---|---|
| `Task` | One line — what needs doing. |
| `Created` | `YYYY-MM-DD` the task entered the folder. |
| `Target` | `YYYY-MM-DD` due / aim date, or `—`. |
| `Status` | `active` \| `suspended` \| `complete` \| `dropped`. |
| `Notes` | Context, blockers, `[[links]]`. |

Archive files (`suspended` / `complete` / `dropped`) add a `Moved` column — the date the
row was relocated. A domain may add columns (`Priority`, `Source`, …); keep `Task` /
`Created` / `Status` as the first three.

## Rules

- **The user enters and prioritises tasks by hand.** Claude does not invent tasks or
  reorder them.
- **To retire a task:** set its `Status` cell in `active.md` to `suspended` / `complete`
  / `dropped`, then run `/reconcile-tasks` — the skill moves the row to the matching
  archive file and stamps `Moved`. Nothing leaves `active.md` until the skill runs.
- `active.md` should contain only `Status: active` rows after a reconcile.
- Archive files are append-only in spirit — a resumed task is moved back to `active.md`
  by hand (or a future skill), not rewritten in place.
- Created on first use.

## Mounts

Either: `_core/_tasks/` for vault-level work, `<domain>/tasks/` for a domain's own.
`/reconcile-tasks` sweeps every `tasks/` folder in the vault.

## Template

[[TSchemaTasks]]

## Deviations

Add `tasks/README.md` starting `Extends _schema/_tasks.md. Differences:` for extra
columns, extra statuses, or (later) an Obsidian Bases view in place of the markdown
tables.
