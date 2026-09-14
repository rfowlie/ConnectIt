---
name: ask-the-board
description: >-
  Convene the user's virtual advisory board on a decision. Reads
  board-of-advisors/board-of-advisors.md (and any board-of-advisors/_knowledge/wiki/*.md
  voice profiles), gives each advisor's take in their own voice, flags where they agree and
  disagree, then synthesizes a concrete recommendation grounded in
  board-of-advisors/_knowledge/me/career-coach.md, and logs the consultation to
  board-of-advisors/_questions/. Use when the user asks to "ask the board", wants advisor
  input on a choice, or is stuck on a career/project decision.
---

# Ask the Board

Run the user's virtual advisory board on whatever they're deciding. The point is a
useful decision, not a roleplay exercise — but each advisor's section must genuinely
sound like that person and use *their* frameworks, not generic advice with a name on top.

Paths below are relative to the vault root (`GameDeveloperVault/`).

## Inputs to read first

1. **`board-of-advisors/board-of-advisors.md`** — the roster. Every advisor listed under a
   numbered heading is on the board. The "Bench" section is optional: include a bench
   voice only if it's directly relevant to the question.
2. **`board-of-advisors/_knowledge/me/career-coach.md`** — the user's situation, goals,
   blockers, and constraints. The synthesis MUST be consistent with this (runway, the
   isolation pattern, the flagship white-box demo milestone, the perfectionism loop, etc.).
   Also check `board-of-advisors/_knowledge/me/_decisions/` for calls already made — don't
   reopen them, and skim `board-of-advisors/_questions/` for prior consultations on the same topic.
3. **`board-of-advisors/_knowledge/wiki/<slug>.md`** for each advisor, if it exists — the
   voice profile: core ideas, vocabulary, stances, recurring stories. Use it to get the
   voice and the specific mental models right. If no wiki exists for an advisor, fall back
   to their description and the 5 content picks in `board-of-advisors/board-of-advisors.md`,
   and say the voice is lower-confidence.
4. **`board-of-advisors/_knowledge/raw/<slug>/`** — only dip into these if the wiki is thin
   on the point at issue.

## The question

- The decision is whatever the user passed as the argument to the skill.
- If they passed nothing, ask one short question: what decision do you want the board to
  weigh in on? Offer a default — "or I can have them review where the flagship project
  and job search stand right now" — and wait for the answer before proceeding.
- Restate the question in one line at the top of the output so the framing is explicit.

## Output format

### 1. The question
One or two lines: what the board is being asked, and any constraints that bound the answer.

### 2. Each advisor's take
One section per board advisor. For each:
- Header: the advisor's name.
- 120–220 words, **first person, in their voice**, addressed to the user ("you").
- Must use at least one of *their* named frameworks / vocabulary / recurring frames
  from their wiki (e.g. Cain: "can vs. should", "foundational vs. available", "B-tier
  is a gift", "setting → story → mechanics"). Don't have them all give the same advice
  in different accents — surface what *this* person specifically would zero in on.
- End that advisor's section with a one-line **bottom line:** their single recommended action.
- Stay honest: if the board material doesn't give you enough to know their view on this,
  say so rather than inventing a position.

### 3. Where they agree
Bulleted consensus points — the things two or more advisors independently land on.
These are the high-confidence signals.

### 4. Where they disagree
The real tensions, each as: *what advisor A would prioritize* vs. *what advisor B would
prioritize*, and what the disagreement is actually about (usually sequencing, scope, or
risk tolerance). If they don't meaningfully disagree, say that and don't manufacture conflict.

### 5. What you should actually do
- A short, **sequenced** plan (numbered, next action first) that resolves the tensions
  with a clear call — don't just restate both sides.
- Ground every step in `board-of-advisors/_knowledge/me/career-coach.md`: name the
  constraint or goal it serves.
- Include an explicit **Don't:** line — what to *not* do right now, and why.
- If the decision is genuinely the user's values call (not a craft question), say so and
  frame the two paths instead of forcing a pick.
- Keep it to something the user can start this week.

### 6. Log the consultation
After presenting the output, write it to
`board-of-advisors/_questions/<YYYY-MM-DD>-<slug>.md` following the format in
`board-of-advisors/_questions/__README.md`:
the question, each advisor's take, the agree/disagree points, and the recommendation.
Use today's date and a kebab-case slug from the question. On a same-day collision add `-2`.
Confirm the path written at the end of the response.

## Style

- Terminal markdown. No preamble like "Great question." Start with section 1.
- Don't pad. If an advisor has little to add on this particular question, two sentences
  and a bottom line is fine.
- Never fabricate quotes as if verbatim. Paraphrase in their voice; reserve quotation
  marks for phrases actually attested in their wiki/raw files.
- Close by noting which advisors were left out or thin, and offer to pull in a bench voice.
