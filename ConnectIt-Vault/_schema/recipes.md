# _schema/recipes.md

A `recipes/` folder holds **step-by-step task procedures** — "add a new turn action",
"add a turn-order strategy", "hook a system into the turn-end gate". One page per task.

Where `code/` explains *what a type is* and `systems/` explains *how a flow works*,
`recipes/` assembles both into *what to do*, in order, for a concrete change.

Follows the provenance model in [`code.md`](code.md).

## Layout

```
recipes/
  index.md              one row per recipe: task | when you'd do it | status
  <task-slug>.md         one page per task (e.g. add-a-turn-action.md)
```

## Page frontmatter

```yaml
---
schema: recipes
task: <one-line statement of the goal>
touches:                 # files / types a correct implementation edits — also the cross-impact list
  - <repo-relative path or type name>
reconciled: <YYYY-MM-DD>
commit: <short SHA>
---
```

## Page body (fixed order)

1. **Goal** — what you'll have when done.
2. **Prerequisites** — what must already exist.
3. **Steps** — numbered. Each step names the concrete file / type / member and the change.
   Code snippets allowed but minimal — link to the `code/` page for detail.
4. **Verify** — how to confirm it works: PIE steps, what to watch in the debug widgets,
   any automation test.
5. **Pitfalls** — the mistakes people make doing this task.
6. **See also** — the `code/` / `systems/` pages behind the steps.

## Rules

- **Executable without leaving the vault + the named source files.** A reader should not
  need to reverse-engineer anything the recipe skipped.
- **`touches:` is the cross-impact list.** If a step edits a file, it's in `touches:`.
- Keep snippets short; the authoritative signature lives on the `code/` page or in the
  source.
- Provenance + refresh per [`code.md`](code.md).

## Deviations

Add `recipes/README.md` starting `Extends _schema/recipes.md. Differences:` for
section-specific rules.
