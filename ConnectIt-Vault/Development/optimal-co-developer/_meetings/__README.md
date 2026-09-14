# optimal-co-developer/_meetings/

One dated note per session or conversation with the owner worth recording as a meeting —
not the routine session bookends themselves.

Governed by [`_schema/_meetings.md`](../../../_core/_schema/_meetings.md).

Extends `_schema/_meetings.md`. Differences: a note produced by
[`meeting-open`](../_skills/meeting-open.md) / [`meeting-closed`](../_skills/meeting-closed.md)
adds `## Overview`, `## Key Points`, `## Insights`, and a verbatim `## Transcript` section
(never edited after writing, same rule as `_discussions/`'s transcript section) on top of
the base template's `## Actions` / `## Decisions` / `## Next`. `attendees:` for these notes
is always `Owner` and `Co-developer` as plain strings, not `_people/` links — this is an
internal check-in, not a call with a third party the `_people/` CRM is for.
