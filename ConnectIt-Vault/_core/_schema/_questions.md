# _schema/_questions.md

A `questions/` folder holds **one note per open question or unresolved thread** — the
things the vault (or a domain) hasn't settled yet. It's the live counterpart to
`_decisions` (settled calls) and distinct from an `_ask-vault` record (a one-shot Q&A
snapshot); a `questions/` note can *link* to either as the thing that closes it.

## Structure

```
questions/
  <kebab-slug>.md    one per question — a stable name that persists until the question closes
```

Not dated in the filename: a question is a living item, not an event.

## What a note holds

Frontmatter: `created:`, `status: open | answered | parked`, `closed-by:` (`[[decision]]`
or `[[ask-vault note]]`, filled when it closes), `tags: [question]`.

Body:

- `# <the question>` — stated plainly.
- `## Why it matters` — what turns on the answer.
- `## What would answer it` — the evidence, note, or decision that would resolve it.
- `## Current thinking` — the running notes; updated in place while `open`.
- `## Resolution` — filled when `status` moves to `answered` or `parked`: the answer (or
  why it's parked) and a link to whatever settled it.

## Rules

- **Never delete a closed question.** Set `status`, fill `## Resolution`, point
  `closed-by` at the note that resolves it.
- A question that becomes a settled call also gets a `_decisions` note; `closed-by` links
  to it.
- `ABOUT.md`'s informal "Open questions" list is the seed for `_core/_questions/` —
  migrate items here as they get real notes.
- Created on first use.

## Mounts

Either: `_core/_questions/` for vault-level threads, `<domain>/questions/` for a
domain's own.

## Template

[[TSchemaQuestions]]

## Deviations

Add `questions/__README.md` starting `Extends _schema/_questions.md. Differences:` for extra
frontmatter or a required triage cadence.
