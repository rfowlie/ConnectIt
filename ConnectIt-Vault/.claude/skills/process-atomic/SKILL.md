---
name: process-atomic
description: >-
  When the user wants to enforce the vault's single atomic-notes pool. Finds any
  atomic/ or _atomic/ folder that isn't _core/_atomic/, moves its notes into
  _core/_atomic/, and deletes the invalid folder. Use for "process atomic",
  "/process-atomic", "check for duplicate atomic folders", "merge atomic notes".
---

# Process Atomic

Enforce `_schema/_atomic.md`'s single-pool rule: **there is exactly one atomic folder in
the vault, `_core/_atomic/`.** Anything else calling itself `atomic/` or `_atomic/` gets
folded in and removed.

## Purpose

`_schema/_atomic.md` says a second atomic folder anywhere is an error to be merged and
deleted, but nothing checks for one. This skill is that check, run as a fix: find every
offending folder, move its notes into `_core/_atomic/`, delete what's left behind.

## Inputs to read first

1. Every folder in the vault named `atomic` or `_atomic` — glob for both, at any depth.
2. [`_core/_atomic/`](../_atomic/) — the canonical pool, to check for filename collisions
   before moving anything into it.
3. [`_schema/_atomic.md`](../_schema/_atomic.md) — the note shape, to sanity-check a file
   before moving it is actually an atomic note and not a stray README.

## Arguments

- None. This always sweeps the whole vault — a partial run would leave the invariant
  ("exactly one pool") unverified.

## Steps

1. List every `atomic/` or `_atomic/` folder found that is **not** `_core/_atomic/`.
   If none, say so and stop.
2. For each offending folder, for every file in it (skip its own `__README.md` — that's
   folder-specific documentation, not a note to merge):
   - If no file of that name exists in `_core/_atomic/`, move it there unchanged.
   - If a file of that name **already exists** in `_core/_atomic/`, do **not** overwrite
     either copy — leave both in place and flag the collision for the user to resolve by
     hand.
3. After moving what it safely can, delete the offending folder **only if it is now
   empty**. If collisions left files behind, leave the folder (now holding only the
   collided files) and say so explicitly.
4. Never touch `_core/_atomic/`'s own contents beyond adding the moved-in files.

## Output format

### 1. Merged
Per offending folder: `folder path → files moved`, and whether the folder was deleted.

### 2. Flags
Every filename collision (both paths named, neither altered) and any offending folder
left behind because of one.

### 3. Log
Write an `_core/_logs/<YYYY-MM-DD-HHMM>.md` note per `[[TSchemaLogs]]` (scope `vault`),
and add its line to `_core/_logs/__INDEX.md`. Confirm the path.

## Style

Terminal markdown, no preamble. Never overwrite a file to resolve a collision — flag it
instead. Never delete a folder that still has files in it.
