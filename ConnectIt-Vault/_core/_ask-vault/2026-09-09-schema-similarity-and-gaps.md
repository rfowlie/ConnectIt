---
created: 2026-09-09
question: "Assess all the schemas for near-duplicates; flag gaps and roughly suggest new schemas to fill them."
status: insufficient
tags:
  - ask-vault
---

# Are any of the schemas mostly similar, and what gaps need new schemas?

## Question

Assess every schema rulebook for near-duplicates, and where there are obvious gaps,
rough-suggest new schemas. Scope assumed: the `_schema/` section rulebooks.

## Answer

Insufficient material in the vault to answer this.

## Sources

None.

## Gaps

- **The answerable corpus is empty.** `/ask-vault` reads content notes only — atomic
  pool, `data/wiki/`, `_frameworks/`, `decisions/`, `maps/`, `people/` / `meetings/` /
  `daily/`, domain reference docs. This vault currently has none of those and no domains;
  it is a pure schema/template vault.
- **The subject is out of scope by design.** The question is about the `_schema/`
  rulebooks themselves. The skill's hard constraints bar `_schema/`, `CLAUDE.md`,
  `ABOUT.md`, and `_extras/` machinery as answer sources ("machinery, not knowledge").
- **How to get it answered:** run it as a direct request (not `/ask-vault`), or via a
  dedicated schema-audit / `improve-system`-style pass; or first capture the comparison
  as a content note (e.g. a `_maps` entry) that a later `/ask-vault` could cite.

## Confidence

`insufficient` — the answerable corpus is empty and the subject matter is excluded from
this skill's sources by design.
