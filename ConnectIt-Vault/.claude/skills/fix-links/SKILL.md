---
name: fix-links
description: >-
  When the user wants a vault-wide link health check. Finds every standard Markdown link
  (`[text](path)`) in the vault, checks whether its target actually exists on disk, and
  for each broken one proposes a fix by searching the vault for the likely real target —
  applying nothing without the user confirming each proposed fix. Use for "fix links",
  "/fix-links", "check for broken links", "link health check", or after any file move or
  rename.
---

# Fix Links

Keep every Markdown link in the vault pointing at a file that actually exists. The
deliverable is **broken links found, a fix proposed for each one that has a confident
match, and nothing changed until the user says which proposals to apply.**

## Purpose

`_core/CLAUDE.md` sets standard Markdown links (`[text](path)`) as this vault's link
format specifically because they're directly actionable — at the cost that nothing
rewrites a link's path when its target moves or is renamed (see
`_core/_atomic/link-format-choice-depends-on-maintainer-and-consumer.md`). This skill is
that missing upkeep: a mechanical sweep that catches what the format doesn't fix on its
own. It never guesses silently — a wrong auto-fix is worse than a link left broken and
flagged.

**Obsidian `[[wikilinks]]` are out of scope by design.** Obsidian keeps those targets
correct automatically on rename/move; if any remain in the vault (a skill file, `_atomic/`
cross-references, `Development/`'s older content), note their count but don't check or
touch them — checking them the same way as a Markdown link would be redundant work for a
format that already self-heals.

## Inputs to read first

1. Every `*.md` file in the vault (excluding `.claude/`, `.obsidian/`) — the corpus to
   scan. `.claude/skills/**/SKILL.md` is excluded deliberately, not just as boilerplate:
   it's a byte-identical mirror of an authored `_core/_skills/` (or `<domain>/_skills/`)
   file, sitting one directory level deeper (the extra `<name>/` folder) — so a relative
   link that's correct in the authored copy reads as broken in the mirror even when
   nothing is actually wrong. Check the authored copy; a `process-skills` mismatch is a
   different, already-covered problem.
2. `_core/CLAUDE.md` — confirms the link-format standard this skill enforces.

## Arguments

- None — scans the whole vault.
- A path (domain or folder) — scope the scan to files under it. Restate the resolved
  scope in one line at the top of the output.

## Steps

1. **Extract.** In every `.md` file, find every `[text](path)` link. For each: skip it if
   `path` starts with `http://`, `https://`, or is a bare `#fragment`. Split off any
   `#fragment` from the rest. Count (but don't inspect) any `[[wikilink]]` seen in the
   same pass, for the report's tally.
2. **Resolve.** Treat the remaining path as relative to the linking file's own directory
   (or vault-root-relative if it starts with `/`). Check whether that file exists on disk.
3. **For each broken link, search for the real target** before giving up:
   - Same basename, different directory — the file moved. Vault-wide glob for that exact
     filename.
   - Same directory, close-but-different name — a rename. Check for the vault's own known
     renaming conventions first (`index.md` → `__INDEX.md`, `README.md` → `__README.md`),
     then a case-insensitive or fuzzy match in the same folder.
   - If **exactly one** plausible candidate turns up by either check, that's the proposed
     fix. If **zero or more than one**, it has no confident match — don't guess between
     them.
4. **Never apply a fix on this pass.** Collect every finding into the report below and
   stop — fixes are applied only in a follow-up pass once the user has said which
   proposals to accept (see Output format).
5. If the user is continuing a prior run and has confirmed specific fixes, apply exactly
   those (and only those) via a direct edit of the `(path)` portion of the link — leave
   the display text untouched unless the user asked to change it too. Re-run step 1–2
   afterward to confirm the vault is clean, or at least that those specific links now
   resolve.

## Output format

### 1. Broken links with a confident fix
Table: `file | link text | broken path | proposed fix`. Grouped by file. This is what
needs the user's yes/no.

### 2. Broken links with no confident match
Table: `file | link text | broken path | why (0 candidates / N candidates)`. If there are
multiple candidates, list them all so the user can pick.

### 3. Clean
One line: how many Markdown links were checked and resolved fine.

### 4. Wikilinks not checked
One line: how many `[[wikilinks]]` were seen and skipped, and where (by domain/folder),
so the user knows the scope wasn't silently incomplete.

### 5. Ask
End by asking which of section 1's proposed fixes to apply — "all", a list, or "none" —
and, for section 2, whether the user can supply the right target directly. Do not apply
anything until they answer.

### 6. Log (after fixes are applied, not before)
Write an `_core/_logs/<YYYY-MM-DD-HHMM>.md` note per `_core/_templates/TSchemaLogs.md`
(scope `vault`) — `## Changed` lists every link actually rewritten, `file: old path ->
new path`. Add its line to `_core/_logs/__INDEX.md`. Confirm the path. Skip this step
entirely if the user applied nothing.

## Style

Terminal markdown, no preamble. Never rewrite a link without it appearing in the user's
confirmed list. Never treat "one candidate found" as certain if the candidate is a poor
name match found only by a loose fuzzy search — say so and let the user decide instead of
overstating confidence.
