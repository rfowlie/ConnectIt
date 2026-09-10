---
schema: code
kind: <UCLASS | USTRUCT | UENUM | UINTERFACE | class>
role: <primary | internal>
source:
  - <repo-relative path to .h>
  - <repo-relative path to .cpp>          # omit if header-only
reconciled: <YYYY-MM-DD>                  # date a human last checked the page vs the code
commit: <short SHA — last commit to touch the source: paths (git log -1 --format=%h -- …)>
---

# <TypeName>

## What it is

1–2 sentences.

## When you touch this

The situations that bring a developer to this type.

## Entry points

The handful of members you call or override — not a full list.

## Collaborators

What it talks to; delegates it fires / consumes; `[[wikilinks]]` to other `code/` pages
and `systems/` flows.

## Gotchas

Authority/replication, ordering, safe-vs-raw delegate surfaces, other footguns.

## Cross-impact

Concrete "change this → also update <files/types>".

## Changes

Dated bullets, newest first — the change history for this type. Append only.

- <YYYY-MM-DD> — <what changed, and why it matters>

## See also

In-repo `Docs/` anchor, related `systems/` / `recipes/` pages.
