# devlog/

Dated records of notable decisions and work sessions on ConnectIt — the "why we did it
this way" log that doesn't belong to any single plugin.

This section's marker is [`_section.md`](_section.md) (`schemas: []` — the entry format
below is a local convention, not a schema).

## What goes here

- Design decisions and the reasoning behind them (and options rejected).
- Session notes: what was attempted, what landed, what's still open.
- Cross-cutting changes that touch several plugins at once.

Plugin-specific change logs go in that plugin's `updates/` folder instead — see
[`../_schema/updates.md`](../_schema/updates.md). Use `devlog/` when the entry is broader
than one plugin or is about direction rather than a specific code change.

## Convention

One file per entry, named `YYYY-MM-DD-<slug>.md`. Suggested shape:

```markdown
# <YYYY-MM-DD> — <title>

## Context
What prompted this.

## Decision / what happened
...

## Consequences
What changes as a result; follow-ups.

## Links
Commits, PRs, related notes.
```
