---
Date: 2026-09-14
status: Active
superseded by:
tags:
  - actions-ui
  - hud
  - root-action
---

## Decision

The Actions selection UI is owned and shown by `AConnectIt_HUD`, not by a "Root" action
instance on the action stack. The HUD is hooked to `OnTurnStart`, checks whether it's the
local player's turn, and shows/hides the UI based on that — hidden on the opponent's turn,
so there's no risk of a local player seeing or interacting with UI meant for the other
side. The UI persists across action selection: selecting an action highlights and disables
that action's button (reverting to normal when a different action is selected) rather than
the whole menu disappearing.

This supersedes the "Create Root action class which shows the actions UI" task as
originally scoped — a root/do-nothing action was going to be pushed onto the stack to
display the UI, with the real action pushed over it on selection.

## Why

The root-action approach was tried conceptually and rejected: if the do-nothing/root
action is what displays the UI, the UI disappears the moment the stack switches to the
selected action (the root action is no longer active), which is not the desired behavior —
the menu needs to stay visible and just reflect which action is currently selected.

Owning the UI in the HUD sidesteps the problem entirely: the UI's visibility is driven by
turn state (whose turn is it) rather than by which action happens to be active on the
stack, so switching the active action no longer implies hiding the thing that let you pick
it. It also matches how the HUD already gates other local-only display concerns.

## What Would Change It

- The existing task "Create Root action class which shows the actions UI" needs updating
  or replacing — it's no longer the right description of the work (still needs a
  hand-authored task update; see the 2026-09-14 check-in meeting note's task candidates).
- The Actions UI still hardcodes its Place Piece / Swap Piece buttons rather than reading
  the player's real available actions from `PlayerState` — a separate, already-tracked
  task, unaffected by this decision but worth doing once the action-state-on-PlayerState
  shape (see [2026-09-14 — Action state on PlayerState, ActionsComponent as builder](2026-09-14-action-state-on-playerstate-actionscomponent-as-builder.md))
  is built out.
- If a future UI need genuinely requires the action stack itself to drive visibility (not
  just turn state), this would need revisiting — no such case is known today.
