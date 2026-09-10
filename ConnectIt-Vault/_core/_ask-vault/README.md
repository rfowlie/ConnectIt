# _core/_ask-vault/

One note per `/ask-vault` run (or a Q&A done by hand the same way) — a record of *what the
vault could say about a question at the time it was asked*, answered **only** from the
vault's own content notes.

Not a `decisions/` note: a Q&A is an *input* to a decision, not the decision itself. If a
Q&A settles a call, that call also gets a `decisions/` note.

## Filename

`<YYYY-MM-DD>-<slug>.md` — kebab-case slug from the question; add `-2`, `-3` on a same-day
collision.

## Note shape

```markdown
---
created: <YYYY-MM-DD>
question: "<the question, one line>"
status: answered | partial | insufficient
tags:
  - ask-vault
---

# <the question, stated plainly>

## Question
One line: the question as answered, plus any scope that had to be assumed.

## Answer
The synthesis, built only from cited notes, with `[[wikilinks]]` at each claim — or the
single line "Insufficient material in the vault to answer this."

## Sources
- [[note]] — what it contributed
(or "None.")

## Gaps
What's missing to answer fully: empty/thin content areas, unsupported sub-questions, the
note(s) that would need to exist.

## Confidence
`answered` / `partial` / `insufficient`, and why.
```

## Rules

- **Append-only in spirit.** Don't rewrite an old Q&A — ask again and write a new note if
  the vault has changed.
- **Content-sourced only.** Every answer claim cites a vault content note; no web, no
  outside knowledge (the `/ask-vault` skill enforces this).
- **Act on the gaps.** A gap worth filling becomes a stub in `_clippings/` or a new note
  in the relevant section — the Q&A record just names it.
