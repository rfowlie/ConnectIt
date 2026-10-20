---
Date: 2026-09-18
status: Active
superseded by:
tags:
  - architecture
  - gameevent
  - unrealgamemechanics
---
## Decision

`UGameEventTaskSubsystem` ([[UnrealGameMechanics/code/UGameEventTaskSubsystem|code page]])
gained `OnAnyTagComplete` — a `BlueprintAssignable` multicast delegate that fires for
**every** tag's completion, not one specific tag. Broadcast from inside the existing
`HandleOnManagerComplete` (one line, additive — doesn't touch the per-tag
`OnManagerComplete` delegate `BindOnTagComplete` binds to, and doesn't affect either of
the two existing `BindOnTagComplete` callers,
[[AConnectIt_GameMode|AConnectIt_GameMode]]'s `PlayerWin` binding and
`UTurnBasedParticipantManagerComponent`'s turn-end binding).

## Why

The registry-mapping fix
([2026-09-18 — Registry mapping refresh on GameEvent complete](2026-09-18-registry-mapping-refresh-on-game-event-complete.md))
originally called for three separate `UFUNCTION` handlers on `UConnectIt_PieceRegistry`,
each bound to one tag via its own `BindOnTagComplete` call. Flagged during implementation
planning as needless boilerplate: `HandleOnManagerComplete` already gets notified of
every tag's completion internally regardless of which tag, so exposing that as one
generic delegate means a listener that dispatches by tag itself needs one bind + one
handler, not a new function-and-binding pair every time a new tag needs watching.
`BindOnTagComplete` stays the right tool for a listener that genuinely only ever cares
about one tag — this doesn't replace it, it adds the other shape.

## What Would Change It

`AConnectIt_GameMode`'s and `UTurnBasedParticipantManagerComponent`'s existing
`BindOnTagComplete` bindings were deliberately **not** migrated to `OnAnyTagComplete` —
each cares about exactly one tag, so migrating would only add tag-filtering logic they
don't need today. Revisit only if either ever needs to react to more than its one tag.
