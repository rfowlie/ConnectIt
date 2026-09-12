---
Date: 2026-09-06
status: Active
superseded by:
tags:
  - conventions
  - suite
---

## Decision

Cross-cutting conventions the whole plugin suite + game module follow, recorded so new
code lands consistent with what's already there:

- **Gameplay tags drive sequencing.** The phase-barrier (`UGameEventTaskSubsystem`) fires
  tag-groups; the turn system gates turn-end on tags. Vocabulary lives in
  `Config/DefaultGameplayTags.ini`. Tag strings are dot-hierarchical
  (`ConnectIt.Game.PlacePiece`); the C++ identifier replaces every `.` with `_`. Three
  families: `ConnectIt.Game.*` (phases + request-type routing), `ConnectIt.Tile.*`
  (tile visual feedback), `ConnectIt.Event.*` (board events consumed by
  `UGameEventTaskSubsystem`).
- **No `Client_` / `Multicast_` RPCs in `Source/ConnectIt`.** Every "tell everyone" need
  is met by a single authoritative replicated property the client converges on (see
  [[board-state-single-source-of-truth|code/systems/board-state-single-source-of-truth]]).
  Before adding a multicast RPC, check whether the need can be a replicated-property
  change instead — mixing both patterns for the same kind of event is what caused a past
  sequencing bug.
- **Extension hook:** `UnrealTurnBasedMechanics` carries `FInstancedStruct Payload` on
  its request / modifier structs, so the game module extends turn data without editing
  the plugin.
- **Debug widgets:** `UnrealGameMechanics` provides `UDWidgetBase`; each plugin adds its
  own `UDWidget_*` subclasses (one per tracked class). A future `UnrealUIMechanics` may
  consolidate this.
- **Delegate style is *not* uniform.** `UnrealTurnBasedMechanics` pairs every dynamic
  multicast delegate with a `_Native` non-dynamic sibling (cheaper C++ binding). The
  other three plugins declare dynamic-only. Whether the `_Native` pattern becomes the
  house style is an open owner decision — do not assume either way when adding a delegate.

## Why

The suite is built as layered, independently-extractable plugins with the game on top.
Without a written convention set, each class was being categorised / wired independently
and the drift (category-string spacing, `ClassGroup` values, a stale "Chimera" project
name in one metadata string, a `BlueprintreadWrite` casing bug) was being rediscovered
repeatedly. Recording the conventions once gives new code a target and the known
inconsistencies one home. Full drift list was in the in-repo `Conventions.md`.

## What Would Change It

- A deliberate decision to standardise the `_Native` delegate pattern across all plugins
  (or to declare it a `UnrealTurnBasedMechanics`-only optimisation).
- Introducing online-session code (`AdvancedSessions`) that genuinely needs client RPCs
  the replicated-property pattern can't express.
- A mechanical category-string cleanup pass once a canonical per-plugin prefix is agreed.
