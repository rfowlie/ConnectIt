# _schema/_people.md

A `_people/` folder holds **one note per person** the owner deals with — a lightweight
CRM. It's the durable record of who someone is and the history of working with them.

## Filename

`_people/<kebab-name>.md` — e.g. `_people/jordan-mensah.md`. Disambiguate collisions with a
suffix (`_people/alex-kim-designer.md`).

## What a note holds

- Who they are, the relationship, how you know them.
- Standing context — role, what they care about, how they like to work.
- A dated, append-only log of notable interactions.
- Links out to `_meetings/` notes and to any project domain they're part of.

## Rules

- **Every mention of a person elsewhere in the vault links to their `_people/` note.**
- One person per file. Organisations get their own note; link members to it.
- Created on first use — don't seed a directory of people you haven't worked with.

## Template

[[TSchemaPeople]]

## Deviations

Add `_people/__README.md` starting `Extends _schema/_people.md. Differences:` if a domain
tracks extra fields (e.g. deal stage, retainer terms).
