# ConnectIt/_discussions/

One dated note per discussion (calls, brainstorms, transcribed voice memos), organized by
topic-threading rather than subfolders — each note links every topic it covers back to the
most recent prior note that also covered it. Governed by
[[_core/_schema/_discussions|_core/_schema/_discussions.md]].

Newest first:

- [2026-09-14-swap-implementation-qa](2026-09-14-swap-implementation-qa.md) — five
  implementation Q&A exchanges from SWAP work: BP UI local-controller gating, a
  local-player-index-vs-faction-slot bug in `GetLocalConnectItPlayerState`, activating an
  action by tag instead of constructing a new instance from its class, and removing/
  renaming the `SwapUsesRemaining` tampering surface. 4 topics, 1 continuing
  `action-state-architecture`, 3 first mentions.
- [[ConnectIt/_discussions/2026-09-12-action-state-architecture|2026-09-12-action-state-architecture]]
  — where per-action state (uses remaining, cooldowns, availability) should live; surfaces
  the `PlayerState`-can't-see-live-state circularity. 4 topics, 1 continuing
  `board-mediator-and-action-config-design`, 3 first mentions.
- [[ConnectIt/_discussions/2026-09-12-board-mediator-and-action-config-design|2026-09-12-board-mediator-and-action-config-design]]
  — whether the Board Request Mediator folds into GameMode or stays a swappable UObject,
  and whether `BoardRules` dissolves into independent rule interfaces. 7 topics, 2
  continuing `swap-action-ui-design`, 5 first mentions.
- [[ConnectIt/_discussions/2026-09-11-swap-action-ui-design|2026-09-11-swap-action-ui-design]]
  — the SWAP action's Actions UI: HUD-owned widget fed by a GameMode-side actions mediator,
  interface+delegate pattern keeping PlayerController decoupled. First `_discussions/` note,
  5 topics, all first mentions.

## Related

- [[ConnectIt/code/__INDEX|code/__INDEX.md]] — inventory, suite map, known issues
- [[ConnectIt/_logs/__INDEX|_logs/]] — ConnectIt-scoped maintenance passes, including each
  `/process-discussion` run that filed a note above
