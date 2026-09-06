# Architecture overview

> Living doc. Seeded from the plugin exploration on 2026-09-06; keep it in sync with
> `../../Plugins/` and `../../Source/ConnectIt/`.

## The idea

ConnectIt is built as a **layered suite of game-agnostic plugins** with the game module
on top. Each plugin is meant to be reusable in another project — none of them know the
rules of ConnectIt. The game module (`../../Source/ConnectIt/`) is the only place
game-specific logic lives.

## Dependency graph

```
                 ConnectIt  (game module — Source/ConnectIt)
                    |
   +----------------+----------------+----------------+
   |                |                |                |
UnrealTurnBasedMechanics   UnrealGameIntelligence   UnrealCodingUtils
   |        |                (standalone)             (standalone)
   |        +--------> UnrealGridMechanics
   |                        |
   +------------------------+--------> UnrealGameMechanics  (base)
                                          ^
                                          |  (EnhancedInput)

UnrealAIMechanics  — standalone, DORMANT (not enabled)
UnrealUIMechanics  — standalone, STUB   (not enabled)
```

- **`UnrealGameMechanics`** is the base layer — the phase-barrier / tag-queue,
  turn bookkeeping, pooling, input tag binding, scoring and state-machine helpers.
- **`UnrealGridMechanics`** depends on it and adds the 2D board: coords, grid/shape math,
  tiles, pieces, registries, board state, shift.
- **`UnrealTurnBasedMechanics`** depends on both and adds the replicated match: action
  stack, participant manager state machine, turn-order strategies, framework actor bases.
  Its turn-end sequence routes through `UnrealGameMechanics`' `UGameEventTaskSubsystem`.
- **`UnrealGameIntelligence`**, **`UnrealCodingUtils`** — standalone, engine-only deps.
- **`UnrealAIMechanics`**, **`UnrealUIMechanics`** — standalone and not yet enabled in
  `ConnectIt.uproject`.
- **`AdvancedSessions-5-5`** — third-party (vendored), not integrated; not part of this
  suite.

## Enabled vs not

Enabled in `ConnectIt.uproject`: `UnrealGameMechanics`, `UnrealGridMechanics`,
`UnrealTurnBasedMechanics`, `UnrealGameIntelligence`, `UnrealCodingUtils`.
Not enabled: `UnrealAIMechanics` (dormant), `UnrealUIMechanics` (stub), both
AdvancedSessions modules.

## Cross-cutting conventions

- **Gameplay tags** drive sequencing: the phase-barrier fires tag-groups; the turn
  system gates turn-end on tags. Vocabulary is in `../../Config/DefaultGameplayTags.ini`.
- **Delegates** in `UnrealTurnBasedMechanics` are declared in dual form (dynamic +
  `_Native`); other plugins are less consistent.
- **Debug widgets** — `UnrealGameMechanics` provides `UDWidgetBase`; individual plugins
  add `UDWidget_*` subclasses. A future `UnrealUIMechanics` may consolidate this.
- **Extension hook** — `UnrealTurnBasedMechanics` carries `FInstancedStruct Payload` on
  its request/modifier structs so the game module can extend turn data without changing
  the plugin.

## Known suite-level tensions

- MinMax game-tree code exists **twice**: header-only templates in
  `UnrealGameIntelligence` and a parallel implementation in the game module. They should
  converge or one should be removed.
- `UnrealGridMechanics` has two overlapping hover/tracker subsystems.
- Several orphaned interfaces across plugins (see each `high-level.md`'s rough edges).
