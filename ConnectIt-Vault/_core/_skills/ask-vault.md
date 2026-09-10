---
name: ask-vault
description: >-
  When the user wants a question answered ONLY from what is already written in the vault —
  no web, no outside knowledge. Scans the vault's content notes, answers with citations,
  and when the material isn't there says so and names the gaps. Logs every Q&A to
  _core/_ask-vault/. Use for "ask the vault", "what does my vault say about…", or any
  question that must be grounded solely in existing notes.
---

# Ask the Vault

Answer a question using only what the vault already contains. The deliverable is a
**sourced answer or an honest "not enough here"** — never a plausible answer padded with
general knowledge.

## Purpose

Treat the vault as a closed corpus. Every claim in the answer traces to a note you cite.
If the notes don't cover the question, that is the finding: report insufficiency and show
where the holes are so the user knows what to capture next.

## Hard constraints

- **No web.** Do not use WebSearch, WebFetch, or any external lookup.
- **No outside knowledge.** Don't answer from what you already know about the topic. A
  sentence not supported by a cited vault note does not go in the answer — it goes in Gaps.
- **Content only.** Don't mine `_schema/`, `CLAUDE.md` routers, `ABOUT.md`, or
  `_core/_templates|_logs|_clippings/` for answers — that's machinery, not knowledge.

## Inputs to read first

Resolve these against the host vault's layout (a domain's `CLAUDE.md` folder map lists its
content sections — read the map only to find files, not as an answer source). Scan
whichever exist for material on the question:

1. `_core/_atomic/` — the single-idea pool, incl. notes tagged `mental model`; usually
   the densest hits.
2. `data/wiki/**` in every domain — per-source synthesis.
3. `decisions/` — settled calls and their reasoning; `questions/` — open threads.
4. `maps/` — curated entry points; follow their links.
5. `people/`, `meetings/`, `daily/` — when the question is about a person, a conversation,
   or a date.
6. `code/` pages — for questions about a domain's source (what a type does, a flow, a
   recipe).
7. Domain lead / reference docs named in each domain's `CLAUDE.md` folder map.
8. `_core/_ask-vault/` — prior Q&As on the topic; build on them, don't repeat.

If an expected area is absent or empty, note it — that's a gap, not licence to guess.

## Arguments

- The question is whatever follows `/ask-vault`.
- If nothing was passed: ask "What question should I put to the vault?" and wait.
- Restate the resolved question in one line at the top of the output.

## Output format

### 1. Question
One line: the question as you're answering it, plus any scope you had to assume.

### 2. Answer
The synthesis, built only from cited notes, `[[wikilink]]` at each claim. If the vault
doesn't support an answer, this section is exactly one line:
**"Insufficient material in the vault to answer this."**
Partial coverage is fine — answer what's supported and push the rest to Gaps.

### 3. Sources
Every note used, one per line: `[[note]] — what it contributed`. "None." if nothing
relevant was found.

### 4. Gaps
What's missing to answer fully: which content areas are empty or thin, which sub-questions
have no support, and the specific note(s) that would need to exist. Always present — even a
well-answered question names its limits.

### 5. Confidence
One line — `answered` / `partial` / `insufficient`, and why (e.g. "one atomic note, no
corroboration").

### 6. Log it
Write sections 1–5 to `_core/_ask-vault/<YYYY-MM-DD>-<slug>.md` using the shape in
`_core/_ask-vault/README.md`. Kebab-case slug from the question; `-2` on a same-day
collision. Confirm the path at the end of the response.

## Style

Terminal markdown. No preamble — start at section 1. Don't pad; a thin answer with honest
gaps beats a padded one. Never present uncited knowledge as a vault finding.
