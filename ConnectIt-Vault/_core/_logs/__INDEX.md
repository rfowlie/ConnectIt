# _core/_logs/

The **vault-wide** maintenance log — automated passes that touch the whole vault or cross
domains. One timestamped note per run, newest first.

Governed by [`_schema/_logs.md`](_logs.md). A single domain's own passes go
in `<domain>/_logs/`, not here.

Newest first:

- [2026-09-18-0154](2026-09-18-0154.md) — process — full sweep: `process-code` refreshed
  21 drifted pages (4+ days of undocumented Board Shift/SWAP/action-refactor drift across
  ConnectIt + 3 plugins); `process-data`/`process-tasks`/`process-atomic`/`process-skills`
  all clean, nothing to do. Flagged a concurrent `BlueprintImplementableEvent` conflict on
  `ConnectIt_PieceRegistry.h` found mid-sweep.
- [2026-09-18-0048](2026-09-18-0048.md) — process-tasks — filed 2 `complete` rows from
  `ConnectIt/_tasks/active.md` to `complete.md` (`DefaultLevelConfig`, Actions UI HUD
  wiring); other two `_tasks/` folders had nothing to move
- [2026-09-18-0038](2026-09-18-0038.md) — session — filed the check-in's task candidates
  to `ConnectIt/_tasks/active.md` (2 complete, 3 updated/rewritten, 3 added) and
  `optimal-co-developer/_tasks/active.md` (networking outreach, its first row); updated
  `action-gameplay-tags.md`'s Current thinking
- [2026-09-18-0035](2026-09-18-0035.md) — meeting-closed — filed the 2026-09-18 check-in
  (`Development/optimal-co-developer/_meetings/`) + 4 `ConnectIt/_decisions/` notes:
  registry mapping refresh, LoadOut reference on PlayerState, the generic turn-end
  requirement system, visual-reactions-in-Blueprint convention
- [2026-09-17-1511](2026-09-17-1511.md) — session — retired `improve-system` (never once
  run — confirmed via `_logs/`, no entry anywhere has `source: improve-system`); cleaned
  up every reference (`_core/_skills/__INDEX.md`, `Development/CLAUDE.md`'s routing table
  and log-row, `Development/board-of-advisors/CLAUDE.md`'s `_logs/` row), `ABOUT.md`
  updated
- [2026-09-17-1502](2026-09-17-1502.md) — session — `/session-close` extended with a
  "route decisions, questions, and discussions" step (decisions written directly,
  questions/discussions offered as candidates) — closes the gap where neither
  `/session-close` nor `/improve-system` actually populated a game domain's `_decisions/`/
  `_questions/`/`_discussions/` on their own

- [2026-09-16-1610](2026-09-16-1610.md) — session — wired `_data/` → `_atomic/`
  extraction: `process-atomic` gained a Pass 2 that sweeps un-swept `_data/wiki/` notes
  into `_core/_atomic/`; new `TSchemaWiki` template with `atomic-summary` tracking;
  `process-data`-before-`process-atomic` made an explicit dependency; atomic word cap
  500 → 1000
- [2026-09-16-1609](2026-09-16-1609.md) — session — `/session-open`'s map output rewritten
  to bullet-list-per-block (owner preference) — 5 blocks + the session-note template +
  `.claude/skills/` mirror updated; `/session-close` intentionally left untouched pending
  the owner confirming whether the same preference extends there
- [2026-09-16-1557](2026-09-16-1557.md) — session — removed 6 stale repo-root
  `.claude/skills/` mirrors (staged, uncommitted); added a permanent guardrail to
  `ConnectIt/CLAUDE.md` against ever re-adding a "Current state" section; rewrote
  `session-open`/`session-close` (+ mirrors) to source state from `_tasks/active.md` +
  `_meetings/` + `_decisions/` instead of `CLAUDE.md` fields
- [2026-09-14-0204](2026-09-14-0204.md) — meeting-closed — closed the first
  `Development/optimal-co-developer/_meetings/` note (2026-09-14 check-in); filed 3
  `ConnectIt/_decisions/` notes (action-state-on-PlayerState/ActionsComponent-as-builder,
  Actions-UI-owned-by-HUD, turn-end AND/OR requirement groups); task candidates listed in
  the meeting note, not filed to `_tasks/active.md`
- [2026-09-14-0055](2026-09-14-0055.md) — process — full sweep: 1 task row filed, 7 of 35
  `current` code pages refreshed (`ConnectIt` + `UnrealTurnBasedMechanics`, both
  SWAP-touched since last reconciliation — the other 5 domains clean), atomic/skills/data
  all clean; flagged a missing `ITurnBasedActionNotifier` inventory row + stale
  `ConnectIt/code/__INDEX.md` framing
- [2026-09-13-2339](2026-09-13-2339.md) — session — authored the `/meeting-open` /
  `/meeting-closed` check-in skill pair in `Development/optimal-co-developer/_skills/` +
  mirrors, extended `_meetings/__README.md` for the transcript+summary shape and
  plain-string attendees
- [2026-09-12-1809](2026-09-12-1809.md) — session — resolved the repo-root
  `.claude/skills/` open question: retired `reconcile-tasks` (→ `process-tasks`), deleted
  stale `ask-the-board`/`ask-vault` root copies, re-authored + mirrored `improve-system`
  (now runs `/process` first) and `session-open`/`session-close` (new `Development/_skills/`
  + new `_sessions` schema), removed the "vault-wide skill needs no mirror" policy
- [2026-09-11-1500-2](2026-09-11-1500-2.md) — session — converted the remaining 9
  authored `[[TSchemaLogs]]` wikilinks (7 skills + 2 schema files) to the Markdown-link
  standard and re-mirrored the 7 skills; caught and fixed a filename collision with the
  pre-existing `2026-09-11-1500.md` along the way (see that note — restored, untouched)
- [2026-09-11-2340](2026-09-11-2340.md) — session — `_discussions` added as a full schema
  (topic-threaded, no subfolders) + `TSchemaDiscussions` template + new
  `/process-discussion` skill to file transcripts into it
- [2026-09-11-1500](2026-09-11-1500.md) — session — `_`-prefix naming rule reversed
  vault-wide (every schema-governed section folder, not just `_core/`); 5 folders
  `git mv`'d, 12 schema rulebooks + every cross-reference fixed, 34 `ConnectIt/`
  wikilinks retargeted; `optimal-co-developer` schema folders built out + new
  `/ask-partner` skill; `Development/CLAUDE.md` "Default posture" section added;
  `/process-skills` extended for domain skills and run (2 new mirrors, 5 refreshed)
- [[2026-09-11-1430]] — session — link-format standard set (Markdown links, not
  wikilinks); 9 `CLAUDE.md` files converted; two `_core/_atomic/` notes on the reasoning;
  new `fix-links` skill
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
