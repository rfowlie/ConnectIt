# _schema/_atomic.md

An `atomic/` folder is a **flat pool of single-idea notes** — the Zettelkasten building
block. Each note captures exactly one concept, is fully self-contained, and is free to
link to any other note regardless of topic.

## Where it mounts

**Exactly one atomic pool per vault, and it is `_core/_atomic/`.** Never per-domain,
never at another path. Ideas distilled anywhere in the vault go into this one folder so
unrelated notes can mingle and spark connections.

> **Warn on multiples.** If a second `atomic/` (or `_atomic/`) folder appears anywhere in
> the vault, that's an error — merge its notes into `_core/_atomic/` and delete it.
> Don't silently maintain two pools. `/process-atomic` runs this check and fix.

## Structure

```
_core/_atomic/
  __README.md               what this folder is
  about-atomic-notes.md   the editable "how this vault writes atomic notes" reference
  mental-model.md         anchor note for notes tagged `mental model`
  <kebab-slug>.md         one note per idea, named for the idea (not its source)
```

Flat — no topic subfolders.

## What a note holds

- **Frontmatter** — `created:`, a `status:` of `baby` | `child` | `adult` (how developed
  and polished the note is), and a few high-level topic `tags:`.
- **Body** — a mini-essay in your own words with a clear start, middle, and end. Under
  ~500 words; it should fit on screen without scrolling.
- **`## References`** — a link back to the source note the idea came from, plus links to
  related atomic notes.

## Rules

- **One idea per note.** The moment you're adding a second concept, that's a second note.
- **Self-contained.** It must re-inform you months later with no other note or outside
  context required — never "see X for background".
- **Your own words.** Don't paste from the source; rewriting is what makes the idea land.
- **Link heavily, and always link back to the source.** An orphan atomic note is a smell.
- **Flat forever.** Never file atomic notes into category folders. Curated entry points
  live in `_maps`, not in the directory tree.
- **Comes from one of three places** — a fleshed-out source insight, a recurring theme
  spotted across several sources, or an original thought (no source needed).
- **Named mental models** (the 80/20 rule, flow, the Wundt curve, inversion) are atomic
  notes tagged `mental model`, each linking `[[mental-model]]`. There is no separate
  frameworks section.
- Created on first use; don't pre-populate.

### Versus its neighbours

- `_data/wiki/` is **per source** — synthesis tied to one origin. `_atomic` is **per
  idea** and source-independent; a `wiki/` insight worth pulling out becomes an atomic
  note that links back.
- `_maps` curates reading paths across atomic notes — the index layer on top of the flat
  pool.

## Template

[[TSchemaAtomic]]

## Reference

[[how this vault writes atomic notes]] — the fuller explainer, meant to be edited per-vault to tune how
atomic notes behave.

## Deviations

Add `_core/_atomic/__README.md` differences inline, or a note in that `__README` — there
is only one atomic folder, so the usual per-domain deviation pattern doesn't apply.
