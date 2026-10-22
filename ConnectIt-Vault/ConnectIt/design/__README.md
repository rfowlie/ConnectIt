# ConnectIt/design/

Game design notes: how actions and systems are meant to work, feature sketches, open design questions -- and the
**plans** that turn them into code. Deliberately schema-less (see the domain [CLAUDE.md](../CLAUDE.md)); the one
convention is for plans.

## Plans

A plan is a design note whose title starts `# Plan:`, usually named `<topic>-plan.md`. Every plan starts with these
properties (Obsidian frontmatter), so plans can be listed and filtered by state:

```yaml
---
type: plan
status: draft          # draft | not-started | in-progress | complete | suspended | dropped
approved: false        # true once the owner approves it
approved-on:           # YYYY-MM-DD
completed-on:          # YYYY-MM-DD, once every stage is done
playtested: false      # true once the owner has checked it in PIE
design:                # "[note](note.md)" -- the design note it builds
question:              # "[note](../_questions/note.md)" -- the discussion behind it, if any
decision:              # "[note](../_decisions/note.md)" -- the decision it produced, once built
requires:              # "[plan](other-plan.md)" -- a plan that must be built first (or a YAML list of them); omit if none
---
```

- **status**: `draft` while being written or discussed; `not-started` once written (approved or not); `in-progress`
  from the first stage built; `complete` when every stage is done; `suspended` paused on purpose; `dropped` abandoned
  (keep the note, say why in it).
- **approved** is separate from status: a plan can be approved and not started.
- **requires** names prerequisite plans; say in the body which stages wait for them (often only some do).
- **playtested** tracks the PIE check that "built" plans used to note in prose ("PIE pending").
- Whoever changes a plan's state updates these properties in the same edit (a status line in the body may stay as the
  human-readable summary).
- Links use the vault's standard Markdown form, quoted so YAML keeps them as text.

Older plan-shaped notes that predate this (e.g. `board-events.md`, the `board-operations` revisions) carry their state in a
`## Status` section; add the properties when one of them is next touched.
