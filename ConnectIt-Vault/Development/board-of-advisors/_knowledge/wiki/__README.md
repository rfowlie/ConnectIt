# board-of-advisors/_knowledge/wiki/

Extends [`../../../_schema/knowledge.md`](../../../_schema/knowledge.md). Differences:

One synthesis per source person, built from everything in the matching `raw/<person-slug>/`
folder. `raw/` is the evidence; `wiki/` is what's distilled from it. These are the **voice
profiles the `ask-the-board` skill reads** to speak as each advisor.

Current: `tim-cain.md`, `jonas-tyroller.md`, `tim-ruswick.md`.

## Format

`wiki/<person-slug>.md`, each covering:

- **Who they are** (as it bears on the advice they give)
- **Core ideas** — numbered, each with the mechanism and their own examples
- **Vocabulary & catchphrases** — a table of their terms
- **Stances** — the flags they plant, as one-liners
- **Recurring stories** — their canon / go-to anecdotes
- **How this applies to the flagship project & pivot** — the bridge back to
  [`../me/career-coach.md`](career-coach.md), with `[[other-slug]]` links noting where
  advisors agree or differ

## Rules

- Paraphrase in the person's voice; reserve quotation marks for phrases actually attested in `raw/`.
- Keep each wiki in sync with its `raw/` folder — a new raw file means a wiki update.
