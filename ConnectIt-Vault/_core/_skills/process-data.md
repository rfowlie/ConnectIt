---
name: process-data
description: >-
  When the user wants every raw source note reflected in its synthesis note. Finds every
  data/ folder in the vault, matches each raw/<slug> entry to its wiki/<slug>.md, and
  writes any missing wiki note as a summary of the raw material. Use for "process data",
  "/process-data", "catch up my wiki notes", "summarize new raw sources".
---

# Data Process

Keep every `data/wiki/` in sync with its `data/raw/` — one synthesis note per source,
written the moment a raw file has none.

## Purpose

`_schema/_data.md` requires "every `raw/<slug>/` file is reflected in `wiki/<slug>.md`."
This skill is the mechanical sweep that enforces it: find gaps, write the missing
synthesis, never touch `raw/`.

## Inputs to read first

1. Every `data/` folder in the vault — glob `**/data/raw/**` to find them (a domain's own
   `<domain>/data/`, or a bare vault-root `data/`).
2. [`_schema/_data.md`](../_schema/_data.md) — the rules a new wiki note must follow
   (paraphrase in the source's voice; reserve quotes for what's actually in `raw/`; one
   canonical file per subject).
3. For each `data/` found, its `raw/<slug>/` entries and existing `wiki/<slug>.md` files —
   to compute the gap.

## Arguments

- None. If the user names a domain or a specific `data/` folder, limit the sweep to it.

## Steps

For each `data/` folder found:

1. List every source slug in `raw/` (one subfolder per source).
2. List every existing `wiki/<slug>.md`.
3. For each `raw/<slug>/` with **no** matching `wiki/<slug>.md`: read every file in
   `raw/<slug>/` in full, then write `wiki/<slug>.md` — a synthesis in the source's own
   voice, covering everything in that raw entry, quotation marks reserved for phrases
   actually attested in the raw text. Never edit anything under `raw/`.
4. A `raw/<slug>/` that already has a matching `wiki/<slug>.md` is left untouched — this
   skill only fills gaps, it never refreshes an existing summary.
5. An empty or unreadable `raw/<slug>/` is skipped and flagged, not fabricated.

## Output format

### 1. Processed
A table: `data/ folder | slug | action`. List only the gaps actually filled, plus a
one-line total (`N raw entries scanned, M wiki notes written`).

### 2. Flags
Empty/unreadable raw entries skipped; any `wiki/` file with no matching `raw/` slug
(a possible orphan worth the user's attention).

### 3. Log
Per the vault's standing rule: write an `_core/_logs/<YYYY-MM-DD-HHMM>.md` note (scope
`vault`) per `[[TSchemaLogs]]`, and add its line to `_core/_logs/__README.md`. Confirm the
path.

## Style

Terminal markdown, no preamble. Never edit or reorganize `raw/`. Never rewrite an
existing `wiki/` note — only create the missing ones.
