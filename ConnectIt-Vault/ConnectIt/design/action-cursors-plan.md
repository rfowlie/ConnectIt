---
type: plan
status: complete
approved: true
approved_on: 2026-10-29
completed_on: 2026-10-30
started: 2026-10-30
stages: 4
current_stage: "complete (code); owner content checks open"
owner_done: false          # the owner's editor checks for this plan (a checkbox; separate from the code stages)
playtested: false
design: "[hover-cursor](hover-cursor.md)"
question:
decision: "[2026-10-30-each-action-is-its-own-cursor-manager](../_decisions/2026-10-30-each-action-is-its-own-cursor-manager.md)"
---

# Plan: each player action owns its cursors (simple version)

Design: [hover-cursor](hover-cursor.md) ("Simpler for now", 2026-10-29). Builds on the finished
[hover-component-plan](hover-component-plan.md) (components → `UGridHoverSubsystem` → the actions, which already get
hover changed / ended / clicked). Written 2026-10-29 in a design-only session, **revised the same day** after the
owner's answers; **approved by the owner 2026-10-29**. **Built 2026-10-30** (all code stages in one go, at the
owner's request; 86/86 tests). Decision:
[each action is its own cursor manager](../_decisions/2026-10-30-each-action-is-its-own-cursor-manager.md); recipe:
[give a player action a cursor](../../UnrealTurnBasedMechanics/code/recipes/give-a-player-action-a-cursor.md). The
owner's content checks (below) are still open.

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
- **As built (2026-10-30):**
  - `AGridTileBase`:
    - `SetCursorState`, `ClearCursorState`, `GetCursorState` (the state itself is private; read it with the getter);
    - the protected BlueprintNativeEvent `OnCursorStateChanged(Previous, Current)`, then the
      `OnTileCursorStateChanged(Tile, Previous, Current)` delegate;
    - `SendGameplayTag` is untouched.
  - The tags are in `Tile/GridCursorStateTags.h`: `Grid.CursorState.Hovered / Selected / Valid / Invalid`.
  - Test: `UnrealGridMechanics.Tile.CursorState`.

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
- **As built (2026-10-30):**
  - All the helpers are public `BlueprintCallable`, as named above, plus `HoldsActionCursor(Cursor)`: a kept pointer is
    stale once released.
  - `CursorOffset` defaults to (0, 0, 5).
  - Cleanup runs in `FinishAction` (complete and cancel) and in `ForceDeactivate_Internal`; an override of that must
    call Super.
  - **Added beyond the plan:** a `HandleInvalidHover(Tile)` hook. The hover flow calls it, after the previous tile's
    `HandleHoverCleared`, when the cursor moves onto a tile `IsValidHoverTile` refuses. Swap's "invalid" cursor
    needed it, because before this only valid tiles reached an action.
  - **Revised (owner, 2026-10-30): every cursor implements `IGridCursor`** (grid plugin `Cursor/`), and it's
    mandatory with no fallback:
    - `AcquireActionCursor` refuses any other class with an error, and ConnectIt's cursor class properties carry
      `MustImplement`;
    - the interface is `MoveCursor(Tile, Location, bReactivate)` (the follow cursor reactivates when it lands on a
      different tile; Swap's marker always does), `ShowCursor` and `HideCursor`;
    - the pool's activate / deactivate is now "in use" (on acquire / release), and Show / Hide is "visible while held".
  - Test: `ConnectIt.PlayerAction.Cursors`. It uses a test action (ConnectIt's tests, since the plugin has no test
    folder) and covers the per-tile class swap, a marker, and cancel / complete / force each returning everything.

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
- **As built (2026-10-30):**
  - **Kept tags:** `TagActionMouseHover` and Swap's `TagFirstSelection` stay as the cursor-state values. Their
    Blueprint values (`ConnectIt.Tile.ValidHover`, ...) carry over, so the tile Blueprint can switch on the same tags
    in `OnCursorStateChanged`. The C++ defaults are now `Grid.CursorState.Hovered` / `.Selected`.
  - **Removed:** `TagActionGridState` on all three actions, because neutral is now no tag. Its Blueprint values are
    dropped on load.
  - **Untouched:** Place's unused `TagActionMouseHoverInvalid`.
  - **Place:** `HoverCursorClass`.
  - **Swap:**
    - follow-cursor classes `OwnPieceCursorClass` / `OpponentPieceCursorClass` (by the hovered tile's piece) and
      `InvalidCursorClass` (`HandleInvalidHover`), plus `MarkerCursorClass` on the first pick;
    - the first-picked tile keeps its Selected state while hover moves on, because nothing overwrites it, so nothing
      is re-sent;
    - `ClearFirstSelection` unmarks it on the second pick and on clear.
  - **Shift:** the hovered line's tiles through `SetTileCursorState`.
  - **Tests:** `ConnectIt.PlayerAction.CursorsAdopted` covers Place and Swap returning cursors and tile states on
    complete, cancel and force. Their hover-driven paths need a board, so they're an owner PIE check (Verification).

## Owner checks: content

- Cursor Blueprints (hover variants; a marker), set on the actions' defaults.
- **Tile Blueprints:** implement `OnCursorStateChanged` (the new explicit path) instead of reacting in the tag handler.
- Remove any `AGridCursorManagerBase` from maps using action cursors.

## Stage 5: vault

Decision note (each action is its own cursor manager; tiles cache a `CursorState` set explicitly); code pages for
`UnrealGridMechanics` and `UnrealTurnBasedMechanics`; a recipe "give a player action a cursor"; update
[hover-cursor](hover-cursor.md); log.

- **As built (2026-10-30):**
  - the decision note;
  - new `UnrealGridMechanics/code/AGridTileBase.md`;
  - `UTurnBasedPlayerAction.md` and `hover-relay.md` updated;
  - recipe `UnrealTurnBasedMechanics/code/recipes/give-a-player-action-a-cursor.md`;
  - [hover-cursor](hover-cursor.md) marked built;
  - log `_logs/2026-10-30-1026.md`.

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
