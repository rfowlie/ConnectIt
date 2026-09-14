# ConnectIt/_tasks/

ConnectIt-domain work tracked as status tables — game/code tasks that have entered play,
separate from the vault-wide `_core/_tasks/` tracker.

Governed by [`_schema/_tasks.md`](../../_core/_schema/_tasks.md).

- Add rows to `active.md` by hand.
- To retire a task: set its `Status` to `suspended` / `complete` / `dropped`, then run
  `/process-tasks` — it moves the row to the matching file and stamps `Moved`.
