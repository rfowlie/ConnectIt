# _core/_logs/

The **vault-wide** maintenance log — automated passes that touch the whole vault or cross
domains. One timestamped note per run, newest first.

Governed by [`_schema/_logs.md`](_logs.md). A single domain's own passes go
in `<domain>/logs/`, not here.

Newest first:

- [[2026-09-11-0112]] — session — `/process` full sweep: 0 drift across 35 code pages,
  bootstrapped `.claude/skills/` (7 mirrors, none existed before)
- [[2026-09-10-0900]] — session — `_core/` migration: retired `_section.md` + `high-level.md`, per-domain `CLAUDE.md` routers, root `CLAUDE.md` → domain map, `ABOUT.md` created, `devlog/`+`extras/` removed, logs conformed + split to `ConnectIt/logs/`, skills mirrored to `.claude/`
- [[2026-09-10-1000]] — session — folded `architecture/Overview.md` into `ConnectIt/code/index.md` Map; `architecture/` removed
- [[2026-09-10-1100]] — session — full re-ingest of every domain's `code/` to the `_code` schema (nested `systems/`+`recipes/`, per-page `## Changes`, index inventory+Map, re-anchored `commit:`)
- [[2026-09-10-1200]] — session — extracted `old/` into vault notes (`ConnectIt/decisions/`, known-issues, per-plugin gotchas); `old/` removed
- [[2026-09-09-10]] — session — renamed `_extras/` → `_core/` across the vault
- [[2026-09-09-9]] — session — added `_core/CLAUDE.md` (module router); root `CLAUDE.md` → thin stub + `@import`
- [[2026-09-09-8]] — session — schema-model revision: `_code` consolidation, drop `_frameworks`/`_systems`/`_recipes`/`_updates`, add `_tasks`/`_questions`/`_attachments`, `_core/` reframe
- [[2026-09-09-7]] — session — log-on-change rule added to `CLAUDE.md`; this session backfilled
- [[2026-09-09-6]] — session — `/ask-vault` skill + `_core/_ask-vault/` folder
- [[2026-09-09-5]] — user — `_schema/` moved into `_core/`; doc reconciliation pending
- [[2026-09-09-4]] — session — `_core/` machinery drawer + `_logs` flatten + `_inbox`→`_clippings` + skills model
- [[2026-09-09-3]] — session — `extras/` → `_core/` path sync
- [[2026-09-09-2]] — session — `_atomic` section added
- [[2026-09-09]] — session — stray notes in `_schema/` reconciled; `_code`/`_systems`/`_recipes`/`_updates` promoted
