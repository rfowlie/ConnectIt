# CLAUDE.md — UnrealGameMechanics

The reusable gameplay-scaffolding backbone the rest of the plugin suite builds on — no
game-specific rules. Its centrepiece is a gated **phase-barrier**:
`UGameEventTaskManager` holds a set of tasks that must all finish before it fires
`OnManagerComplete`; `UGameEventTaskSubsystem` (a `UWorldSubsystem`) adds a per-gameplay-tag
registry + serialized FIFO queue (`QueueTagContainer`) so callers fire tag-groups that run
in parallel and only advance when every task completes. Around it: turn bookkeeping, an
actor object pool, an explicit-lifetime Enhanced Input tag binder, generic
scoring/selection helpers, two state-machine patterns, a tag-broadcast component, and the
`UDWidgetBase` debug-widget base.

## Domain

- **Source:** `../Plugins/UnrealGameMechanics/Source/` — one Runtime module. Sub-areas:
  `GameEvent/`, `GameTurn/`, `Pooling/`, `Input/`, `Scoring/`, `State/`, `Broadcasters/`,
  `Widget/`, `Debug/`.
- **Status:** active — enabled in `ConnectIt.uproject`. Plugin dep: `EnhancedInput`.
- **Type:** game-agnostic plugin, independently extractable.
- **Base of the suite:** consumed by [[UnrealGridMechanics/CLAUDE|UnrealGridMechanics]],
  [[UnrealTurnBasedMechanics/CLAUDE|UnrealTurnBasedMechanics]], and
  [[ConnectIt/CLAUDE|ConnectIt]]. TurnBased's turn-end sequence routes through
  `UGameEventTaskSubsystem`.

## Sections

- **`code/`** — per-type pages, `code/systems/` flow narratives, `code/recipes/` task
  procedures, `code/index.md` (inventory of every public type + a module map). Governed by
  [[_core/_schema/_code|_core/_schema/_code.md]].
- `logs/` — domain-scoped maintenance passes, created on first use
  ([[_core/_schema/_logs|schema]]).

## Start here

[[UnrealGameMechanics/code/index|code/index.md]] — inventory + where to start reading.

## Known rough edges

- `BlueprintreadWrite` casing typo in `GameTurnHandler.h`.
- Orphaned `IGameEventTaskHandler`; retired `QueueTagSequence` referenced only in stale
  comments.
- Near-duplicate `UGameTurnParticipant` vs `UGameTurnParticipantComponent`; two
  overlapping state-base patterns (`UGameMechanicsStateBase` vs `…Simple`).
- `UStackedStateMachine::SwapState()` is an unimplemented stub; `PopState()` has a
  delegate-unbind gap; `PeakState` is a misspelling of "Peek".
- Inconsistent `Category` strings across UFUNCTIONs/UPROPERTYs.
