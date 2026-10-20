---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/Input/InputTagBinder.h
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Private/Input/InputTagBinder.cpp
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/Input/InputTagBinding.h
reconciled: 2026-09-18
commit: 9187568
---

# UInputTagBinder

Plain `UObject`. Builds and owns **one** `UInputMappingContext` from a list of
`FInputTagBinding` entries, and scopes it to an explicit `BindAll()` / `UnbindAll()`
lifetime instead of Enhanced Input's usual "add at BeginPlay, leave running". Every bound
`UInputAction` dispatches straight to **its own binding's** `InputActionDelegate` — no
shared broadcast delegate, no consumer switching on anything. `HandleInputTriggeredExecuteDelegate`
does a linear search of `Bindings` for the entry whose `InputAction` matches the fired
`FInputActionInstance` and calls `ExecuteIfBound()` on that entry's delegate.

## When you touch this

- Giving a dynamically-lived owner (an action-stack entry, a transient tool/mode) its own
  input context that comes and goes with it.
- Adding a new bound input: a new `FInputTagBinding` (`InputAction`, `TriggerEvent`, `Key`),
  with `InputActionDelegate` bound (e.g. `BindDynamic`) to whatever should run when it
  fires. No shared switch statement anywhere to extend.

## Entry points

- `Initialise(UEnhancedInputComponent*, UEnhancedInputLocalPlayerSubsystem*,
  const TArray<FInputTagBinding>&, MappingContextPriority = 1)` — call once; builds the
  context immediately (`GetMappingContext()` valid right after).
- `BindAll()` / `UnbindAll()` — idempotent add/remove of the context + action bindings.
- `GetMappingContext()` — exposed so a consumer with bespoke input needs (e.g.
  `UTurnBasedAction`'s hover+confirm selection) can add one more key mapping onto the
  same context.
- `FInputTagBinding`: `InputAction`, `ETriggerEvent` (default `Triggered`), `FKey`,
  `InputActionDelegate` (bound per-entry to the actual handler).

## Collaborators

- Both Enhanced Input dependencies are **injected** via `Initialise` (no `GetWorld()`).
- Consumers: `UnrealTurnBasedMechanics` `UTurnBasedAction` — overrides
  `ConstructInputBindings()` to populate `InputBindings`, called from `InitialiseAction`
  before this binder is constructed.

## Gotchas

- Dependencies are injected, not resolved — the owner must supply a valid
  `UEnhancedInputComponent` + local-player subsystem.
- `BindAll` is idempotent but `UnbindAll` clears **only** what this object registered —
  don't share one binder's component across binders.
- `HandleInputTriggeredExecuteDelegate`'s search doesn't discriminate by `TriggerEvent` and
  doesn't `break` after a match — two `Bindings` entries sharing the same `InputAction`
  (whether intentionally, for two different `TriggerEvent`s, or by accident) will both fire
  on every trigger of that action, regardless of which `TriggerEvent` actually happened.
  Matches the still-open `// TODO: check if there are duplicate Input Actions in bindings`
  in `UTurnBasedAction::InitialiseAction` — not yet guarded against.
- If `InputActionDelegate` isn't bound on a matching entry, `ExecuteIfBound()` fails and a
  `Warning` is logged (`InputTagBinder - HandleInputTriggeredExecuteDelegate`) — check logs
  if a configured input silently does nothing.

## Cross-impact

Change `FInputTagBinding`'s shape or the dispatch mechanism and also update every consumer's
`ConstructInputBindings()` override and any designer data holding `FInputTagBinding` arrays.

## Changes

- 2026-09-17 — dispatch model changed from a shared `OnInputTagTriggered` broadcast +
  consumer tag-switch to a per-binding `InputActionDelegate`; `BindingTag` field and the
  old broadcast delegate/handler removed as dead code once nothing read them.
- 2026-09-10 — re-ingested to the `_code` schema; provenance re-anchored.

## See also

- In-repo: [[UnrealGameMechanics/CLAUDE|UnrealGameMechanics overview]] → *Input*.
- Consumer: [[UnrealTurnBasedMechanics/code/UTurnBasedAction|UTurnBasedAction]]
