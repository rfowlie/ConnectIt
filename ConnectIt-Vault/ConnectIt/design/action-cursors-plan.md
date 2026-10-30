---
type: plan
status: not-started
approved: true
approved_on: 2026-10-29
completed_on: 
started:
stages: 4
current_stage: 1: tile cursor state (plugin)
owner_done: false          # the owner's editor checks for this plan (a checkbox; separate from the code stages)
playtested: false
design: "[hover-cursor](hover-cursor.md)"
question:
decision:
---

# Plan: each player action owns its cursors (simple version)

Design: [hover-cursor](hover-cursor.md) ("Simpler for now", 2026-10-29). Builds on the finished
[hover-component-plan](hover-component-plan.md) (components → `UGridHoverSubsystem` → the actions, which already get
hover changed / ended / clicked). Written 2026-10-29 in a design-only session, **revised the same day** after the
owner's answers; **approved by the owner 2026-10-29**. Not built -- building happens in its own session.

## Context

Settled with the owner (2026-10-29): start **simple, expand later** -- the style tables, role tags, selection objects and
level overrides are **parked** (details unclear).

- **The standard: player actions control cursors.** Tiles never choose or move a cursor.
- **Tiles carry a `CursorState`** -- a **gameplay tag** (flexible: the options aren't all known yet) -- that caches which
  state they are in, and they respond when given a new one. **`SetCursorState` is its own call, explicit:** it does not
  also go through `SendGameplayTag`, so a tile knows it is being told a cursor state.
- **Each player action is its own cursor manager.** It is already tied into the hover subsystem and receives every
  hover / click call, so it handles the cursors itself -- there can be variable situations and constant cursor
  switching, and **each action decides how its follow cursor works** (possibly dynamically).
- **`AGridCursorManagerBase` stays for now, untouched, and the new path doesn't use it.** (Don't place both in a map: it
  would show a second cursor.)

What's there today (checked): the manager follows the hovered tile on its own and knows nothing of actions; Place, Swap
and Shift each drive tile looks with `Tile->SendGameplayTag(...)` (Swap re-sends a "first selection" tag to keep the
first-picked tile marked); `UTurnBasedPlayerAction` binds hover changed / ended / clicked on activate and unbinds on
deactivate; `UActorPoolSubsystem` exists.

Each stage builds and passes the tests before the next; the editor must be closed for builds; the owner commits between
stages.

## Stage 1: tile cursor state (plugin)

- On **`AGridTileBase`**: `FGameplayTag CursorState` (empty = neutral; Blueprint-readable) and
  **`SetCursorState(NewState)`** -- a no-op if unchanged; otherwise stores it and raises a Blueprint-native
  **`OnCursorStateChanged(Previous, Current)`** plus a delegate. `GetCursorState()`, `ClearCursorState()`.
- **It does not call `SendGameplayTag`.** `SendGameplayTag` stays as it is for anything else that uses it.
- A standard set of native tags in the plugin (`Grid.CursorState.Hovered`, `.Selected`, `.Valid`, `.Invalid`); an
  action may set any tag, so nothing is limited to the set.
- Tests: setting the same state raises nothing; a change raises one event with the right previous / current; clear →
  neutral.

## Stage 2: the action's cursor helpers (plugin)

On **`UTurnBasedPlayerAction`** (so every action has them), using the actor pool directly -- no manager:

- **Cursors from the pool:** `AcquireActionCursor(CursorClass) → AActor*` (from `UActorPoolSubsystem`, returned hidden
  and inactive, remembered by the action), `ReleaseActionCursor(Cursor)`, `ReleaseActionCursors()`.
- **The follow cursor** (the common case) as a slot: `ShowHoverCursor(CursorClass, Tile)`, `HideHoverCursor()`,
  `GetHoverCursor()`. If the class differs from the current one it swaps (release, acquire -- cheap with the pool), so an
  action can choose the class **per tile** -- e.g. Swap uses one cursor on your own tile and another on an opponent's.
- **Other cursors** (markers, previews): `AcquireActionCursor`, then `ShowCursorAt(Cursor, Tile)`, `HideCursor(Cursor)` --
  the action shows, hides and moves them itself. A `CursorOffset` property (default a few units up) places them.
- **Tile states:** `SetTileCursorState(Tile, State)` remembers the tiles the action touched.
- **Cleanup is automatic:** deactivate, force-deactivate and cancel call `ReleaseActionCursors()` and clear every
  touched tile to neutral -- an action that forgets can't leak a cursor or leave a tile stuck.
- Tests: the pool gets every cursor back after each exit path; touched tiles return to neutral; showing a different
  class swaps the hover cursor and returns the old one (a test world where needed).

## Stage 3: ConnectIt's actions adopt it

- **Place:** `ShowHoverCursor(HoverCursorClass, Tile)` on a valid hover, `HideHoverCursor()` when hover clears; tile
  states through `SetTileCursorState`.
- **Swap:** the hover cursor class chosen per tile (own piece / opponent's piece / invalid), plus a **marker cursor**
  (`AcquireActionCursor`) shown on the first pick and hidden when the selection clears -- **replacing the
  re-sent `TagFirstSelection`**.
- **Shift:** its hover through `SetTileCursorState`; no cursor of its own yet.
- Cursor classes are `EditDefaultsOnly` properties on each action; **none set = no cursor** (the tile state alone carries
  the look).
- Each action's `SendGameplayTag` calls become `SetTileCursorState`.
- **Lands together with Stage 4:** until the tile Blueprints handle `OnCursorStateChanged`, tiles show no hover /
  state looks (nothing forwards to the old tag path).
- Tests: Place and Swap acquire the cursors they declare and release them on every exit; Swap's marker follows the
  pick; the per-tile class choice swaps the hover cursor.

## Owner checks: content

- Cursor Blueprints (hover variants; a marker), set on the actions' defaults.
- **Tile Blueprints:** implement `OnCursorStateChanged` (the new explicit path) instead of reacting in the tag handler.
- Remove any `AGridCursorManagerBase` from maps using action cursors.

## Stage 5: vault

Decision note (each action is its own cursor manager; tiles cache a `CursorState` set explicitly); code pages for
`UnrealGridMechanics` and `UnrealTurnBasedMechanics`; a recipe "give a player action a cursor"; update
[hover-cursor](hover-cursor.md); log.

## Not in this plan (expand later)

- A cursor style table / role tags / selection objects, and level overrides.
- Tile-chosen cursors (the owner's rule: tiles don't choose cursors).
- A cursor when no action is active (e.g. the opponent's turn) -- there is none for now; a default could be added later
  (e.g. the manager, or a hover-only cursor component).
- A cursor for the AI's move; several overlapping states per tile (one at a time here); lerped cursor movement (snaps).
- Retiring `AGridCursorManagerBase` (kept for now).

## Verification

- Each stage: build (editor closed); `Automation RunTests ConnectIt` and the plugin tests pass.
- Owner, PIE: choosing Place shows its hover cursor following the mouse; switching to Swap returns it and shows Swap's
  (a different class on an opponent's tile); the first-picked tile keeps its marker while the mouse moves; cancelling or
  ending the turn leaves no cursor and no tile stuck in a state.

## Settled before approval (owner, 2026-10-29: as written)

1. A tile's `CursorState` is a **gameplay tag** with a native standard set (confirmed).
2. The cursor helpers live on the plugin's `UTurnBasedPlayerAction` (so every action in any game has them), rather
   than a separate helper class.
3. With no action active there is **no cursor** for now.
4. The Stage 3 / Stage 4 pairing: actions switch to `SetCursorState` and the tile Blueprints gain `OnCursorStateChanged`
   in the same session, since nothing forwards to the old path.
