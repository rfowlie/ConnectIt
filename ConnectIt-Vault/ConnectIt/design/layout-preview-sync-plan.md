---
type: plan
status: not-started
approved: true
approved-on: 2026-10-28
completed-on:
playtested: false
design: "[editor-notes-2026-10-28](editor-notes-2026-10-28.md)"
question:
decision:
---

# Plan: layout tool -- moving a preview moves its builder

From [editor-notes-2026-10-28](editor-notes-2026-10-28.md), item 6 (decided 2026-10-28: add this behaviour). Written
2026-10-28 in a design-only session; **approved by the owner 2026-10-28**. Not built.

## Context

`UConnectIt_LayoutToolLibrary::PreviewTileCatalog` spawns, for each builder tile, a **transient** catalog tile at the
builder's transform (hidden from the outliner, tagged as a preview) and hides the builder; nothing links the two, so
moving a preview changes nothing and the owner has to flip back to the builders. Decided: **moving a preview moves its
builder.**

## Stage 1: link and sync (editor only)

- Each preview remembers its builder (a weak pointer on the preview, set at spawn -- a small editor-only component or a
  map kept by the tool, editor-only).
- **On an editor move of a preview** (`GEditor` actor-moved notification, `WITH_EDITOR`): snap the preview to the grid
  cell under it (the grid definition), move its builder to the same transform, keep the builder hidden.
- **Deleting a preview** in the editor: leave the builder (only Clear removes previews); a log line.
- Saving stays safe: previews are transient. As a safety net, previews are cleared before a save (the editor's
  pre-save world hook), and restored only by pressing Preview again.
- **Show previews in the outliner** (needed to select them by name) -- or keep them hidden and rely on viewport
  selection (to decide in the build).
- Tests: none (editor-only); checked in the editor.

## Stage 2: vault

Layout tool recipe / code page; log.

## Verification

- Owner, editor: Preview → drag a preview tile two cells → Clear → its builder is at the new cell; save during preview
  → no preview tiles in the saved map.

## Settled before approval (owner, 2026-10-28: as written)

1. Previews snap to the grid cell when moved.
2. Previews are cleared automatically before a save.
