# _schema/_daily.md

A `daily/` folder holds **one free-form note per day** — a capture surface for whatever
happened, got done, got stuck, or came to mind. The scratchpad the rest of the vault is
distilled from.

## Filename

`daily/<YYYY-MM-DD>.md` — one per calendar day, created when first needed.

## What a note holds

Whatever's useful that day — no fixed structure. Common shape: what got done, what's
open, notes to self, links to anything created or touched.

## Rules

- Dated filename, one per day (see `_schema/README.md` → **Dated event notes**) — but
  unlike the others, a daily note *is* edited through its own day.
- **Not a log.** `_logs` is machine-written maintenance history; `daily/` is the human's
  own running notes. Different folder, different purpose.
- Daily notes are raw input. When something in one matters beyond the day, move it to the
  right section (`decisions/`, `people/`, `data/`, a project domain) and link back.
- Don't backfill empty days.

## Template

[[TSchemaDaily]]

## Deviations

Add `daily/README.md` starting `Extends _schema/_daily.md. Differences:` if a domain
wants a fixed daily structure or a weekly/monthly rollup.
