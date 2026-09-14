---
date: 2026-09-12
topics:
  - action-state-genericization
  - action-state-authority-placement
  - actions-as-stateless-logic-vs-payload
  - actions-component-as-push-pop-state-machine
tags:
  - discussion
---

# 2026-09-12 — Action State Architecture

## Summary

Design pass on where per-action state (uses remaining, cooldowns, availability) should
actually live. Surfaces a circularity: the Actions UI wants to read what's available from
`PlayerState` (so every client can see it cleanly), but `PlayerState` doesn't itself know
each action's live state — only the `ActionsComponent` and its action instances do. Storing
that state directly on the (client-reachable) `ActionsComponent` opens it to tampering;
storing it on `PlayerState` instead means treating it as replicated/read-only there, but
then something still has to keep it in sync with the source of truth. Works through a
reframe — actions are transient, high-level logic; their actual state is a payload separate
from the action itself — and tentatively lands on shrinking `ActionsComponent` to a
push/pop state machine over already-state-set action instances, with an Actions Menu UI
gating itself off immutable player-action-state data instead of the component managing
that gating.

## Topics

- **Action state genericization** — first time this came up
- **Action state authority placement (PlayerState vs. ActionsComponent, tampering)** — continues [2026-09-12 — Board Request Mediator & Player Action Config Design](2026-09-12-board-mediator-and-action-config-design.md)
  (that note's "action granting/revoking design" question — where should the check/state
  for granting or revoking a player's actions live — is the same question being worked
  through here from the state-authority angle)
- **Actions as stateless logic vs. state-as-payload** — first time this came up
- **ActionsComponent as a push/pop state machine** — first time this came up

## Questions

- Can per-action state be genericized into a couple of fixed shapes (permanent +
  turn-uses + cooldown; non-permanent + turn-uses + uses-available + cooldown), or does
  each action just need its own project-specific interface for its expected info/setup?
  Not decided.
- Where should the authoritative copy of an action's state (uses, cooldowns, availability)
  live so that every client can read it cleanly *and* it can't be tampered with? Storing
  it on the `ActionsComponent` (client-reachable, per-controller) risks tampering; storing
  it on `PlayerState` means treating it as replicated/read-only there — but explicitly
  unresolved: something still has to be the thing that updates `PlayerState` from
  whatever's actually computing the state.
- If `PlayerState` is the read side, what updates it? The `ActionsComponent`/its actions
  are what actually hold that state today — does that mean a server round trip is needed
  just to keep `PlayerState` current? Flagged directly as "a bad setup," not accepted as
  the answer.
- Should the `ActionsComponent` stop tracking live action state (uses, cooldowns) itself
  at all, and instead just receive requests to pop/push already-preconstructed,
  state-set action instances — making it, in effect, a generic action-focused state
  machine? Not settled, but the direction the reasoning leans toward.
- If an Actions Menu UI can gate itself entirely off immutable player-action-state data,
  does the `ActionsComponent` still need to own gating logic at all, or does it become
  redundant with the UI layer? Left open — the transcript's own closing thought treats
  keeping a thin `ActionsComponent` wrapper as possibly fine "for clarity and ease of
  use," not as a settled conclusion.

## Updates

- Tentative direction, not a firm decision: even after stripping away most of its
  state-tracking responsibility under this rethink, it may still be worth keeping
  `ActionsComponent` around as a thin, action-specific state-machine-like wrapper class —
  justified purely on clarity/ease-of-use grounds, since some kind of actions-menu
  gating mechanism will exist in any turn-based game this suite builds (Final
  Fantasy/Fire Emblem-style action menus were named as the reference shape).

## Transcript

Action Stats

* is it possible to genericize state for an action?
   * Permanent (always available) + turn uses, cooldowns
   * Non-permanent + turn uses + uses available + cooldown
* Or do we just create project specific interfaces to get the expected info and setup?

The real concern here is setting up the player state to track action state and have it not exposed unnecessarily.
Encountering a state issue when it comes to Actions. The ActionsComponent is tracking the state, but we want the player state to know about the current state of each action so all players can have access to that information. Just having it on the actions component of the local player controller is not ideal.
At the same time we run into the issue of tampering. If a players actions and their associated state are stored on the actions component there is opportunity to tamper. If on the player state, it becomes a read only and imitate setup situation locally.
We have a bit of a circle. The Actions UI (which the player uses to switch between place piece and swap piece) reads what actions (place, swap) are available for this player through the player state (we want it hear so that all clients can have access to this information cleanly?). But then how does the player state know what the state of each action is? The ActionsComponent and each action it holds contain that. So we need to send some server request to update it? This is a bad setup.
Something to consider is that Actions in theory are transient in nature in that are just logic to perform an action. How that action behaves is dictated by some state (which we can consider a payload). Actions themselves are actually high level concepts, not atomic. They should be directed at accomplishing a super specific task (action). How this happens is totally up to interpretation (internal state machine, etc.).
What if the actions component doesn't maintain a live set of actions that track their own set and cooldowns etc. What if the actions component simply receives requests to pop, push already preconstructed and state set actions? Would this just make it a glorified state machine (action machine?).
Realistically no matter what turn based game we make, there will always exist an actions menu (think final fantasy, fire emblem) which guards and gates what actions can be performed, and when. We don't actually need the Actions component to manage this. An actions menu UI reading from immutable player action state data will set itself up properly so that inappropriate actions cannot be selected or performed.
So where does this leave us with the current actions setup? It removes much of the responsibility but perhaps it is okay that we leave it as a state machine like wrapper class that is specific to actions for clarity and ease of use.
