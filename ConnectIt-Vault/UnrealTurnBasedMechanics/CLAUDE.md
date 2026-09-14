# CLAUDE.md — UnrealTurnBasedMechanics

A network-replicated turn-based match framework with no knowledge of grids or game rules.
Each controller's moment-to-moment behaviour is an entry on an **action stack**
(`UTurnBasedActionsComponent`) fed from a designer `UActionLoadoutDataAsset` with five
system slots (root / idle / spectator / pause / awaiting-confirmation). `UTurnBasedAction`
is the per-turn action, with cooldowns and an Enhanced-Input grid-tile
hover→validate→select pipeline. `UTurnBasedParticipantManagerComponent` (on the game
state) is the replicated match state machine: ready-check, turn timers,
forfeit-on-disconnect, reconnect handling, and a **gameplay-tag-gated turn-end sequence**
routed through [UnrealGameMechanics](../UnrealGameMechanics/CLAUDE.md)'
`UGameEventTaskSubsystem`. Turn order is a pluggable strategy. A coordinator component
wires the participant + actions components identically onto the player and AI controllers.

## Domain

- **Source:** `../Plugins/UnrealTurnBasedMechanics/Source/` — one Runtime module
  (`LogTurnBasedMechanics`). Sub-areas: `Action/`, `Action/Spectator/`, `Framework/`
  (Controller / GameMode / GameState / PlayerState), `Turn/Order/`, `Turn/Participant/`,
  `GameEvent/`, `Debug/`.
- **Status:** active — enabled. The most disciplined plugin (consistent dual dynamic +
  `_Native` delegates, `_Internal` wrapper pattern, `Turn Based|` Category prefix). Deps:
  `EnhancedInput`, [UnrealGameMechanics](../UnrealGameMechanics/CLAUDE.md),
  [UnrealGridMechanics](../UnrealGridMechanics/CLAUDE.md).
- **Type:** game-agnostic plugin, independently extractable.
- **Consumed by:** [ConnectIt](../ConnectIt/CLAUDE.md) — its GameMode / GameState / controllers
  derive from the `ATurnBased*` bases.

## Sections

- **`code/`** — per-type pages, `code/systems/`, `code/recipes/`, `code/__INDEX.md`.
  Governed by [`_core/_schema/_code.md`](../_core/_schema/_code.md).
- `logs/` — created on first use ([schema](../_core/_schema/_logs.md)).

## Start here

[code/__INDEX.md](code/__INDEX.md).

## Known rough edges

- `.Build.cs` lists grid / game-mechanics in both Public and Private dependency lists
  (redundant).
- Likely-broken source resolution in
  `UDWidget_TurnBasedParticipantManagerComponent::BindDelegates`.
- History to keep straight: turn-end gating moved from a fixed-duration
  `TurnResolutionDuration` timer → a `BeginResolutionHold`/`EndResolutionHold` counter →
  the current `TurnEndEventTag` + `QueueTagContainer` gate. **The counter and the timer
  are both gone** — any doc/diagram mentioning a "resolution hold" describes retired code.
