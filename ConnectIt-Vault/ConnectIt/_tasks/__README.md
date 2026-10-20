# ConnectIt/_tasks/

ConnectIt-domain work tracked as status tables — game/code tasks that have entered play,
separate from the vault-wide `_core/_tasks/` tracker.

Extends [`_schema/_tasks.md`](../../_core/_schema/_tasks.md). Differences:

- Adds a **`Priority`** column (`Bug` | `Design` | `Wishlist` | `Urgent`), placed after
  `Status`, before `Notes` — what kind of work a row is, not just whether it's open.
  `Urgent` jumps the queue regardless of the other three. Added 2026-09-24 at the owner's
  request; ConnectIt-only — `_core/_tasks/` and `Development/optimal-co-developer/_tasks/`
  don't carry it, since they track different kinds of work.

- Add rows to `active.md` by hand.
- To retire a task: set its `Status` to `suspended` / `complete` / `dropped`, then run
  `/process-tasks` — it moves the row to the matching file and stamps `Moved`.
