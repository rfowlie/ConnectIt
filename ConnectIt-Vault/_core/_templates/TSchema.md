# _schema/_<section>.md

One or two sentences: what a `<section>/` folder is and what belongs in it — light
enough that a reader knows in ten seconds whether their thing goes here.

## Structure

A map of the folder this schema governs — a fenced tree or a table of its files /
subfolders and what each holds. Every schema has this; it makes the rulebook double as
the folder's MOC. (If there's no structure to draw, this probably isn't a schema — it's
a note template that belongs in `_core/_templates/`.)

## Where it mounts

Root, domain, or either. Delete this section if it only ever mounts in one place.

## What a note holds

Bulleted list of the parts of a note in this section — frontmatter and body. Delete if
the section defines a folder layout rather than notes of a fixed shape.

## Rules

- The invariants — what must always be true, what must never happen here.
- Created on first use; don't pre-scaffold.
- How this section relates to its neighbours (what goes here vs. the adjacent section).

## Template

`[[TSchema<Section>]]` — or, if the section has no fixed note shape, one line saying so
and why.

## Deviations

Add `<section>/__README.md` starting `Extends _schema/_<section>.md. Differences:` if a
domain needs this section to behave differently. List only the deltas.

---

Checklist before committing a new schema file:

- [ ] Paths are generic (`<domain>/…`) — no reference to a specific vault or domain.
- [ ] H1 is the short label `# _schema/_<section>.md` (files live in `_core/_schema/`).
- [ ] Has a `## Structure` map, a `## Template` section, and a `## Deviations` section.
- [ ] It governs a file structure — not just a note shape (that's a `T*` template).
- [ ] Added to the section catalogue in `_core/_schema/__README.md`.
- [ ] `ABOUT.md` section table + Changelog updated.
