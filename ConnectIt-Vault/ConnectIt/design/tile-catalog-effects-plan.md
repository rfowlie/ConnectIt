---
type: plan
status: not-started
approved: true
approved-on: 2026-10-28
completed-on:
playtested: false
design: "[editor-notes-2026-10-28](editor-notes-2026-10-28.md)"
question: "[tile-catalog-actor-matching](../_questions/tile-catalog-actor-matching.md)"
decision:
---

# Plan: the tile catalog ties each power tag to its effect

From [editor-notes-2026-10-28](editor-notes-2026-10-28.md), item 5 (rounds 3-4, 2026-10-28). Written 2026-10-28 in a
design-only session; **approved by the owner 2026-10-28**. Not built.

## Context

Builders hold a **power tag**; the tag is looked up in the **tile catalog** (`UConnectIt_TileCatalogDataAsset`, a
`TMap<FGameplayTag, FConnectItTileCatalogEntry>`; an entry = tile class + `FConnectItTilePower`). Problems: the power's
`Name` tag repeats the map key (they can disagree), and every entry shows every field (`FireAtMultiplier`,
`MultiplierFloor`, `MultiplierCeiling`...) whether or not the power uses it. Decided:

- **Each tag is automatically tied to its effect** -- the C++ effect declares its tag.
- **Only values relevant to a power are exposed** -- they live on the effect.
- **The trigger mask stays exposed** on the entry for now (playtesting).
- Matching tile classes to powers: a question for later.

## Stage 1: effects own their tag and values

- `FConnectItTilePowerEffect` gains `virtual FGameplayTag GetPowerTag() const` -- each effect returns its native tag
  (`ConnectIt.TilePower.Push` etc., already defined in C++).
- **Values move onto the effects that use them.** The multiplier gate (today `FConnectItTilePower::FireAtMultiplier`,
  read by `ConnectIt_TilePowerChain.cpp`) becomes a reusable `FConnectItMultiplierGate` property on the effects that
  want one; the chain asks the effect (`virtual bool PassesGate(Context)`, default true). `MultiplierFloor` /
  `MultiplierCeiling` go where something reads them, or are removed if nothing does (they look unread today).
- `FConnectItTilePower` keeps **`Triggers`** (exposed, as asked), **`CooldownTurns`** (generic) and **`Effect`**; its
  **`Name`** is removed -- the tag is the effect's.
- Tests: each effect reports its tag; the gate on an effect stops / allows firing as before.

## Stage 2: the catalog keys by the effect's tag

- The catalog's storage becomes an **array of entries** (tile class + power); the tag → entry lookup is **built from
  each entry's effect tag** when the catalog loads, so the key can't disagree with the effect. Validation: two entries
  with the same effect tag; an entry with no effect.
- `GetTileClassFor(Tag)` / the rules' power lookup unchanged for callers.
- Builders keep their **power tag** (picker limited to `ConnectIt.TilePower`).
- Existing catalogs: their entries are set again (owner, editor) -- or a one-off load fix-up copying map entries into the
  array.
- Tests: lookup by tag; duplicate tags reported.

## Stage 3: vault

Decision note (effects own their tag and values; the catalog keys by effect); update the tile-powers recipe and the
tile catalog code page; log.

## Not in this plan

- Tying a tile class to one power ([question](../_questions/tile-catalog-actor-matching.md)).
- Hiding the trigger mask (kept for playtesting).

## Verification

- Build (editor closed); `Automation RunTests ConnectIt` passes.
- Owner, editor: a catalog entry shows only Push's own values for Push; Vacuum's for Vacuum; PIE: powers fire as
  before.

## Settled before approval (owner, 2026-10-28: as written)

1. The catalog's storage changes from a map to an array (the lookup built from effect tags).
2. Unread `MultiplierFloor` / `MultiplierCeiling` are removed unless a power needs them.
