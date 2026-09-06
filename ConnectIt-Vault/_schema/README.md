# _schema/

The **section rulebooks** for this vault, plus the spec for the folder-marker system
itself. One file per *kind* of folder. They are portable and change rarely — they exist
so that a `logs/` (or `updates/`, or `knowledge/`) folder behaves the same way in every
section, and in every vault this `_schema/` is dropped into.

`_schema/` is not auto-loaded. A `CLAUDE.md` points here; it never restates these rules.

## The three layers

| Layer | File | Auto-loaded? | Scope | Answers |
|---|---|---|---|---|
| **Router / state** | `CLAUDE.md` | yes — Claude Code loads every `CLAUDE.md` from repo root down to the working dir | the vault | where am I, what exists, current state, what to read next |
| **Schema / spec** | `_schema/<kind>.md` | no (referenced) | any folder of that kind, in any vault | what a `<kind>/` folder *is* — its invariants, file-naming rule, what must never happen there |
| **Section marker** | `<folder>/_section.md` | no (referenced) | one section | "this folder is a section"; its lead doc; which `_schema/<kind>.md` govern its subfolders; local deviations |

If a `CLAUDE.md` and a schema disagree: the **schema wins** on section rules; the
`CLAUDE.md` wins on state and routing.

## Discovery rule

- A folder is the **schema set** if it is named `_schema/`.
- A folder is a **section** if it contains `_section.md`.
- The `_` prefix marks both as *structural*, not content.

See [`section.md`](section.md) for the `_section.md` shape and its fields.

## Adopting this system in a new vault

1. Copy `_schema/` in at the vault root.
2. For each top-level content folder, add a `_section.md` naming its `lead` doc and the
   `schemas` that govern its subfolders (`[]` if none). See [`section.md`](section.md).
3. Give the vault a root `CLAUDE.md` that routes here and carries a folder map. It should
   not restate any rule that lives in `_schema/`.
4. Schema-governed subfolders are created on first use — never pre-scaffolded.

Nothing in `_schema/` refers to anything outside `_schema/` except by generic
placeholder, so step 1 is a plain copy.

## Rulebook index

| Rulebook | Governs | Used in this vault? |
|---|---|---|
| [`section.md`](section.md) | the `_section.md` marker — how a folder declares itself a section | yes — all sections |
| [`updates.md`](updates.md) | `updates/` — dated change record for one code unit, `raw/` + `wiki/` split | yes — every code-unit section |
| [`knowledge.md`](knowledge.md) | `knowledge/` — stable reference material, `raw/` (verbatim) + `wiki/` (synthesis) | available, unused |
| [`decisions.md`](decisions.md) | `decisions/` — one dated note per settled non-obvious decision | available, unused |
| [`logs.md`](logs.md) | `logs/` — append-only record of automated maintenance passes | available, unused |
| [`inbox.md`](inbox.md) | `inbox/` — unprocessed drop zone, emptied as items are routed | available, unused |
| [`skills.md`](skills.md) | `skills/` — authored copies of a section's runnable procedures | available, unused |

[`skill-template.md`](skill-template.md) is the starting point for a new skill under
`skills.md`.

## How a section deviates from a rulebook

Add a `README.md` **inside the governed subfolder**, opening with:

```
Extends _schema/<kind>.md. Differences:
```

…and list only the deltas. Section-wide notes can instead go in the body of that
section's `_section.md`. The rule text lives in the folder it governs, so it travels if
the folder is ever moved or extracted.

Only add a **new** `_schema/<kind>.md` when the behaviour is a genuinely different kind of
thing another vault could reuse — not a one-off tweak.
