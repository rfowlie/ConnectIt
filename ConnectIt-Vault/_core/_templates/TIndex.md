# <domain or _core>/<section>/

<One or two sentences: what this file tracks, and its ordering rule — newest-first,
alphabetical, or by-status.>

Governed by [`_schema/_<section>.md`](<relative path to the schema file>).

<Ordering label, e.g. "Newest first:">

- [[<note>]] — <one-line summary>

<!--
Base shape: a flat linked list, one line per item, in the section's stated order. A
section's own schema may specialize this into a richer structure when the section calls
for it — e.g. _schema/_code.md's __INDEX.md (inventory table + codebase map) or
_schema/_decisions.md's dated table. A specialized index still satisfies this contract;
it does not need to match this list shape verbatim. Renaming an existing specialized
index to __INDEX.md doesn't mean rewriting it to this generic list shape.
-->
