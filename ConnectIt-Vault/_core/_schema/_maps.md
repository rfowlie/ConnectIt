# _schema/_maps.md

A `maps/` folder holds **hand-curated Maps of Content** — index notes that are the
deliberate entry points into a body of notes. A map is a reading path someone chose, not
a generated list.

## Filename

`maps/<kebab-slug>.md` — e.g. `maps/game-feel.md`, `maps/pricing.md`.

## What a note holds

- A short framing of the topic and why this map exists.
- Curated links to the notes that matter, grouped and ordered by the curator's judgement.
- Brief connective prose between links — how the pieces relate, where to start.

## Rules

- **Curated, not exhaustive.** A map points to the notes worth reading, in a useful
  order. It is not a dump of everything tagged with the topic.
- **Hand-written.** Maintained by a person as understanding develops. (Obsidian Bases was
  considered for this and not adopted — a mechanical query is a different tool for a
  different job.)
- A map can link to other maps. Deep areas get a top-level map that routes to sub-maps.

## Template

[[TSchemaMaps]]

## Deviations

Add `maps/__README.md` starting `Extends _schema/_maps.md. Differences:` if a domain wants
a fixed section order or a required "start here" map.
