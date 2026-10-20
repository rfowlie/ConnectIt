---
created: 2026-10-07
tags:
  - plan
  - portfolio
  - advertising
source: "[2026-10-07 interview](_discussions/2026-10-07-advertising-strategy-interview.md) + web research (links at end)"
---
# Advertising strategy — where each piece of work lives

Extends [portfolio-plan.md](portfolio-plan.md) (what to show); this file is *where and how to publish it*.

## Principles (from the interview)

1. **Recruiters first.** Anyone landing on a link has ~2 minutes: one clear page, one short video, one link
   to proof. Everything else serves that path.
2. **Hub and spoke.** Each platform does one job. The website is the curated front door and links out; it
   never has to *store* heavy media again (solves the 1 GB cap).
3. **Write once, in the vault.** Each post starts as markdown in `Portfolio/`; platform versions are
   adaptations (shorter caption, different first line), not rewrites. This is what makes ~5 h/week work.
4. **Weekly, small, consistent** beats occasional and big. Weekly devlog = 1 clip + 1 short write-up.
5. Comfortable formats only: written posts, silent captioned clips/GIFs, voiceover video. No on-camera.

## Platform roles

| Platform | Job | What goes there | Audience served |
|---|---|---|---|
| **Portfolio site** (builder) | Hub / front door | Project page (hero video embed, pitch, 4 case-study cards), devlog index (title + 1-line + links), contact. Short pages only. | Recruiters |
| **LinkedIn** | Discovery + recruiter reach | Native-uploaded short clip + 3–5 sentence post per devlog; Featured section pins the hub, showcase repo, best video; headline/About carry the pitch | Recruiters, hiring managers |
| **YouTube** | Video host of record | Public channel, one **ConnectIt Devlog** playlist; trailer (60–90 s) + weekly clips + occasional narrated walkthroughs; chapters + links in description; embed everywhere else | Everyone (host), programmers (search) |
| **GitHub** | Proof of code + long-form | Public **`ConnectIt-Showcase`** repo: README (pitch, GIFs, diagrams, honest status), curated excerpts with "why" headers, `docs/` holding the long write-ups as markdown; pinned on profile | Programmers, technical recruiters |
| **itch.io** | Playable proof | One project page: "white-box systems prototype" build, trailer embed, devlog posts mirrored (short), tags (strategy, turn-based, multiplayer, Unreal) | Recruiters who play, collaborators |
| **dev.to / Reddit / Bluesky** (optional) | Peer discovery | Cross-post the long write-ups (dev.to, canonical URL pointing at GitHub/site); one clip to r/unrealengine / r/gamedev on milestone weeks; Bluesky #gamedev for the weekly clip | Programmers, collaborators |

Do **not** start new platforms (Medium, X, Discord server, Steam page) until this set is running for 4 weeks.

## Media placement matrix

| Asset | Canonical home | Also posted to | Format notes |
|---|---|---|---|
| Trailer / hero video (60–90 s) | YouTube (public) | Site embed, LinkedIn Featured, itch page, GitHub README (thumbnail link) | 1080p, captions burned or uploaded, no voice required |
| Weekly clip (15–45 s) | YouTube (devlog playlist) | **LinkedIn as native upload** (not a YouTube link), Bluesky | Silent + on-screen captions; first 3 seconds show the result |
| GIFs | GitHub repo (`/media`) | README, site only if small | Keep each small (a few MB); prefer short MP4 where the host allows |
| Screenshots | GitHub repo + itch | Site, LinkedIn carousel | PNG/WebP, consistent framing and annotation style |
| Architecture diagrams | GitHub repo (SVG + PNG) | Site case study, write-ups | One visual style across all three |
| Long write-ups | GitHub `docs/` (markdown) | Site (summary + link), dev.to (cross-post), LinkedIn (excerpt) | Problem → Role → Decision → Outcome |
| Playable build | itch.io | Linked from site, README, LinkedIn Featured | Windows zip via `butler` (website upload is capped lower; butler handles larger) |
| Code excerpts | Showcase repo | Linked from write-ups | 3–5 line "why it's shaped this way" header each |

## The weekly loop (~5 h)

| Slot | Time | Output |
|---|---|---|
| Capture | 1 h | One new 15–45 s clip of what changed this week (or an old feature, re-framed) |
| Write | 1.5 h | Vault markdown devlog: Problem → Decision → Result, ≤ 400 words, embedded clip link |
| Publish | 1 h | YouTube upload (title, chapters, links) → GitHub `docs/` entry → site devlog index line → itch devlog (optional) |
| Adapt + post | 1 h | LinkedIn native clip + 3–5 sentence version; optional Bluesky/Reddit on milestone weeks |
| Engage | 30 min | Reply to comments; send 1–2 person-to-person messages (the networking step from the plan) |

Every 3rd–4th week the Write slot grows into a full case study (the 4 in `portfolio-plan.md`).
If a week is missed: skip it; do not backfill. A week with only the clip still counts.

## Naming and consistency

- One handle everywhere (`rfowlie` already on GitHub — claim the same on YouTube/itch/Bluesky where possible).
- One project name and tagline: *ConnectIt — a replicated turn-based strategy prototype in UE5 C++*.
- One thumbnail/banner style; one color/annotation style for diagrams.
- Every profile bio links to the **hub**, never to the other platforms individually. Hub lists them all.
- Pin/feature: LinkedIn Featured = hub + trailer + showcase repo; GitHub pinned = Showcase; YouTube channel
  trailer = the 60–90 s cut; itch page = trailer + build.

## Rollout (order matters — each step needs the one before)

1. **Safety:** resolve the public-repo / `Development/` question first (see [`_tasks/active.md`](_tasks/active.md)).
2. **Trailer + 3 GIFs** (the footage from the plan) → YouTube (public).
3. **Showcase repo** (README with GIFs + diagrams) → pin on GitHub.
4. **Site hub page** rebuilt to link out; old heavy media moved to YouTube/GitHub; old posts kept as short
   pages linking to their new homes (don't break existing links).
5. **LinkedIn** profile update (headline, About, Featured) + first native-video post.
6. **itch.io build + page** once a packaged build runs (check it works without Steam).
7. Start the **weekly loop**; first long write-up in week 3.

## Risks and open items

- **Packaged build vs Steam:** `steam_appid.txt.txt` is in the tree; confirm the build runs and a match can be
  played without Steam before promising a playable download.
- **Website builder limits:** confirm it supports YouTube embeds and external link blocks; if not, a single
  GitHub Pages page can serve as the hub at no storage cost (open question).
- **Vendor-blog claims** below (LinkedIn native-video reach) are directional marketing numbers, not
  guarantees; treat as "native upload is worth the extra minute," not as a metric to chase.
- **Don't overbuild:** the loop is sized to ~5 h. If it grows, cut optional channels, not the clip.

## Sources consulted (2026-10-07)

- [How to build a game dev portfolio that gets you hired](https://respawn.outlookindia.com/gaming/gaming-news/how-to-build-a-game-dev-portfolio-that-gets-you-hired) — 3–5 projects, Problem-Role-Decision-Outcome, itch.io for playables + site for narrative, GitHub for code.
- [Game developer portfolio guide (MAGES)](https://mages.edu.sg/blog/game-developer-portfolio-guide/) — what reviewers expect.
- [itch.io forum: uploads over 1 GB via butler](https://itch.io/jam/scream-jam-2024/topic/4084561/are-uploads-greater-than-1gb-okay-as-long-as-theyre-uploaded-via-butler-and-accessible-through-itch-no-external-links) — upload limits (website vs butler), 1000-file page limit.
- [LinkedIn video in 2026](https://www.visla.us/blog/guides/linkedin-video-in-2026-whats-working-and-how-to-make-it/) and [LinkedIn native video strategy](https://www.trymypost.com/blog/linkedin-native-video-strategy-b2b-growth-2026) — native upload outperforms outbound links.
- [Unlisted vs public YouTube videos](https://ponderworthy.com/unlisted-videos-on-youtube-how-they-actually-work-and-why-most-people-use-them-wrong-ojo) — unlisted videos aren't searchable, so a public devlog playlist is better for discovery.
