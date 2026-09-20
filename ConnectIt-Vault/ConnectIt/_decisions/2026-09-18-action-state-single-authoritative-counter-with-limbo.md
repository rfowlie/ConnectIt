---
Date: 2026-09-18
status: Active
superseded by:
tags:
  - action-state
  - playerstate
  - turn-end
  - replication
---
## Decision

Per-action use counts have **one authoritative home: the plugin's `ATurnBasedPlayerState`**
(not `AConnectIt_PlayerState`), replacing the action instances' own `CompletionsThisTurn` /
`TurnsUntilAvailable`. To handle the fact that state replicates after the server confirms a
request, the actions component adds a **limbo state**:

1. Server confirms a request → the action **completes as it does today** (so the player can't
   fire off repeat requests by accident).
2. The component immediately enters limbo — stack mutation and new pushes stay blocked.
3. The only exit is `PlayerState::OnActionRuntimeStateUpdated` (server: fired locally when the
   state changes; clients: from the `OnRep`). On it the component leaves limbo, reads the
   updated state, and runs the auto-end-turn check.

Ordering guard: the pre-request `UsesThisTurn` is recorded at send time; if the state has
already advanced when limbo is entered (update arrived before the outcome RPC) the component
exits immediately. A timeout/failure path keeps a lost update from freezing the turn.

Related shape decisions made in the same pass:

- `FTurnActionRequest` gains `ActionTag`, stamped by `UTurnBasedAction::RequestBoardChange`
  so an action can't forget it; the server resolves the requester's state entry from it.
  It is client-supplied, so each Mediator handler must still assert its `RequestType` is one
  that action may produce.
- PlayerState holds the loadout reference (replicated as an asset ref) plus arrays of
  `FPermanentActionRuntimeEntry` / `FNumberedActionRuntimeEntry` (class + the owner's state
  struct); a `TMap` can't replicate.
- Server API on PlayerState: `InitialiseActionState`, `CanUseAction`, `ConsumeActionUse`,
  `GrantActionUses` (clamped to `MaxHeldUses`), `ResetActionTurnCounters`,
  `TickActionCooldowns`.

Supersedes the "runtime state on PlayerState, client tree reads action instances" implication
in [2026-09-18 — Generic turn-end requirement system](2026-09-18-generic-turn-end-requirement-system.md).

## Why

The owner preferred a single fully-authoritative counter over two counters (a client
prediction plus a server truth). The catch — auto-end-turn is evaluated client-side right
after an action completes, before replication — is answered by the game's own flow: every
action already produces a visual update that only starts once the server returns the updated
board (and therefore PlayerState), and players shouldn't act during it. Limbo formalises
that pause instead of racing it. Completing the action first (rather than holding it open)
was the owner's correction: it closes the window in which a player could spam requests.

## What Would Change It

If a game's actions can complete without any server-side state change, nothing would ever
fire the delegate and limbo would only exit via its timeout — such actions would need to
consume a use (even a zero-cost one) or bypass limbo explicitly. Every action in this game
consumes a use.
