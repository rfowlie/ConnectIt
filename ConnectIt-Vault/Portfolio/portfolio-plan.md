---
created: 2026-09-30
tags:
  - plan
  - portfolio
---
# ConnectIt → Portfolio: what to show, how to advertise it, what to highlight

> Lives in the [`Portfolio`](CLAUDE.md) domain. Task tracking: [`_tasks/active.md`](_tasks/active.md).
> Paths below are relative to the repo root (`ConnectIt/`) unless noted.

## Context

ConnectIt is a large, well-architected solo UE5 C++ project (110 commits since 2025-07, ~8 first-party
plugins, ~42 dated architecture decision notes, replicated turn-based multiplayer, async MinMax AI).
The portfolio shows none of it. The goal is a **technical artist → gameplay/systems programmer** pivot
(games only), job hunting now. The bottleneck is **presentation, not building** — so this plan adds no
new systems. Everything is about packaging, evidence, and distribution.

Constraints from the user (this session + memory):
- Existing portfolio site; **add a ConnectIt page** (not a rebuild).
- Can record gameplay footage now; no packaged build yet.
- **Curated excerpts only** — the main repo is not the public showcase.
- No ex-employer specifics in public content (say "shipped titles as a technical artist").
- Career notes flag: isolation, perfectionism spirals, motivation cliffs → keep steps small, time-boxed,
  each with one visible "next action." Sharing work doubles as low-pressure networking.

## Pre-flight findings (survey of the repo, 2026-09-30)

- **Repo has a GitHub remote (`rfowlie/ConnectIt`) and contains private material.** `ConnectIt-Vault/Development/`
  holds `career-coach.md` / `user.md` ("unemployed, let go", motivation/psychology notes) — and
  `career-coach.md` names the ex-employer and franchise, which violates the public-content constraint if the
  repo is or becomes public. **Verify repo visibility first.**
- README is near-empty boilerplate and overclaims ("multiple AI strategies" — AI plugin is dormant).
- No screenshots/GIFs/video, no tests, no license, stray files (`FIXES.txt`, `steam_appid.txt.txt`,
  `.sln.DotSettings.user`, tracked `workspace.json`), unclear-license Content packs.
- Unfinished/dormant: UnrealAIMechanics (dormant), UnrealUIMechanics (stub), UnrealSkinMechanics (slice 1).
  `AdvancedSessions` is vendored third-party — never claim it.
- Status 2026-09-30: `github.com/rfowlie/ConnectIt` answers HTTP 200 unauthenticated → **the repo is public**.
  The earlier 6-week career plan no longer exists; this plan replaces it.

## Positioning (the one-sentence pitch)

> Gameplay/systems programmer with a technical-artist's instinct for **designer-facing, data-driven tools** —
> built a replicated turn-based strategy game with async AI and a plugin architecture, solo, in UE5 C++.

The TA background is a *differentiator*, not a footnote: studios want gameplay engineers who make systems
designers can actually drive. Lead with that.

## What to highlight — 4 case studies (source: existing decision notes)

| # | Case study | Shows | Source notes (vault) |
|---|---|---|---|
| 1 | **Async MinMax AI calling the real game rules** | Threading (`UE::Tasks`), thread-safety design, AI, reuse of one rule implementation for play + search | `_decisions/2026-09-24-minmax-calls-real-rules-as-static-thread-safe-functions`, `design/classic-minmax-rebuild` |
| 2 | **Authoritative replicated board state** | Networking/replication, server gate, state revision counter, mediator-only mutation, request → ordered step-list events | `board-architecture-overhaul`, `server-gate-in-mediator-and-state-revision`, `board-change-event-is-an-ordered-step-list`, `systems/board-state-single-source-of-truth` |
| 3 | **Designer-editable turn-end requirement trees** | Instanced UObjects, AND/OR composition, data-driven config — the TA→programmer bridge | `turn-end-tree-as-instanced-uobject-nodes`, `generic-turn-end-requirement-system`, `action-config-keyed-by-class-not-tag` |
| 4 | **Built it, then removed it** | Judgment, refactor discipline, written decisions — answers "can you scope and finish?" | `faction-visuals-subsystem-built-then-removed`, `retire-legacy-mvvm-pipeline`, `legacy-action-system-removed-stage-3` |

Supporting "breadth" strip (one line each, not full write-ups): board shift (row/col/diagonal rotation with
opt-out tiles and re-scoring), game-agnostic plugin suite (grid / turn-based / game / intelligence), influence-map
debug visualization, editor validator, tag-driven Enhanced Input binder.

**Honest-claims rule:** describe dormant/stub/slice-1 plugins as such or omit them; credit AdvancedSessions.

## Workstreams and sequence (time-boxed; ~4 weeks, front-loaded for the hunt)

### Week 0 — Safety (do before anything is public; ~half day)
1. Check repo visibility (`gh repo view rfowlie/ConnectIt --json visibility`). If public: make private, or
   decide to scrub history. The showcase will be a **separate new public repo** (below).
2. Move `ConnectIt-Vault/Development/` out of this repo (separate private vault) or confirm it's never
   published; remove ex-employer mentions from any file that could become public.
3. Grep the whole vault + repo for the ex-employer/franchise names before any excerpt is lifted.

### Week 1 — Evidence (visuals + the 10-item "demo-complete" checklist)
4. **Record footage** (60–90 s trailer-style cut + 3 short GIFs): place piece → line score; MinMax AI turn;
   board shift; two-client multiplayer turn. Plus 5–6 clean screenshots. (Editor capture is fine; white-box
   is part of the story — label it "white-box systems prototype.")
5. **Architecture diagrams** (3): plugin layering; board-state mediator flow; MinMax threading handoff.
   Reuse vault "systems" pages as source; one clean diagram each, no polish spiral.
6. Write the ≤10-item demo-complete checklist (per career-coach §9). Nothing off-list gets polished.

### Week 2 — The showcase repo + portfolio page
7. New public repo **`ConnectIt-Showcase`** (name flexible): real README (pitch, video/GIFs, architecture
   diagrams, feature list with honest status, tech stack, links to the 4 write-ups), MIT or all-rights
   note on excerpts, `THIRD_PARTY.md`. Contains **curated code excerpts** (not the tree): e.g.
   `ApplyLineScoring` static function, the requirement-node classes, the board mediator's request/gate path.
   Each excerpt has a 3–5 line "why it's shaped this way" header.
8. **Portfolio page on existing site:** hero video → one-paragraph pitch → role/tools/solo/duration →
   4 case-study cards (problem / approach / result, each linking to its write-up) → breadth strip →
   link to Showcase repo. Keep TA work on its own page, cross-linked as "how I think about tools."
9. Optional: put a "Systems prototype" build on itch.io **only if** packaging takes ≤ 1 day; otherwise skip.

### Week 3 — Write-ups (technical writing = public presence)
10. Publish 3 posts (dev.to / Medium / personal site, ~800–1200 words each), one per case studies 1–3,
    adapted from the decision notes (they're already written reasoning — this is mostly editing):
    - "Running MinMax off the game thread without duplicating your rules"
    - "An authoritative board in UE5: mediator, requests, and a revision counter"
    - "Designer-editable AND/OR rule trees with Instanced UObjects"
11. Post 4 ("Built it, then removed it") as a shorter LinkedIn-native piece.

### Week 4 — Distribution, outreach, applications
12. **Channels** (one post each, staggered, video first): LinkedIn (primary — recruiters), r/unrealengine,
    Unreal Source forums, r/gamedev (Feedback Friday), Bluesky/Mastodon/X game-dev tags, #screenshotsaturday.
13. **Network rebuild via sharing (3–5 named people):** send the demo + one write-up to former colleagues
    who "clicked," one indie dev, one gameplay programmer a step ahead. Ask for a specific 15-min feedback,
    not a job. (Directly addresses the isolation pattern.)
14. **Applications:** rewrite resume top section + LinkedIn headline/About around the pitch; tailor one
    ConnectIt bullet set per role type (gameplay programmer, tools programmer, technical designer, generalist
    at small studios). Add the portfolio link and 3-line "what I built" to every application. Fixed
    weekly application count, tracked.
15. **Interview narrative prep:** a 2-min walkthrough per case study + "why pivot from TA" story + honest
    answer on scope ("here's what I cut, here's why") using the decision log.

### Small proof-gap fixes (time-boxed to ≤ 1 day total, inside Week 1–2, only if they serve the showcase)
- 5–8 UE Automation tests for scoring/board rules (pure functions, cheap, visible signal).
- Fresh-clone build note (how to build, UE version) in the Showcase README.
- Do **not** refactor, finish dormant plugins, or polish internals (perfectionism guard).

## Success metrics
- By end of Week 2: public page live with video + 4 case-study cards; Showcase repo README complete.
- By end of Week 3: 3 write-ups published.
- By end of Week 4: ≥ 1 post on each of LinkedIn + one community channel; ≥ 3 network conversations
  started; every outgoing application carries the portfolio link.
- Ongoing: interview/recruiter replies that mention ConnectIt (tracks whether the framing works).

## Critical files / sources
- Case-study raw material: `ConnectIt-Vault/ConnectIt/_decisions/*.md`, `ConnectIt-Vault/ConnectIt/design/*.md`,
  `ConnectIt-Vault/ConnectIt/code/systems/*.md`
- Code excerpt sources: `Source/ConnectIt/Private/Board/Rules/ConnectIt_LineScoringRule.cpp`,
  `Source/ConnectIt/Private/MinMax/ConnectIt_MinMaxTreeBuilder.cpp`, board mediator/state in
  `Plugins/UnrealGridMechanics/`, turn-end requirement nodes in `Plugins/UnrealTurnBasedMechanics/`
- Must fix/remove before public: `ConnectIt-Vault/Development/**`, `FIXES.txt`, `steam_appid.txt.txt`
- README to replace: `README.md` (repo root) — or leave as-is if the main repo stays private and only
  the Showcase README is public.

## Verification (how we know it's done)
- `gh repo view` shows the main repo private (or Development/ scrubbed from history).
- Repo-wide grep for ex-employer/franchise names returns nothing in any public artifact.
- A fresh reader (friend/peer) can, in < 3 min on the portfolio page, state what ConnectIt is and name one
  technical challenge solved.
- Video plays from the page; all links resolve; claims about AI/plugins match actual status.

## How I can help when execution starts
Draft the Showcase README, the portfolio page copy, all 4 write-ups from the decision notes, resume/LinkedIn
rewrites, the excerpt selection with "why" headers, the automation tests, and the 10-item checklist — you
supply footage, screenshots, and the publishing clicks. Afterward: update memory (stale `career-plan-file`
pointer) and log the plan in the vault.
