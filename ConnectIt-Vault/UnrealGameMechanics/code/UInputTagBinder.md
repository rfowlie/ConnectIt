---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/Input/InputTagBinder.h
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Private/Input/InputTagBinder.cpp
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/Input/InputTagBinding.h
reconciled: 2026-09-06
commit: e28339c
---

# UInputTagBinder

Plain `UObject`. Builds and owns **one** `UInputMappingContext` from a designer list of
`FInputTagBinding` entries, and scopes it to an explicit `BindAll()` / `UnbindAll()`
lifetime instead of Enhanced Input's usual "add at BeginPlay, leave running". Every bound
`UInputAction` fires the **same** `OnInputTagTriggered` delegate, carrying the entry's
`BindingTag` — the consumer switches on the tag rather than binding one function per input.

## When you touch this

- Giving a dynamically-lived owner (an action-stack entry, a transient tool/mode) its own
  input context that comes and goes with it.
- Adding a new bound input as **data only** — new `FInputTagBinding`, one `case` in the
  consumer's tag switch, no new C++.

## Entry points

- `Initialise(UEnhancedInputComponent*, UEnhancedInputLocalPlayerSubsystem*,
  const TArray<FInputTagBinding>&, MappingContextPriority = 1)` — call once; builds the
  context immediately (`GetMappingContext()` valid right after).
- `BindAll()` / `UnbindAll()` — idempotent add/remove of the context + action bindings.
- `OnInputTagTriggered` (`FOnInputTagTriggered`, `(const FInputActionInstance&,
  FGameplayTag BindingTag)`).
- `GetMappingContext()` — exposed so a consumer with bespoke non-tag input (e.g.
  `UTurnBasedAction`'s hover+confirm selection) can add one more key mapping onto the
  same context.
- `FInputTagBinding`: `BindingTag`, `InputAction`, `ETriggerEvent` (default `Triggered`),
  `FKey`.

## Collaborators

- Both Enhanced Input dependencies are **injected** via `Initialise` (no `GetWorld()`).
- Consumers: `UnrealTurnBasedMechanics` `UTurnBasedAction` (`InputBindings` →
  `OnBoundInputTriggered`).

## Gotchas

- Dependencies are injected, not resolved — the owner must supply a valid
  `UEnhancedInputComponent` + local-player subsystem.
- `BindAll` is idempotent but `UnbindAll` clears **only** what this object registered —
  don't share one binder's component across binders.
- `TODO` in the header questions whether the `BindingTag` param is needed vs. reading it
  off `FInputActionInstance`; don't rely on that staying as-is.

## Cross-impact

Change `FInputTagBinding` or the dispatch signature and also update every consumer's tag
switch (`UTurnBasedAction::OnBoundInputTriggered`) and any designer data assets holding
`FInputTagBinding` arrays.

## See also

- In-repo: `old/Plugins/UnrealGameMechanics/Docs/README.md` → *Input*; `old/Plugins/UnrealGameMechanics/Docs/Systems.md` → input section.
- Consumer: [[UnrealTurnBasedMechanics/code/UTurnBasedAction|UTurnBasedAction]]
