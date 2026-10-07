---
Date: 2026-10-02
status: Active
superseded by:
tags:
  - ai
  - turn-based
  - architecture
---
## Decision

AI controllers have **no actions component and no coordinator**. `AConnectIt_AIController` runs its own turn: seeds its
PlayerState from the AI profile's loadout, binds `ParticipantComponent->OnTurnNotificationReceived_Native` (decides once
per turn number; cancels on turn end/timeout/skip/game over) and `GameState->OnMatchPhaseChanged_Native` (cancels on
pause, re-decides on resume), and ends its turn with `ATurnBasedPlayerState::CanEndTurn()` +
`ParticipantComponent->ServerSubmitTurnEnd()`. The plugin's `ATurnBasedAIController` stopped creating those two
components. Turn-end requirements are now evaluated in one place, `ATurnBasedPlayerState::AreTurnEndRequirementsMet`,
which the human actions component also calls (behaviour unchanged).

## Why

The actions component is a human's input-driven action stack; the AI used four small things from it and forced
human-only setup on enemy loadouts (mandatory `RootActionClass`, viewer actions) plus a component-subclass trick for the
turn hook. Of the three options discussed (keep it / a slim plugin AI component / controller does it), the owner chose
the controller doing it: fewest moving parts, all AI turn handling in one file. Accepted cost: this plumbing lives in
ConnectIt, not the plugin.

## What Would Change It

A second game (or several AI controller types) needing the same plumbing -- then lift it into a slim plugin component.
