---
schema: systems
spans:                   # the source paths / types this flow crosses
  - <repo-relative path>
reconciled: <YYYY-MM-DD>
commit: <short SHA>
status: current                 # current | retired
summary: <one line: what the flow does -- shown in the Dataview index>
---

# <flow name>

## What happens

The flow in 3–6 sentences.

## Diagram

```mermaid
%% sequence, stateDiagram, or flowchart as fits — required
```

## Steps

The ordered walkthrough. Each hop names the type + member, `[[wikilinked]]` to its
`code/` page.

## Gotchas

Failure modes, timing / authority constraints.

## Cross-impact

What else must change if this flow changes.

## See also

In-repo `Docs/`, related `code/` / `recipes/` pages.
