---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/State/StackedStateMachine.h
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Private/State/StackedStateMachine.cpp
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/State/StackedState.h
reconciled: 2026-09-06
commit: 99cdd6a
---

# UStackedStateMachine

`UObject`. A generic stack-based state machine — ported as-is from an earlier project,
renamed to be project-agnostic. Owns a stack of `UStackedState` entries. Use it for any
"mini state machine within a larger flow" (e.g. a multi-step action walking a player
through selection phases) instead of hand-rolling an enum + switch.

Covers `UStackedState` (the abstract per-entry base).

## When you touch this

- Building a system with several ordered sub-modes where each mode owns its own
  setup/teardown.
- Note: `SwapState` is a **`TODO` stub** — not implemented.

## Entry points

- `UStackedStateMachine` (methods are `protected` — subclass it):
  `SetBaseState(InState, bDeinitialize)` (replace whole stack),
  `PushState(InState, bDeinitialize)` (`bDeinitialize` = tear down vs. leave running
  underneath), `PopState()`, `PeakState()`, `PrintStateMachineInfo()` (debug).
- `UStackedState` (abstract, subclass per mode): `StateTag`, `PlayerController`,
  `Initialize()` / `Deinitialize()` (BlueprintNativeEvent). A state **requests**
  transitions by broadcasting `OnSetBaseState` / `OnPushState` / `OnPopState` — it never
  mutates the stack itself; the machine binds these on push, unbinds on pop.

## Collaborators

- Machine ↔ state via the three transition-request delegates.
- Ported deliberately *without* the source project's input-handler interface — concrete
  states bind their own input in `Initialize` / `Deinitialize` (same approach as
  `UTurnBasedAction`'s selection binding).

## Gotchas

- **`SwapState` does nothing** — don't call it expecting a replace-in-place.
- The public API is `protected` — you must subclass the machine; there's no
  ready-to-use concrete machine in the plugin.
- A state must not touch the stack directly; fire a transition delegate.
- `PushState(..., bDeinitialize=false)` leaves the state below live — make sure its
  `Initialize`d resources tolerate running while not on top.

## Cross-impact

Self-contained pattern. If you change the transition-delegate set, update every
`UStackedState` subclass and the machine's bind/unbind in `PushState` / `PopState`.

## See also

- In-repo: `old/Plugins/UnrealGameMechanics/Docs/README.md` → *State*; `old/Plugins/UnrealGameMechanics/Docs/Systems.md` → the two state-machine patterns
  (single-state `UGameMechanicsStateBase` vs. this stack-based one).
