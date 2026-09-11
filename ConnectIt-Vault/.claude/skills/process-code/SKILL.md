---
name: process-code
description: >-
  When the user wants their code/ documentation checked or refreshed against the actual
  source. Finds every code/ folder, checks each page's commit: provenance anchor against
  git history, and re-reads + rewrites any page that has drifted. Updates __INDEX.md
  status cells. Use for "process code", "/process-code", "check the code docs are
  current", "refresh stale code pages".
---

# Process Code

Keep every `code/` page honest against the source it documents. The deliverable is
**drift caught and fixed** — a page that's still accurate is left alone; a page that's
drifted is re-read and rewritten; a type with no page yet is named, not authored.

## Purpose

`_schema/_code.md` defines a provenance model — `commit:` is the last commit that touched
a page's `source:` paths, and a page is fresh only while
`git log <source paths> <commit>..HEAD` is empty. Nothing currently runs that check. This
skill is the mechanical sweep: find drift, refresh what's drifted, leave everything else
untouched.

## Inputs to read first

1. Every `code/__INDEX.md` in the vault — glob `**/code/__INDEX.md`. Each row names a
   type, its `source:` paths (from the matching page's frontmatter), and its current
   `status`.
2. [`_schema/_code.md`](../_schema/_code.md) — the page shapes (`[[TSchemaCode]]`,
   `[[TCodeSystem]]`, `[[TCodeRecipe]]`) and the refresh rule.
3. For each page with `status: current` or `status: stale`, its own frontmatter
   (`source:` / `spans:` / `touches:`, `commit:`, `reconciled:`).

## Arguments

- None. If the user names a domain or a `code/` folder, limit the sweep to it.

## Steps

For each `code/` folder found:

1. For every page listed in `__INDEX.md` with a real page (`status` `current` or `stale`),
   run `git log -1 --format=%h -- <its source/spans/touches paths>` and
   `git log <those paths> <stored commit:>..HEAD`.
2. **Empty log → no drift.** Leave the page untouched. If `__INDEX.md` still said
   `stale`, flip it to `current` only if you also update `reconciled:` — otherwise leave
   both as they are.
3. **Non-empty log → drift.** Re-read the current source at those paths in full, rewrite
   only the sections that no longer match it (per `_schema/_code.md`'s body order for that
   page kind), then set `commit:` to the new `git log -1 --format=%h -- <paths>` result and
   `reconciled:` to today. Set the `__INDEX.md` row to `current`.
4. **A `source:`/`spans:`/`touches:` path that no longer exists in the repo** — don't
   guess a replacement. Leave the page, set `__INDEX.md` status to `stale`, and flag it.
5. **`status: stub` rows** — skip. A stub means no page has been authored yet; writing
   one requires reading and understanding a type this skill hasn't been told to look at.
   Never author a new page or add a new inventory row on your own initiative.
6. Leave `code/__README.md` and any deviation notes untouched.

## Output format

### 1. Refreshed
A table: `code/ folder | page | old commit → new commit`. Only pages actually rewritten.

### 2. Unchanged
One line: how many pages were checked and confirmed still current.

### 3. Flags
Stale pages left unrefreshed (missing source paths), and every `stub` row seen (a
reminder of what still has no page — not a task list to act on).

### 4. Log
Write an `_core/_logs/<YYYY-MM-DD-HHMM>.md` note per `[[TSchemaLogs]]` (scope `vault`),
and add its line to `_core/_logs/__INDEX.md`. Confirm the path.

## Style

Terminal markdown, no preamble. Never invent a page for a `stub` row. Never touch a page
that isn't actually stale. Rewrite only the sections the source change actually affects —
don't rewrite a whole page to fix one paragraph.
