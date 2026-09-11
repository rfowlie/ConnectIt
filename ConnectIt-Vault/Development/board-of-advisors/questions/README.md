# board-of-advisors/questions/

One file per board consultation — the record of a time the virtual board was convened on a
decision. Written by the `/ask-the-board` skill, and by hand when a board is run manually
(see [`../board-of-advisors.md`](../board-of-advisors.md) → "How to run a virtual board").

Not a `decisions/` folder: a consultation is an *input* to a decision, not the decision
itself. If the consultation settles a call, that call also gets a note in
`../knowledge/me/decisions/` (see [`../../_schema/decisions.md`](../../_schema/decisions.md)).

## Filename

`YYYY-MM-DD-<slug>.md` — e.g. `2026-09-06-save-system-now-or-after-demo.md`

## Contents

```markdown
# <the question, stated plainly>

**Date:** <YYYY-MM-DD>

## The question
One or two lines: what the board was asked, and the constraints that bound the answer.

## Each advisor's take
- **<advisor>** — their view in their voice; bottom line.
- … one per board advisor …

## Where they agree / disagree
The consensus points and the real tensions.

## Recommendation
The sequenced call, grounded in career-coach.md, with an explicit "Don't:" line.
```

## Rules

- Append-only in spirit — don't rewrite an old consultation; run a new one if the situation changed.
- Keep the advisor takes as they were given; this is a record, not a living doc.
