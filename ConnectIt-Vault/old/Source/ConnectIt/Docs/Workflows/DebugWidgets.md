# Pattern: One Debug Widget Per Tracked Class

## Problem

A single monolithic debug widget that caches "board state," "turn state,"
"action state," etc. as grouped categories has two costs that compound as a
project grows. First, its refresh footprint is coarse: any one delegate
firing tends to trigger a refresh of every category, not just the one that
actually changed, which both wastes work and — as happened here — makes it
easy to refresh the wrong category or miss one entirely (a delegate fires,
`OnDebugStateUpdated()` broadcasts, but the specific `Refresh<Category>()`
that should have run never gets called because the category groupings don't
line up 1:1 with the delegates that actually exist). Second, its class
boundary doesn't match the codebase's real class boundary: "Turn State" as
a debug-widget concept doesn't correspond to any one class, so when the
underlying types change shape there's no single, obvious place to update.

## Pattern

Give each concrete class worth observing its own widget, named after
exactly the class it tracks, subclassing a single shared, minimal base.
Each concrete widget resolves a pointer to exactly one source object,
binds only the delegates that source class itself exposes, and pushes
each changed value straight through to its own dedicated
`BlueprintImplementableEvent` as it happens — no cached fields, no
`BlueprintPure` getters, no coarse "something changed, redraw everything"
signal. BP holds whatever state it wants in its own variables, seeded once
from the source's own `GetInfo()` and updated incrementally by the events
it receives.

```
UDWidgetBase (UnrealGameMechanics, generic)
  BindDelegates() / UnbindDelegates()  (virtual, empty)
  NativeConstruct() calls BindDelegates() once; NativeDestruct() mirrors it
      │
      ├─ DWidget_ConnectIt_BoardStateComponent   (ConnectIt)
      ├─ DWidget_ConnectIt_GameState             (ConnectIt)
      ├─ DWidget_TurnBasedParticipantManagerComponent (UnrealTurnBasedMechanics)
      ├─ DWidget_TurnBasedActionsComponent            (UnrealTurnBasedMechanics)
      └─ DWidget_GameEventTaskSubsystem               (UnrealGameMechanics)
```

Every source class a `DWidget` tracks follows a matching, symmetric
convention: a `GetInfo() const` `BlueprintPure` function returning a
per-class `F<Source>Info` struct with every field that class's widget
needs. A widget calls this exactly once, right after resolving its source
in `BindDelegates()`, to seed initial values through the *same* per-field
events it uses for every later reactive update — not a separate mechanism,
so there's no "seeded via one path, updated via another" drift to worry
about. `IsSourceValid()` — "has this resolved yet" — is the one
deliberately pull-only exception: checked once by BP before trusting any
pushed value, not itself pushed, since it's a one-time gate rather than a
recurring update.

Where a source delegate hands out a live, mutable object (e.g.
`UTurnBasedActionsComponent::OnActionPushed` carrying `UTurnBasedActionBase*`,
which has `Complete()`/`Cancel()` on it), the source class gains a *new*,
purely additive delegate carrying copied/safe data instead (e.g.
`OnActionPushedSafe`, carrying `FTurnActionSnapshot`) — broadcast alongside
the original at the same call site, not replacing it. The original stays
untouched for its existing non-debug consumers; only the new sibling is
debug-widget-facing. Constifying or reshaping the original was considered
and rejected — see Gotchas.

## Class ↔ Widget ↔ Location Mapping

The base class lives in the most foundational plugin (`UnrealGameMechanics`)
so any plugin or the game module can build on it. Each concrete widget lives
in **the module that owns the class it tracks** — not the module that
happens to be using the widget — and, if that module isn't the game module
itself, in a `Debug/` subfolder kept separate from the tracked class's own
folder (so `Debug/` reads as "instrumentation for classes defined
elsewhere in this module," not as part of the class's own feature folder).

| Widget | Tracks | Lives In | Header |
|---|---|---|---|
| `UDWidgetBase` | — (shared base, no tracked class) | `UnrealGameMechanics` | `Public/Widget/DWidgetBase.h` |
| `UDWidget_ConnectIt_BoardStateComponent` | `UConnectIt_BoardStateComponent` | `ConnectIt` (game module) | `Public/UI/DWidget_ConnectIt_BoardStateComponent.h` |
| `UDWidget_ConnectIt_GameState` | `AConnectIt_GameState` | `ConnectIt` (game module) | `Public/UI/DWidget_ConnectIt_GameState.h` |
| `UDWidget_TurnBasedParticipantManagerComponent` | `UTurnBasedParticipantManagerComponent` | `UnrealTurnBasedMechanics` | `Public/Debug/DWidget_TurnBasedParticipantManagerComponent.h` |
| `UDWidget_TurnBasedActionsComponent` | `UTurnBasedActionsComponent` | `UnrealTurnBasedMechanics` | `Public/Debug/DWidget_TurnBasedActionsComponent.h` |
| `UDWidget_GameEventTaskSubsystem` | `UGameEventTaskSubsystem` | `UnrealGameMechanics` | `Public/Debug/DWidget_GameEventTaskSubsystem.h` |

The rule that decides where a widget lives: **the tracked class's own
module**, full stop — never the module of whatever happens to be consuming
the widget today. `UTurnBasedParticipantManagerComponent` and
`UTurnBasedActionsComponent` are generic `UnrealTurnBasedMechanics` types
with no ConnectIt-specific knowledge, so their debug widgets moved there
even though ConnectIt is currently the only consumer — a second project
built on the same plugin stack gets them for free. `UConnectIt_BoardStateComponent`
and `AConnectIt_GameState` are genuinely ConnectIt-specific, so their
widgets stay in the game module.

## Naming Convention

`DWidget_<ClassName>`, where `<ClassName>` is the tracked class's name with
its `U`/`A`/`F` type prefix stripped (matching this project's general
convention of dropping the prefix when a class name is reused elsewhere,
e.g. in `Category` strings). The widget class itself still gets its own `U`
prefix, since it's a `UUserWidget` subclass in its own right — e.g.
`UConnectIt_BoardStateComponent` is tracked by `UDWidget_ConnectIt_BoardStateComponent`.
The shared base has no tracked class to name itself after, so it's just
`UDWidgetBase`.

## Participants / Classes Involved

| Role | Class | Note |
|---|---|---|
| Shared scaffolding | `UDWidgetBase` (`UnrealGameMechanics`, `UUserWidget`) | Just `BindDelegates()`/`UnbindDelegates()` (protected virtuals) plus the `bBound`-guarded call from `NativeConstruct()`/`NativeDestruct()`. Has no dependency on anything project- or plugin-specific beyond `UMG`, and no longer any notion of caching or refreshing — that's fully each concrete widget's own affair now. |
| Concrete widgets | `DWidget_*` (see mapping table above) | Each owns exactly one `UPROPERTY() TObjectPtr<T> ResolvedSource` and one `bool bSourceValid` — no other cached fields. One `BlueprintImplementableEvent` per pushed value, named `On<Field>Updated`. |
| Per-source seeding | `<Source>::GetInfo() const` (`BlueprintPure`, returns `F<Source>Info`) | Called once per widget, from `BindDelegates()` (or wherever the source actually resolves, if that's asynchronous — see `DWidget_ConnectIt_BoardStateComponent`'s ready-signal case), to push initial values through the same events used for later updates. |
| Safe delegate siblings | `On<Event>Safe` (only where the original hands out a mutable object — currently just `UTurnBasedActionsComponent`) | Carries a copied value struct (`FTurnActionSnapshot`) instead of the live pointer; broadcast alongside the original at the same call site, purely additive. |

## Sequence

1. `NativeConstruct()` (base class) calls `BindDelegates()` once.
2. A concrete `BindDelegates()` resolves `ResolvedSource` (via a subsystem, `GetOwningPlayer()->FindComponentByClass`, a utility-library getter, or a "ready" delegate if the source may not exist yet), binds every delegate that source exposes — the safe sibling where one exists, the original otherwise — to a small set of `UFUNCTION()` handlers, then calls `ResolvedSource->GetInfo()` once and pushes every field through its corresponding event to seed initial state.
3. Each handler pushes the specific value(s) implied by whatever just fired — directly, if the delegate's own parameter already *is* the safe value needed (`OnTurnPhaseChanged(ETurnPhase)` → `OnTurnPhaseUpdated(NewPhase)` directly); via a small derivation, if the delegate hands out something the widget has to translate (`OnActiveControllerChanged(AController*)` → read `ResolvedSource->ActiveParticipantIndex` directly rather than resolving an index from the controller); or by re-calling `GetInfo()` and pushing everything it returns, for zero-param pings that don't identify what changed (`OnBoardStateChanged`, `OnActiveManagerTagsChanged`, `OnAllParticipantsReady`, `OnGameOver`).
4. `NativeDestruct()` calls `UnbindDelegates()` once, mirroring step 2's binds exactly.

## Why It's Reusable

- **The base class has zero project knowledge.** `UDWidgetBase` doesn't know what a "board" or a "turn" is — it only knows the resolve/bind/unbind shape. Any project built on `UnrealGameMechanics` gets it for free.
- **A widget's home module is a mechanical decision, not a judgment call.** "Which module defines the tracked class" answers "where does the widget go" every time, which is also what keeps this pattern from silently drifting back toward a monolithic, game-module-only widget as new tracked classes get added.
- **Per-widget update footprint scales with the number of classes actually observed, not with the total state surface of the game** — and within one widget, with the number of fields that actually *changed*, not every field it tracks. Adding a new tracked class means adding one new small widget, never touching an existing one.
- **`GetInfo()` is the one convention every source class follows identically**, so "how does a new widget get its initial values" never needs re-deriving per class — it's always the same one call, seeded through the same events used afterward.

## Gotchas

- **This pattern exists because of a real bug, not as a preemptive best practice.** The predecessor monolithic widget (`UConnectIt_DebugStateWidget`, still present and intentionally untouched — see below) mapped several delegates to category-specific `Refresh<Category>()` calls; some categories only had *dead* or *intermittently-firing* delegates backing them (an empty stub, a phase enum that can "settle back" to its prior value within one replication tick and thus never re-fires its `OnRep`), so that category silently stopped updating while an unrelated, reliably-firing delegate kept the widget's top-level `OnDebugStateUpdated()` broadcasting anyway — everything looked alive, one category just quietly went stale. One-widget-per-class doesn't fix flaky source delegates by itself, but confining each widget to one class's own delegates removes the cross-category mapping step where the original bug lived, and pushing per-field rather than through one coarse signal removes a second, later version of the same risk (see next bullet).
- **The original per-widget design (before this push rewrite) had its own version of the same risk**: every handler called one shared `RefreshAll()`, which re-read *every* cached field regardless of which delegate fired — correct, but wasteful, and it meant a widget newly added to the viewport mid-match showed stale defaults until the *next* change if `RefreshAll()` were ever skipped on construct. The `GetInfo()`-seeded push model fixes both: only the field(s) implied by whatever fired get pushed, and initial state is seeded through the exact same path as every later update, not a separate "hope it ran once" call.
- **A tracked class's own replication/notification reliability still matters.** If the source class's delegate for a given change doesn't fire reliably (e.g. a property with no `ReplicatedUsing` on a monotonic value that should always notify), the widget built on top of it inherits that gap — see `UTurnBasedParticipantManagerComponent::TurnNumber`'s `OnRep_TurnNumber` fix, which existed before this widget family did but is exactly the kind of source-side fix this pattern depends on.
- **Never fold two classes' data into one widget for convenience.** `DWidget_ConnectIt_GameState` deliberately does not surface `AConnectIt_GameState`'s board-derived convenience wrappers (`GetFactionScore`, etc.) even though they're one function call away — that data already has a home in `DWidget_ConnectIt_BoardStateComponent`, and duplicating it here would reintroduce the "which widget is the source of truth for this field" ambiguity this pattern exists to avoid.
- **The original monolithic widget stays as-is, on purpose.** `UConnectIt_DebugStateWidget` (`Source/ConnectIt/Public/UI/ConnectIt_DebugStateWidget.h`) is not deprecated or replaced by this family — it remains a working, single-file, cross-referenced example wiring up every accessor in [RuntimeStateAccess.md](../RuntimeStateAccess.md). The two are parallel options, not a migration in progress. It also still owns the *only* remaining bindings to `UTurnBasedActionsComponent::OnActionPushed`/`OnActionPopped` (the raw-pointer originals) in the project — confirmed unaffected by the new `OnAction*Safe` siblings, since those are purely additive.
- **A delegate handing out a mutable object gets a new, additive sibling delegate — never a constified or reshaped version of the original.** Confirmed by tracing the actual blast radius before doing this for `UTurnBasedActionsComponent`: `FOnActionBaseEvent` (the type backing `OnActionPushed`/`OnActionPopped`) also drives `UTurnBasedActionBase::OnActivated`/`OnDeactivated`/`OnForceDeactivated`, and has a second, non-`DWidget` consumer (`UConnectIt_DebugStateWidget` itself) already bound to it — changing the original's signature would have needed every existing binder updated in lockstep, for a safety concern that's specific to debug/observer consumers, not the delegate's other uses. A new sibling delegate carrying copied data ripples through nothing.

## Source

Introduced as `ConnectIt_DebugWidget_*` inside the ConnectIt game module,
then relocated per-class to the plugin that actually owns each tracked type
and renamed to the shorter `DWidget_*` convention once the pattern proved
out — see the mapping table above for current locations.
