# _schema/section.md

A **section** is a top-level content folder in the vault — one coherent subject with its
own lead document and, optionally, schema-governed subfolders. A folder becomes a section
by containing a `_section.md` marker file.

This rulebook defines the marker. It is the entry point a generic tool or agent uses to
enumerate a vault's sections.

## Discovery rule

- A folder is the **schema set** if it is named `_schema/`.
- A folder is a **section** if it contains `_section.md`.
- The `_` prefix marks both as *structural*, not content. Everything else is ordinary
  notes/attachments belonging to whichever section contains it.

## `_section.md` shape

```markdown
---
type: <free-text shape label — e.g. code-unit | notes | log>
lead: <lead doc filename in this folder — e.g. high-level.md>
schemas: [<kind>, ...]   # _schema/<kind>.md rulebooks that govern this section's subfolders; [] if none
---

# <folder name> — section

<one line: what this section is.> Lead doc: [[<lead without .md>]].
<one line per governed subfolder, pointing to its rulebook — omit if schemas is []>
```

### Fields

| Field | Meaning |
|---|---|
| `type` | A short label for the section's shape, so sections of the same kind can be grouped. Free text; not enforced. Sections with the same `type` should have the same `lead` name and `schemas` set. |
| `lead` | The section's main document, by filename. Decouples "how do I find sections" (this marker) from "what is each section's entry doc called" (varies: `high-level.md`, `overview.md`, `README.md`). |
| `schemas` | The `_schema/<kind>.md` rulebooks that apply to subfolders of this section. Each entry `<kind>` must have a matching `_schema/<kind>.md`. `[]` means the section has no schema-governed subfolders and its lead doc plus local convention is the whole story. |

## Adding a section

1. Create the folder and its `lead` document.
2. Add `_section.md` with the three fields.
3. For each `<kind>` in `schemas`, that subfolder is created on first use and follows
   `_schema/<kind>.md` — do not pre-scaffold it.
4. Add a row for the section to the vault `CLAUDE.md` folder map.

A section can adopt more schemas over time. A code-unit section starts as
`schemas: [updates]` and adds `code`, `systems`, `recipes` once its source has been
ingested (see [`code.md`](code.md)).

## Deviations

If one section needs a governed subfolder to behave differently from its rulebook, add a
`README.md` **inside that subfolder** opening with:

```
Extends _schema/<kind>.md. Differences:
```

…and list only the deltas. Small, section-wide notes can instead go in the body of
`_section.md`. Either way the rule text travels with the folder it governs.

Only mint a **new** `_schema/<kind>.md` when the behaviour is a genuinely different kind
of thing another vault could also reuse — not a one-off tweak.
