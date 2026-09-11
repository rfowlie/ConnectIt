# _core/_tasks/

The **vault-level** task tracker — work that isn't scoped to a single domain. Status
tables, one row per task.

Governed by [`_schema/_tasks.md`](../_schema/_tasks.md). A domain's own tasks live in
`<domain>/tasks/`, not here.

- Add rows to `active.md` by hand.
- To retire a task: set its `Status` to `suspended` / `complete` / `dropped`, then run
  `/process-tasks` — it moves the row to the matching file and stamps `Moved`.
